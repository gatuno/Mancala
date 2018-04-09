/*
 * juego.c
 * This file is part of Mancala
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * Mancala is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Mancala is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mancala. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

/* Para el manejo de la red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif
#include <sys/types.h>

#include "mancala.h"
#include "juego.h"
//#include "netplay.h"
#include "draw-text.h"
//#include "message.h"
#include "ventana.h"

#define ANIM_VEL 2

enum {
	JUEGO_BUTTON_CLOSE = 0,
	
	JUEGO_NUM_BUTTONS
};

enum {
	ANIM_NONE = 0,
	ANIM_RAISE_STONE,
	ANIM_MOVE,
	ANIM_WAIT_DROP,
	ANIM_CAPTURE,
	ANIM_FREE_TURN,
	ANIM_CAPTURE_TURN,
	ANIM_WAIT_ACK
};

int juego_mouse_down (Ventana *v, int x, int y);
int juego_mouse_motion (Ventana *v, int x, int y);
int juego_mouse_up (Ventana *v, int x, int y);
void juego_draw_button_close (Ventana *v, int frame);
void juego_button_frame (Ventana *, int button, int frame);
void juego_button_event (Ventana *, int button);

void juego_dibujar_resalte (Juego *j);
void juego_dibujar_tablero (Juego *j);
void juego_invert_diffs (int *diffs);

/* Algunas constantes */
const int tazas_pos[14][2] = {
	{ 70, 114},
	{ 98, 114},
	{126, 114},
	{154, 114},
	{182, 114},
	{210, 114},
	
	{240, 102},
	
	{210,  84},
	{182,  84},
	{154,  84},
	{126,  84},
	{ 98,  84},
	{ 70,  84},
	
	{ 41, 102}
};

const float stones_offsets[5][2] = {
	{-10.75, -31.20},
	{ -9.70, -31.80},
	{-10.10, -28.90},
	{ -9.50, -31.50},
	{ -8.00, -30.70}
};

const int hint_pos[14][2] = {
	{ 59, 106},
	{ 87, 106},
	{115, 106}, /* 136 - 25 + 4  */ /* 124 - 38 + 20 */
	{143, 106},
	{171, 106},
	{199, 106},
	
	{226, 91},
	
	{199, 77},
	{171, 77},
	{143, 77},
	{115, 77},
	{ 87, 77},
	{ 59, 77},
	
	{ 31, 91}
};

static Juego *network_game_list = NULL;

Juego *get_game_list (void) {
	return network_game_list;
}

static int juego_check_next_cup (Juego *j, int next_cup) {
	int mancala_enemiga;
	
	if (j->inicio == j->turno) {
		mancala_enemiga = 13;
	} else {
		mancala_enemiga = 6;
	}
	
	if (next_cup == mancala_enemiga) {
		next_cup++;
	}
	
	if (next_cup > 13) {
		next_cup = 0;
	}
	
	return next_cup;
}

void juego_draw_board (Juego *j) {
	SDL_Surface *surface, *score_img;
	SDL_Rect rect, rect2;
	int g, h, i;
	MancalaStone *stone;
	
	surface = window_get_surface (j->ventana);
	
	/* Borrar la ventana */
	rect.x = 14;
	rect.y = 30;
	rect.w = images[IMG_PLAYER_1_NORMAL]->w;
	rect.h = images[IMG_PLAYER_1_NORMAL]->h * 2;
	
	window_update (j->ventana, &rect);
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	
	i = IMG_PLAYER_1_NORMAL;
	if (j->estado == NET_READY && j->turno != j->inicio) {
		i = IMG_PLAYER_1_HIGHLIGHT;
	}
	
	/* Dibujar la parte de arriba */
	rect.x = 14;
	rect.y = 30;
	rect.w = images[i]->w;
	rect.h = images[i]->h;
	
	SDL_BlitSurface (images[i], NULL, surface, &rect);
	
	/* Ahora la parte de abajo */
	i = IMG_PLAYER_2_NORMAL;
	if (j->estado != NET_SYN_SENT && j->turno == j->inicio) {
		i = IMG_PLAYER_2_HIGHLIGHT;
	}
	
	rect.x = 14;
	rect.y = 132;
	rect.w = images[i]->w;
	rect.h = images[i]->h;
	
	SDL_BlitSurface (images[i], NULL, surface, &rect);
	
	if (j->estado == NET_SYN_SENT) {
		/* Dibujar el icono de cargando en la parte superior */
		rect2.x = j->loading_timer * 43;
		rect2.y = 0;
		rect2.w = 43;
		rect2.h = 43;
		
		rect.x = 40;
		rect.y = 50;
		rect.w = 43;
		rect.h = 43;
		
		SDL_BlitSurface (images[IMG_WAITING], &rect2, surface, &rect);
		
		/* TODO: Dibujar el texto "Esperando jugador" */
	} else if (j->nick_remoto_image != NULL) {
		rect.x = 32;
		rect.y = 50;
		if (j->turno != j->inicio) {
			rect.w = j->nick_remoto_image->w;
			rect.h = j->nick_remoto_image->h;
		
			SDL_BlitSurface (j->nick_remoto_image, NULL, surface, &rect);
			
			score_img = score_n_black[j->score_other];
			
			rect.y = 54;
			rect.x = 272 - score_img->w;
			rect.h = score_img->h;
			rect.w = score_img->w;
			
			SDL_BlitSurface (score_img, NULL, surface, &rect);
		} else {
			rect.w = j->nick_remoto_image_blue->w;
			rect.h = j->nick_remoto_image_blue->h;
		
			SDL_BlitSurface (j->nick_remoto_image_blue, NULL, surface, &rect);
			
			score_img = score_n_white[j->score_other];
			
			rect.y = 54;
			rect.x = 272 - score_img->w;
			rect.h = score_img->h;
			rect.w = score_img->w;
			
			SDL_BlitSurface (score_img, NULL, surface, &rect);
		}
	}
	
	/* Dibujar el nick local */
	rect.x = 32;
	rect.y = 180;
	if (j->estado == NET_SYN_SENT || j->turno != j->inicio) {
		rect.w = nick_image_blue->w;
		rect.h = nick_image_blue->h;
		
		SDL_BlitSurface (nick_image_blue, NULL, surface, &rect);
		
		score_img = score_n_white[j->score];
		
		rect.y = 184;
		rect.x = 272 - score_img->w;
		rect.h = score_img->h;
		rect.w = score_img->w;
		
		SDL_BlitSurface (score_img, NULL, surface, &rect);
	} else if (j->estado == NET_WAIT_ACK && j->anim == ANIM_WAIT_ACK) {
		/* Cuando estamos esperando una confirmación, dibujar el circulo de cargando a un lado del nick */
		rect2.x = j->loading_timer * 43;
		rect2.y = 0;
		rect2.w = 43;
		rect2.h = 43;
		
		rect.x = 30;
		rect.y = 168;
		rect.w = 43;
		rect.h = 43;
		
		SDL_BlitSurface (images[IMG_WAITING], &rect2, surface, &rect);
		
		rect.x = 75;
		rect.y = 180;
		rect.w = nick_image->w;
		rect.h = nick_image->h;
		
		SDL_BlitSurface (nick_image, NULL, surface, &rect);
		
		score_img = score_n_black[j->score];
		
		rect.y = 184;
		rect.x = 272 - score_img->w;
		rect.h = score_img->h;
		rect.w = score_img->w;
		
		SDL_BlitSurface (score_img, NULL, surface, &rect);
	} else {
		rect.w = nick_image->w;
		rect.h = nick_image->h;
		
		SDL_BlitSurface (nick_image, NULL, surface, &rect);
		
		score_img = score_n_black[j->score];
		
		rect.y = 184;
		rect.x = 272 - score_img->w;
		rect.h = score_img->h;
		rect.w = score_img->w;
		
		SDL_BlitSurface (score_img, NULL, surface, &rect);
	}
	
	/* Dibujar el tablero */
	rect.x = 29;
	rect.y = 95;
	rect.w = images[IMG_BOARD]->w;
	rect.h = images[IMG_BOARD]->h;
	
	SDL_BlitSurface (images[IMG_BOARD], NULL, surface, &rect);
	
	/* Dibujar las piedras */
	for (g = 13; g >= 0; g--) {
		stone = j->tablero[g];
		
		while (stone != NULL) {
			rect.x = 14.0 + stone->x + stones_offsets[stone->color][0];
			rect.y = 30.5 + stone->y + stones_offsets[stone->color][1];
			i = 0;
			if (stone->frame > 0 && stone->frame <= 5) {
				i = stone->frame - 1;
			} else if (stone->frame == 6) {
				i = 4;
				stone->frame++;
			} else if (stone->frame == 7) {
				i = 5;
				stone->frame++;
			} else if (stone->frame == 8) {
				i = 6;
				stone->frame++;
			} else if (stone->frame == 9) {
				i = 7;
				stone->frame = 0;
			}
			h = IMG_STONE_1_0 + i + (stone->color * 8);
			rect.w = images[h]->w;
			rect.h = images[h]->h;
			
			SDL_BlitSurface (images[h], NULL, surface, &rect);
			
			stone = stone->next;
		}
	}
	
	/* Dibujar las piedras en la mano */
	stone = j->mano;
	
	while (stone != NULL) {
		rect.x = 14.0 + stone->x + stones_offsets[stone->color][0];
		rect.y = 30.5 + stone->y + stones_offsets[stone->color][1];
		if (stone->frame == 1 || stone->frame == 2) {
			i = 1;
		} else if (stone->frame == 3 || stone->frame == 4) {
			i = 2;
		} else {
			i = 3;
		}
		
		h = IMG_STONE_1_0 + i + (stone->color * 8);
		rect.w = images[h]->w;
		rect.h = images[h]->h;
		
		SDL_BlitSurface (images[h], NULL, surface, &rect);
		
		stone = stone->next;
	}
}

void juego_first_time_draw (Juego *j) {
	SDL_Surface *surface;
	
	surface = window_get_surface (j->ventana);
	
	SDL_FillRect (surface, NULL, 0); /* Transparencia total */
	
	SDL_SetAlpha (images[IMG_WINDOW], 0, 0);
	
	SDL_BlitSurface (images[IMG_WINDOW], NULL, surface, NULL);
	
	juego_draw_board (j);
	
	window_flip (j->ventana);
}

int juego_timer_callback_cargando (Ventana *v) {
	Juego *j;
	SDL_Rect rect, rect2;
	SDL_Surface *surface;
	
	surface = window_get_surface (v);
	
	j = (Juego *) window_get_data (v);
	
	j->loading_timer++;
	if (j->loading_timer >= 19) j->loading_timer = 0;
	
	juego_draw_board (j);
	
	return TRUE;
}

static int juego_draw_hint (Ventana *v) {
	Juego *j;
	SDL_Rect rect, rect2;
	SDL_Surface *surface, *texto;
	SDL_Color negro;
	char buffer[20];
	
	j = (Juego *) window_get_data (v);
	surface = window_get_surface (v);
	
	
	if (j->hint == -1) {
		return FALSE;
	}
	
	j->hint_frame++;
	
	if (j->hint_frame > 6) {
		
		/* Si aun estamos esperando un ACK, cambiar la función */
		if (j->estado == NET_WAIT_ACK) {
			window_register_timer_events (j->ventana, juego_timer_callback_cargando);
			return TRUE;
		}
		return FALSE;
	}
	
	if (j->hint_frame < 3) {
		/* Aún no hay nada que dibujar */
		return TRUE;
	}
	
	juego_draw_board (j);
	
	/* Taza 2: 136.25, 124.25 */
	rect.x = hint_pos[j->hint][0]; /* 136 - 25 + 4  */
	rect.y = hint_pos[j->hint][1]; /* 124 - 38 + 20 */
	rect.w = images[IMG_FLECHA]->w / 4;
	rect.h = images[IMG_FLECHA]->h / 2;
	
	rect2.x = rect.w * (j->hint_frame - 3);
	rect2.y = rect.h;
	rect2.w = rect.w;
	rect2.h = rect.h;
	
	negro.r = negro.g = negro.b = 255;
	
	if (j->estado == NET_WAIT_ACK) {
		rect2.y = 0;
		negro.r = negro.g = negro.b = 0;
	} else if (j->turno == j->inicio && j->hint >= 0 && j->hint < 6) {
		rect2.y = 0;
		negro.r = negro.g = negro.b = 0;
	}
	
	SDL_BlitSurface (images[IMG_FLECHA], &rect2, surface, &rect);
	
	if (j->hint_frame == 6) {
		/* Renderizar el número */
		sprintf (buffer, "%i", j->mapa[j->hint]);
		
		texto = TTF_RenderUTF8_Blended (ttf16_burbank_small, buffer, negro);
	
		rect.x = hint_pos[j->hint][0] + 25 - (texto->w / 2);
		rect.y = 10 + hint_pos[j->hint][1];
		rect.w = texto->w;
		rect.h = texto->h;
	
		SDL_BlitSurface (texto, NULL, surface, &rect);
	}
	
	return TRUE;
}

static void juego_drop_stone (Juego *j, MancalaStone *current_stone, int cup) {
	MancalaStone *stone;
	float loc8;
	
	current_stone->frame = 6;
	current_stone->next = NULL;
	
	j->mapa[cup]++;
	if (j->tablero[cup] == NULL) {
		j->tablero[cup] = current_stone;
	} else {
		stone = j->tablero[cup];
		while (stone->next != NULL) {
			stone = stone->next;
		}
		
		stone->next = current_stone;
	}
	
	if (cup == 6 || cup == 13) {
		/* Reproducir sonido al 50 % */
	} else {
		/* Reproducir sonido al 20 % */
	}
	
	/* Re-acomodar la piedra con un nuevo X,Y */
	loc8 = 6.283185 * rand() / (RAND_MAX + 1.0);
	if (cup == 6 || cup == 13) {
		current_stone->x = ((float) tazas_pos[cup][0]) + sin (loc8) * (10.0 * rand() / (RAND_MAX + 1.0));
		current_stone->y = ((float) tazas_pos[cup][1]) + cos (loc8) * (25.0 * rand() / (RAND_MAX + 1.0));
	} else {
		current_stone->x = ((float) tazas_pos[cup][0]) + sin (loc8) * (7.5 * rand() / (RAND_MAX + 1.0));
		current_stone->y = ((float) tazas_pos[cup][1]) + cos (loc8) * (7.5 * rand() / (RAND_MAX + 1.0));
	}
}

static void juego_draw_message (Ventana *v) {
	SDL_Surface *surface;
	SDL_Rect rect;
	Juego *j;
	
	j = (Juego *) window_get_data (v);
	surface = window_get_surface (v);
	
	juego_draw_board (j);
	
	rect.x = 14;
	rect.y = 30;
	rect.w = images[IMG_PLAYER_GREY_SCREEN]->w;
	rect.h = images[IMG_PLAYER_GREY_SCREEN]->h;
	
	SDL_BlitSurface (images[IMG_PLAYER_GREY_SCREEN], NULL, surface, &rect);
	
	if ((j->move_counter >= 0 && j->move_counter < 5) ||
	    j->move_counter >= 55) {
		return;
	}
	
	if (j->anim == ANIM_FREE_TURN) {
		if (j->move_counter == 5 || j->move_counter == 52) {
			rect.x = 152 - (free_turn_text[0]->w / 2);
			rect.y = 115 - (free_turn_text[0]->h / 2);
			rect.w = free_turn_text[0]->w;
			rect.h = free_turn_text[0]->h;
		
			SDL_BlitSurface (free_turn_text[0], NULL, surface, &rect);
			if (j->move_counter == 52) {
				rect.x = 152 - (go_again_text[2]->w / 2);
				rect.y = 148 - (go_again_text[2]->h / 2);
				rect.w = go_again_text[2]->w;
				rect.h = go_again_text[2]->h;
			
				SDL_BlitSurface (go_again_text[2], NULL, surface, &rect);
			}
		} else if (j->move_counter == 6 || j->move_counter == 51) {
			rect.x = 152 - (free_turn_text[1]->w / 2);
			rect.y = 115 - (free_turn_text[1]->h / 2);
			rect.w = free_turn_text[1]->w;
			rect.h = free_turn_text[1]->h;
		
			SDL_BlitSurface (free_turn_text[1], NULL, surface, &rect);
			if (j->move_counter == 51) {
				rect.x = 152 - (go_again_text[3]->w / 2);
				rect.y = 148 - (go_again_text[3]->h / 2);
				rect.w = go_again_text[3]->w;
				rect.h = go_again_text[3]->h;
			
				SDL_BlitSurface (go_again_text[3], NULL, surface, &rect);
			}
		} else if (j->move_counter == 7 || j->move_counter == 50) {
			rect.x = 152 - (free_turn_text[2]->w / 2);
			rect.y = 115 - (free_turn_text[2]->h / 2);
			rect.w = free_turn_text[2]->w;
			rect.h = free_turn_text[2]->h;
		
			SDL_BlitSurface (free_turn_text[2], NULL, surface, &rect);
			if (j->move_counter == 50) {
				rect.x = 152 - (go_again_text[3]->w / 2);
				rect.y = 148 - (go_again_text[3]->h / 2);
				rect.w = go_again_text[3]->w;
				rect.h = go_again_text[3]->h;
			
				SDL_BlitSurface (go_again_text[3], NULL, surface, &rect);
			}
		} else if (j->move_counter >= 8 && j->move_counter < 20) {
			rect.x = 152 - (free_turn_text[3]->w / 2);
			rect.y = 115 - (free_turn_text[3]->h / 2);
			rect.w = free_turn_text[3]->w;
			rect.h = free_turn_text[3]->h;
		
			SDL_BlitSurface (free_turn_text[3], NULL, surface, &rect);
		} else if (j->move_counter == 20) {
			rect.x = 152 - (free_turn_text[3]->w / 2);
			rect.y = 115 - (free_turn_text[3]->h / 2);
			rect.w = free_turn_text[3]->w;
			rect.h = free_turn_text[3]->h;
		
			SDL_BlitSurface (free_turn_text[3], NULL, surface, &rect);
		
			rect.x = 152 - (go_again_text[0]->w / 2);
			rect.y = 148 - (go_again_text[0]->h / 2);
			rect.w = go_again_text[0]->w;
			rect.h = go_again_text[0]->h;
		
			SDL_BlitSurface (go_again_text[0], NULL, surface, &rect);
		} else if (j->move_counter == 21) {
			rect.x = 152 - (free_turn_text[3]->w / 2);
			rect.y = 115 - (free_turn_text[3]->h / 2);
			rect.w = free_turn_text[3]->w;
			rect.h = free_turn_text[3]->h;
			
			SDL_BlitSurface (free_turn_text[3], NULL, surface, &rect);
			
			rect.x = 152 - (go_again_text[1]->w / 2);
			rect.y = 148 - (go_again_text[1]->h / 2);
			rect.w = go_again_text[1]->w;
			rect.h = go_again_text[1]->h;
			
			SDL_BlitSurface (go_again_text[1], NULL, surface, &rect);
		} else if (j->move_counter == 22) {
			rect.x = 152 - (free_turn_text[3]->w / 2);
			rect.y = 115 - (free_turn_text[3]->h / 2);
			rect.w = free_turn_text[3]->w;
			rect.h = free_turn_text[3]->h;
			
			SDL_BlitSurface (free_turn_text[3], NULL, surface, &rect);
			
			rect.x = 152 - (go_again_text[2]->w / 2);
			rect.y = 148 - (go_again_text[2]->h / 2);
			rect.w = go_again_text[2]->w;
			rect.h = go_again_text[2]->h;
			
			SDL_BlitSurface (go_again_text[2], NULL, surface, &rect);
		} else if (j->move_counter >= 23 && j->move_counter < 50) {
			rect.x = 152 - (free_turn_text[3]->w / 2);
			rect.y = 115 - (free_turn_text[3]->h / 2);
			rect.w = free_turn_text[3]->w;
			rect.h = free_turn_text[3]->h;
			
			SDL_BlitSurface (free_turn_text[3], NULL, surface, &rect);
			
			rect.x = 152 - (go_again_text[3]->w / 2);
			rect.y = 148 - (go_again_text[3]->h / 2);
			rect.w = go_again_text[3]->w;
			rect.h = go_again_text[3]->h;
			
			SDL_BlitSurface (go_again_text[3], NULL, surface, &rect);
		} else if (j->move_counter == 53) {
			rect.x = 152 - (go_again_text[1]->w / 2);
			rect.y = 148 - (go_again_text[1]->h / 2);
			rect.w = go_again_text[1]->w;
			rect.h = go_again_text[1]->h;
			
			SDL_BlitSurface (go_again_text[1], NULL, surface, &rect);
		} else if (j->move_counter == 54) {
			rect.x = 152 - (go_again_text[0]->w / 2);
			rect.y = 148 - (go_again_text[0]->h / 2);
			rect.w = go_again_text[0]->w;
			rect.h = go_again_text[0]->h;
			
			SDL_BlitSurface (go_again_text[0], NULL, surface, &rect);
		}
	} else if (j->anim == ANIM_CAPTURE_TURN) {
		if (j->move_counter == 5 || j->move_counter == 52) {
			rect.x = 152 - (capture_text[0]->w / 2);
			rect.y = 130 - (capture_text[0]->h / 2);
			rect.w = capture_text[0]->w;
			rect.h = capture_text[0]->h;
		
			SDL_BlitSurface (capture_text[0], NULL, surface, &rect);
		} else if (j->move_counter == 6 || j->move_counter == 51) {
			rect.x = 152 - (capture_text[1]->w / 2);
			rect.y = 130 - (capture_text[1]->h / 2);
			rect.w = capture_text[1]->w;
			rect.h = capture_text[1]->h;
		
			SDL_BlitSurface (capture_text[1], NULL, surface, &rect);
		} else if (j->move_counter == 7 || j->move_counter == 50) {
			rect.x = 152 - (capture_text[2]->w / 2);
			rect.y = 130 - (capture_text[2]->h / 2);
			rect.w = capture_text[2]->w;
			rect.h = capture_text[2]->h;
		
			SDL_BlitSurface (capture_text[2], NULL, surface, &rect);
		} else if (j->move_counter >= 8 && j->move_counter < 50) {
			rect.x = 152 - (capture_text[2]->w / 2);
			rect.y = 130 - (capture_text[2]->h / 2);
			rect.w = capture_text[2]->w;
			rect.h = capture_text[2]->h;
		
			SDL_BlitSurface (capture_text[2], NULL, surface, &rect);
		}
	}
}

static void juego_recalc_scores (Juego *j) {
	int g;
	int total;
	
	total = 0;
	for (g = 0; g <= 6; g++) {
		total += j->mapa[g];
	}
	
	j->score = total;
	
	total = 0;
	for (g = 7; g <= 13; g++) {
		total += j->mapa[g];
	}
	
	j->score_other = total;
}

static int juego_move_stones (Ventana *v) {
	Juego *j;
	MancalaStone *stone, *current_stone, **prev_stone;
	int g, h;
	
	j = (Juego *) window_get_data (v);
	
	if (j->anim == ANIM_RAISE_STONE) {
		stone = j->mano;
		
		while (stone != NULL) {
			stone->frame++;
			
			stone = stone->next;
		}
		
		if (j->mano->frame == 5) {
			j->anim = ANIM_MOVE;
			j->move_counter = 0;
		}
		
		juego_draw_board (j);
	} else if (j->anim == ANIM_MOVE) {
		if (j->move_counter > 2) { // dropFrameWait = 2
			j->move_counter = 0;
			
			if (j->mano != NULL) {
				j->move_next_cup = juego_check_next_cup (j, j->move_next_cup);
				j->move_last_cup = j->move_next_cup;
				/* Desligar la última piedra en la mano */
				current_stone = j->mano;
				prev_stone = &j->mano;
				while (current_stone->next != NULL) {
					prev_stone = &current_stone->next;
					current_stone = current_stone->next;
				}
				*prev_stone = NULL;
				
				/* Soltar la piedra en la taza */
				juego_drop_stone (j, current_stone, j->move_next_cup);
				
				j->move_next_cup++;
			} else {
				/* Detener animación */
				j->anim = ANIM_WAIT_DROP;
			}
		} else {
			j->move_counter++;
		}
		juego_draw_board (j);
	} else if (j->anim == ANIM_WAIT_DROP) {
		/* Esperar a que todas las piedras hayan "caido" */
		h = FALSE;
		
		for (g = 0; g < 14; g++) {
			stone = j->tablero[g];
			
			while (stone != NULL) {
				if (stone->frame != 0) {
					h = TRUE;
					g = 14;
					break;
				}
				stone = stone->next;
			}
		}
		
		if (h == FALSE) {
			/* Aquí checar el turno y cambiar */
			/* Si la última piedra ya cayó, revisar si hay captura o juega de nuevo */
			if (j->move_last_cup == 6 || j->move_last_cup == 13) {
				j->anim = ANIM_FREE_TURN;
				j->move_counter = 0;
			} else if (j->mapa[j->move_last_cup] == 1 && j->inicio == j->turno && j->move_last_cup < 6) {
				/* Captura */
				j->anim = ANIM_CAPTURE;
				j->move_counter = 0;
			} else if (j->mapa[j->move_last_cup] == 1 && j->inicio != j->turno && j->move_last_cup > 6 && j->move_last_cup < 13) {
				/* Captura */
				j->anim = ANIM_CAPTURE;
				j->move_counter = 0;
			} else {
				/* Cambiar el turno */
				juego_recalc_scores (j);
				j->turno++;
				if (j->turno > 1) j->turno = 0;
				
				if (j->estado == NET_WAIT_ACK) {
					j->anim = ANIM_WAIT_ACK;
					j->loading_timer = 0;
					juego_draw_board (j);
					return TRUE;
				}
				
				j->anim = ANIM_NONE;
				juego_draw_board (j);
				
				return FALSE;
			}
		}
	} else if (j->anim == ANIM_FREE_TURN || j->anim == ANIM_CAPTURE_TURN) {
		juego_draw_message (v);
		j->move_counter++;
		
		if (j->move_counter > 60) {
			if (j->anim == ANIM_CAPTURE_TURN) {
				/* Cambiar el turno */
				juego_recalc_scores (j);
				j->turno++;
				if (j->turno > 1) j->turno = 0;
			}
			
			if (j->estado == NET_WAIT_ACK) {
				j->anim = ANIM_WAIT_ACK;
				j->loading_timer = 0;
				juego_draw_board (j);
				return TRUE;
			}
			
			juego_draw_board (j);
			j->anim = ANIM_NONE;
			return FALSE;
		}
	} else if (j->anim == ANIM_CAPTURE) {
		h = 12 - j->move_last_cup;
		
		if (j->move_counter > 2) { // dropFrameWait = 2
			j->move_counter = 0;
			
			if (j->tablero[h] != NULL) {
				current_stone = j->tablero[h];
				
				j->tablero[h] = current_stone->next;
				j->mapa[h]--;
				
				/* Ahora, poner esta piedra en el espacio de la mancala */
				if (j->move_last_cup < 6) {
					g = 6;
				} else {
					g = 13;
				}
				juego_drop_stone (j, current_stone, g);
			} else {
				/* Cuando la última piedra del oponente es enviada al mancala, mandar la piedra que causó la captura */
				current_stone = j->tablero[j->move_last_cup];
				j->tablero[j->move_last_cup] = NULL;
				j->mapa[j->move_last_cup] = 0;
				
				/* Ahora, poner esta piedra en el espacio de la mancala */
				if (j->move_last_cup < 6) {
					g = 6;
				} else {
					g = 13;
				}
				juego_drop_stone (j, current_stone, g);
				
				j->anim = ANIM_CAPTURE_TURN;
				j->move_counter = 0;
				return TRUE;
			}
		} else {
			j->move_counter++;
		}
		
		juego_draw_board (j);
	} else if (j->anim == ANIM_WAIT_ACK) {
		j->loading_timer++;
		if (j->loading_timer >= 19) j->loading_timer = 0;
		
		juego_draw_board (j);
	}
	
	return TRUE;
}

Juego *crear_juego (int top_window) {
	Juego *j, *lista;
	int correct;
	int g;
	
	/* Crear un nuevo objeto Juego */
	j = (Juego *) malloc (sizeof (Juego));
	
	/* Crear una ventana */
	j->ventana = window_create (308, 248, top_window);
	window_set_data (j->ventana, j);
	
	window_register_mouse_events (j->ventana, juego_mouse_down, juego_mouse_motion, juego_mouse_up);
	window_register_buttons (j->ventana, JUEGO_NUM_BUTTONS, juego_button_frame, juego_button_event);
	//window_register_keyboard_events (j->ventana, juego_key_down, NULL);
	
	/* Valores propios del juego */
	memset (j->mapa, 0, sizeof (j->mapa));
	
	j->hint = -1;
	j->hint_frame = 0;
	j->anim = ANIM_NONE;
	
	j->mano = NULL;
	
	j->inicio = 0;
	j->turno = 0;
	j->loading_timer = 0;
	j->mov = 0;
	j->last.cup_sent = -1;
	j->last.next = NULL;
	j->queue_movs = NULL;
	
	j->score = j->score_other = 0;
	
	j->estado = NET_CLOSED;
	j->retry = 0;
	j->nick_remoto_image = NULL;
	j->nick_remoto_image_blue = NULL;
	
	/* Armar el tablero */
	for (g = 0; g < 14; g++) {
		j->tablero[g] = NULL;
	}
	
	/* Generar un número local aleatorio */
	do {
		correct = 0;
		j->local = 1 + RANDOM (65534);
		
		lista = network_game_list;
		while (lista != NULL) {
			if (lista->local == j->local) correct = 1;
			lista = lista->next;
		}
	} while (correct);
	
	/* Ligar el nuevo objeto juego */
	j->next = network_game_list;
	network_game_list = j;
	
	/* Dibujar la ventana la primera vez */
	window_register_timer_events (j->ventana, juego_timer_callback_cargando);
	window_enable_timer (j->ventana);
	
	juego_first_time_draw  (j);
	
	return j;
}

#if 0
void eliminar_juego (Juego *j) {
	/* Desligar completamente */
	if (j->ventana != NULL) {
		/* Destruir la ventana asociada con este juego */
		window_destroy (j->ventana);
		j->ventana = NULL;
	}
	
	if (j->nick_remoto_image != NULL) SDL_FreeSurface (j->nick_remoto_image);
	if (j->nick_remoto_image_blue != NULL) SDL_FreeSurface (j->nick_remoto_image_blue);
	
	/* Sacar de la lista ligada simple */
	if (network_game_list == j) {
		/* Primero */
		network_game_list = j->next;
	} else {
		Juego *prev;
		
		prev = network_game_list;
		while (prev->next != j) prev = prev->next;
		
		prev->next = j->next;
	}
	
	free (j);
}
#endif

void juego_button_frame (Ventana *v, int button, int frame) {
	Juego *j;
	#if 0
	if (button == JUEGO_BUTTON_CLOSE) {
		/* Redibujar el botón */
		juego_draw_button_close (v, frame);
		j = (Juego *) window_get_data (v);
		j->close_frame = frame;
	}
	#endif
}

void juego_button_event (Ventana *v, int button) {
	Juego *j;
	#if 0
	if (button == JUEGO_BUTTON_CLOSE) {
		/* Quitar esta ventana */
		j = (Juego *) window_get_data (v);
		if (j->estado != NET_CLOSED) {
			j->last_fin = NET_USER_QUIT;
			j->retry = 0;
			/* Destruir la ventana asociada con este juego */
			window_destroy (j->ventana);
			j->ventana = NULL;
			
			enviar_fin (j);
		} else {
			eliminar_juego (j);
		}
	}
	#endif
}

int juego_mouse_down (Ventana *v, int x, int y) {
	Juego *j;
	j = (Juego *) window_get_data (v);
	
	if (x >= 102 && x < 206 && y < 22) {
		/* Click por el agarre */
		window_start_drag (v, x, y);
		return TRUE;
	#if 0
	} else if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* El click cae en el botón de cierre de la ventana */
		/* FIXME: Arreglar lo de los botones */
		window_button_mouse_down (v, JUEGO_BUTTON_CLOSE);
		return TRUE;
	#endif
	} else if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	return FALSE;
}

int juego_mouse_motion (Ventana *v, int x, int y) {
	Juego *j;
	int new_hint;
	
	j = (Juego *) window_get_data (v);
	
	if (j->estado == NET_READY) {
		new_hint = -1;
	
		if (x >= 42 && x < 67 && y >= 105 && y < 155) {
			new_hint = 13;
		} else if (x >= 239 && x < 264 && y >= 105 && y < 155) {
			new_hint = 6;
		} else if (y >= 100 && y < 130) {
			if (x >= 69 && x < 97) { // 99
				new_hint = 12;
			} else if (x >= 97 && x < 125) { // 127
				new_hint = 11;
			} else if (x >= 125 && x < 153) { //155
				/* Hint de la taza 2 */
				new_hint = 10;
			} else if (x >= 153 && x < 183) {
				new_hint = 9;
			} else if (x >= 183 && x < 211) {
				new_hint = 8;
			} else if (x >= 212 && x < 239) {
				new_hint = 7;
			}
		} else if (y >= 130 && y < 159) {
			if (x >= 69 && x < 97) { // 99
				new_hint = 0;
			} else if (x >= 97 && x < 125) { // 127
				new_hint = 1;
			} else if (x >= 125 && x < 153) { //155
				/* Hint de la taza 2 */
				new_hint = 2;
			} else if (x >= 153 && x < 183) {
				new_hint = 3;
			} else if (x >= 183 && x < 211) {
				new_hint = 4;
			} else if (x >= 212 && x < 239) {
				new_hint = 5;
			}
		}
	
		if (j->anim == ANIM_NONE) {
			if (new_hint != -1 && new_hint != j->hint) {
				/* Activar el timer para dibujar el hint de la taza */
				j->hint = new_hint;
				j->hint_frame = 0;
		
				window_register_timer_events (v, juego_draw_hint);
				window_enable_timer (v);
			
				//printf ("Activando hint\n");
				return TRUE;
			} else if (new_hint == -1 && j->hint != -1) {
				//printf ("Desactivando hint\n");
				j->hint = -1;
				window_disable_timer (v);
			
				/* Redibujar el tablero para borrar los hints */
				juego_draw_board (j);
			}
		}
	}
	
	#if 0
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* FIXME: Arreglar lo de los botones */
		window_button_mouse_motion (v, JUEGO_BUTTON_CLOSE);
		return TRUE;
	}
	#endif
	if ((y >= 16) || (x >= 102 && x < 206)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	return FALSE;
}

int juego_mouse_up (Ventana *v, int x, int y) {
	int g, h;
	Juego *j;
	int move;
	MancalaStone *stone, *next;
	
	j = (Juego *) window_get_data (v);
	
	if (j->estado == NET_READY) {
		move = -1;
		
		if (j->inicio == j->turno && y >= 130 && y < 159) {
			if (x >= 69 && x < 97) { // 99
				move = 0;
			} else if (x >= 97 && x < 125) { // 127
				move = 1;
			} else if (x >= 125 && x < 153) { //155
				/* Hint de la taza 2 */
				move = 2;
			} else if (x >= 153 && x < 183) {
				move = 3;
			} else if (x >= 183 && x < 211) {
				move = 4;
			} else if (x >= 212 && x < 239) {
				move = 5;
			}
		}
		
		if (j->anim == ANIM_NONE && move != -1 && j->mapa[move] > 0 && j->inicio == j->turno) {
			j->anim = ANIM_RAISE_STONE;
			
			/* Enviar el movimiento por red */
			h = juego_simulate (j, move, j->last.cups_diffs);
			
			j->last.mov = j->mov;
			j->last.cup_sent = move;
			j->last.effect = h;
			
			if (j->inicio == 1) {
				/* Invertir los valores para enviar por red */
				j->last.cup_sent = (j->last.cup_sent + 7) % 14;
				
				juego_invert_diffs (j->last.cups_diffs);
			}
			
			enviar_movimiento (j, j->last.mov, j->last.cup_sent, j->last.effect);
			j->mov++;
			j->loading_timer = 0;
			
			stone = j->tablero[move];
			j->mano = NULL;
			j->move_next_cup = move + 1;
			while (stone != NULL) {
				next = stone->next;
			
				stone->next = j->mano;
				j->mano = stone;
			
				stone->frame = 1;
				stone = next;
			}
		
			j->mapa[move] = 0;
			j->tablero[move] = NULL;
		
			/* Activar el timer para levantar las piedras */
			window_register_timer_events (v, juego_move_stones);
			window_enable_timer (v);
		}
	}
	
	#if 0
	/* Revisar si el evento cae dentro del botón de cierre */
	if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* El click cae en el botón de cierre de la ventana */
		window_button_mouse_up (v, JUEGO_BUTTON_CLOSE);
		return TRUE;
	}
	#endif
	
	if ((y >= 16) || (x >= 102 && x < 206)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

void juego_draw_button_close (Ventana *v, int frame) {
	SDL_Surface *surface = window_get_surface (v);
	SDL_Rect rect;
	#if 0
	/* Dibujar el botón de cierre */
	rect.x = 192;
	rect.y = 26;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	
	SDL_BlitSurface (images[IMG_BUTTON_CLOSE_UP + frame], NULL, surface, &rect);
	window_update (v, &rect);
	#endif
}

void recibir_nick (Juego *j, const char *nick) {
	SDL_Color blanco;
	SDL_Color negro;
	SDL_Rect rect;
	SDL_Surface *surface;
	int first_time = 0;
	
	memcpy (j->nick_remoto, nick, sizeof (j->nick_remoto));
	
	surface = window_get_surface (j->ventana);
	
	/* Renderizar el nick del otro jugador */
	blanco.r = blanco.g = blanco.b = 255;
	negro.r = negro.g = negro.b = 0;
	if (j->nick_remoto_image != NULL) {
		SDL_FreeSurface (j->nick_remoto_image);
	} else {
		first_time = 1;
	}
	j->nick_remoto_image = draw_text_with_shadow (ttf16_comiccrazy, 2, j->nick_remoto, blanco, negro);
	
	blanco.r = 0xD5; blanco.g = 0xF1; blanco.b = 0xFF;
	negro.r = 0x33; negro.g = 0x66; negro.b = 0x99;
	if (j->nick_remoto_image_blue != NULL) {
		SDL_FreeSurface (j->nick_remoto_image_blue);
	}
	j->nick_remoto_image_blue = draw_text_with_shadow (ttf16_comiccrazy, 2, j->nick_remoto, blanco, negro);
	
	/* Quitar el timer solo si es la primera vez que recibo el nick */
	if (first_time == 1) {
		window_disable_timer (j->ventana);
	}
	
	juego_draw_board (j);
}

void juego_start (Juego *j) {
	int color;
	int g, h;
	MancalaStone *stone, *s_next;
	float loc8;
	char buffer[64];
	
	j->mapa[0] = j->mapa[1] = j->mapa[2] = j->mapa[3] = j->mapa[4] = j->mapa[5] = 4;
	j->mapa[7] = j->mapa[8] = j->mapa[9] = j->mapa[10] = j->mapa[11] = j->mapa[12] = 4;
	j->mapa[6] = j->mapa[13] = 0;
	
	j->score = j->score_other = 24;
	
	color = 0;
	
	for (g = 0; g < 14; g++) {
		j->tablero[g] = NULL;
		
		for (h = 0; h < j->mapa[g]; h++) {
			stone = (MancalaStone *) malloc (sizeof (MancalaStone));
			stone->next = NULL;

			s_next = j->tablero[g];

			if (s_next == NULL) {
				j->tablero[g] = stone;
			} else {
				while (s_next->next != NULL) {
					s_next = s_next->next;
				}

				s_next->next = stone;
			}

			stone->color = color;
			stone->frame = 0;
			color++;
			if (color >= 5) {
				color = 0;
			}

			loc8 = 6.283185 * rand() / (RAND_MAX + 1.0);
			if (g == 6 || g == 13) {
				stone->x = ((float) tazas_pos[g][0]) + sin (loc8) * (10.0 * rand() / (RAND_MAX + 1.0));
				stone->y = ((float) tazas_pos[g][1]) + cos (loc8) * (25.0 * rand() / (RAND_MAX + 1.0));
			} else {
				stone->x = ((float) tazas_pos[g][0]) + sin (loc8) * (7.5 * rand() / (RAND_MAX + 1.0));
				stone->y = ((float) tazas_pos[g][1]) + cos (loc8) * (7.5 * rand() / (RAND_MAX + 1.0));
			}
		}
	}
	
	juego_draw_board (j);
}

int juego_simulate (Juego *j, int cup, int *diffs) {
	int mapa[14];
	int piezas;
	int next_cup;
	int last_cup;
	int end;
	int g, h;
	
	memcpy (mapa, j->mapa, sizeof (mapa));
	
	piezas = mapa[cup];
	mapa[cup] = 0;
	next_cup = cup;
	
	while (piezas > 0) {
		next_cup++;
		next_cup = juego_check_next_cup (j, next_cup);
		last_cup = next_cup;
		
		mapa[next_cup]++;
		
		piezas--;
	}
	
	end = 0;
	if (mapa[last_cup] == 1 && j->inicio == 0 && last_cup < 6) {
		end |= 2;
	} else if (mapa[last_cup] == 1 && j->inicio == 1 && last_cup > 6 && last_cup < 13) {
		end |= 2;
	}
	
	/* Si ocurrió una captura, reocger todas las piedras y mandarlas al mancala */
	if (end & 2) {
		g = 12 - last_cup;
		if (last_cup < 6) {
			mapa[6] += mapa[g];
			mapa[6] += mapa[last_cup];
		} else {
			mapa[13] += mapa[g];
			mapa[13] += mapa[last_cup];
		}
		mapa[g] = 0;
		mapa[last_cup] = 0;
	}
	
	/* Checar si es el final del juego */
	if (!mapa[0] && !mapa[1] && !mapa[2] && !mapa[3] && !mapa[4] && !mapa[5]) {
		end |= 4;
	} else if (!mapa[7] && !mapa[8] && !mapa[9] && !mapa[10] && !mapa[11] && !mapa[12]) {
		end |= 4;
	}
	
	if (last_cup == 6 || last_cup == 13) {
		end |= 1;
	}
	
	/* Calcular diferencias entre el mapa inicial y el resultado después del movimiento */
	h = 0;
	for (g = 0; g <= 13; g++) {
		if (j->mapa[g] != mapa[g]) {
			diffs[h] = g;
			diffs[h + 1] = mapa[g];
			
			h = h + 2;
		}
	}
	
	diffs[h] = -1;
	
	return end;
}

int compare_diffs (const void *a, const void *b) {
	const int *izq, *der;
	
	izq = (const int *) a;
	der = (const int *) b;
	
	if (*izq < *der) {
		return -1;
	} else if (*izq == *der) {
		return 0;
	}
	
	return 1;
}

void juego_invert_diffs (int *diffs) {
	int count = 0;
	int g = 0;
	
	while (diffs[g] != -1) {
		diffs[g] = (diffs[g] + 7) % 14;
		
		g = g + 2;
	}
	
	count = g / 2;
	
	qsort (diffs, count, sizeof (int) * 2, compare_diffs);
}

void pop_queue_move (Juego *j) {
	MancalaMov *mov;
	MancalaMov sim;
	MancalaStone *stone, *next_stone;
	int g;
	
	mov = j->queue_movs;
	
	j->queue_movs = mov->next;
	
	/* Verificar que sea el turno del otro para aceptar un movimiento */
	if (j->inicio == j->turno) {
		/* Error de movimiento, enviar fin de turno equivocado */
		printf ("Error de movimiento, era MI turno\n");
		free (mov);
		return;
	}
	
	/* Ejecutar la simulación con el movimiento actual, si la simulación falla, enviar fin */
	sim.effect = juego_simulate (j, mov->cup_sent, sim.cups_diffs);
	
	if (sim.effect != mov->effect) {
		/* Diferente efecto, bye */
		printf ("Error de movimiento, efecto diferente\n");
		free (mov);
		return;
	}
	
	/* Comparar las tazas */
	g = 0;
	while (mov->cups_diffs[g] != -1) {
		if (sim.cups_diffs[g] != mov->cups_diffs[g] ||
		    sim.cups_diffs[g + 1] != mov->cups_diffs[g + 1]) {
			printf ("Error de movimiento, tazas difieren: %i -> %i, esperado: %i -> %i\n", mov->cups_diffs[g], mov->cups_diffs[g + 1], sim.cups_diffs[g], sim.cups_diffs[g + 1]);
			
			free (mov);
			return;
		}
		g = g + 2;
	}
	
	/* Si llegamos hasta aquí, el movimiento es válido. Ejecutarlo */
	j->anim = ANIM_RAISE_STONE;
	
	stone = j->tablero[mov->cup_sent];
	j->mano = NULL;
	j->move_next_cup = mov->cup_sent + 1;
	while (stone != NULL) {
		next_stone = stone->next;
		
		stone->next = j->mano;
		j->mano = stone;
		
		stone->frame = 1;
		stone = next_stone;
	}

	j->mapa[mov->cup_sent] = 0;
	j->tablero[mov->cup_sent] = NULL;

	/* Activar el timer para levantar las piedras */
	window_register_timer_events (j->ventana, juego_move_stones);
	window_enable_timer (j->ventana);
	
	free (mov);
}

void recibir_movimiento (Juego *j, int cup, int mov, int effect, int *cups_diffs) {
	MancalaMov *new, *p;
	
	/* Revisar número de movimiento */
	if (j->mov != mov) {
		if (mov == j->mov - 1) {
			/* Buscar un ganador, si lo hay, enviar el ACK_GAME
			if (j->win != 0) {
				enviar_mov_ack_finish (j, GAME_FINISH_LOST);
				j->estado = NET_CLOSED;
			} else if (j->turno == 42) {
				enviar_mov_ack_finish (j, GAME_FINISH_TIE);
				j->estado = NET_CLOSED;
			} else {*/
				enviar_mov_ack (j);
			//}
		} else {
			fprintf (stderr, "Wrong turn number\n");
			/*j->last_fin = NET_DISCONNECT_WRONG_TURN;
			goto fin_and_close;*/
			return;
		}
	}
	
	/* Validar la taza enviada sea del oponente */
	if (j->inicio == 0) {
		/* Del 0 al 5 son mis tazas */
		if (cup < 6) {
			fprintf (stderr, "Wrong cup received\n");
			/* Enviar fin */
			return;
		}
	} else if (j->inicio == 1) {
		/* Del 7 al 12 son mis tazas */
		if (cup >= 7 && cup <= 12) {
			fprintf (stderr, "Wrong cup received\n");
			/* Enviar fin */
			return;
		}
	}
	
	j->mov = mov + 1;
	
	/* De forma inmediata, enviar la confirmación del movimiento */
	enviar_mov_ack (j);
	
	/* Colocar el movimiento en la lista de movimientos pendientes */
	new = (MancalaMov *) malloc (sizeof (MancalaMov));
	
	new->cup_sent = cup;
	new->mov = mov;
	new->effect = effect;
	memcpy (new->cups_diffs, cups_diffs, sizeof (new->cups_diffs));
	if (j->inicio == 1) {
		/* Invertir la tazas cambiadas */
		new->cup_sent = (new->cup_sent + 7) % 14;
		juego_invert_diffs (new->cups_diffs);
	}
	
	new->next = NULL;
	if (j->queue_movs == NULL) {
		j->queue_movs = new;
	} else {
		p = j->queue_movs;
		
		while (p->next != NULL) {
			p = p->next;
		}
		
		p->next = new;
	}
	
	/* Si no hay nada pendiente que animar, sacar y ejecutar una animación */
	if (j->anim == ANIM_NONE) {
		pop_queue_move (j);
	}
}

