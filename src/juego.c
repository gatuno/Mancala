/*
 * juego.c
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * Find Four is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Find Four is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Find Four. If not, see <http://www.gnu.org/licenses/>.
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
	ANIM_WAIT_DROP
};

int juego_mouse_down (Ventana *v, int x, int y);
int juego_mouse_motion (Ventana *v, int x, int y);
int juego_mouse_up (Ventana *v, int x, int y);
void juego_draw_button_close (Ventana *v, int frame);
void juego_button_frame (Ventana *, int button, int frame);
void juego_button_event (Ventana *, int button);

void juego_dibujar_resalte (Juego *j);
void juego_dibujar_tablero (Juego *j);

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
	
	if (j->turno == 0) {
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
	SDL_Surface *surface;
	SDL_Rect rect;
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
	
	/* Dibujar la parte de arriba */
	rect.x = 14;
	rect.y = 30;
	rect.w = images[IMG_PLAYER_1_NORMAL]->w;
	rect.h = images[IMG_PLAYER_1_NORMAL]->h;
	
	SDL_BlitSurface (images[IMG_PLAYER_1_NORMAL], NULL, surface, &rect);
	
	rect.x = 14;
	rect.y = 132;
	rect.w = images[IMG_PLAYER_2_NORMAL]->w;
	rect.h = images[IMG_PLAYER_2_NORMAL]->h;
	
	SDL_BlitSurface (images[IMG_PLAYER_2_NORMAL], NULL, surface, &rect);
	
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
			if (stone->frame == 1) {
				i = 0;
			} else if (stone->frame == 2) {
				i = 1;
			} else if (stone->frame == 3) {
				i = 2;
			} else if (stone->frame == 4) {
				i = 3;
			} else if (stone->frame == 5) {
				i = 4;
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

void juego_first_time_draw (Juego  *j) {
	SDL_Surface *surface;
	
	surface = window_get_surface (j->ventana);
	
	SDL_FillRect (surface, NULL, 0); /* Transparencia total */
	
	SDL_SetAlpha (images[IMG_WINDOW], 0, 0);
	
	SDL_BlitSurface (images[IMG_WINDOW], NULL, surface, NULL);
	
	juego_draw_board (j);
	
	window_flip (j->ventana);
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
	
	if (j->turno == 0 && j->inicio == 0 && j->hint >= 0 && j->hint < 6) {
		rect2.y = 0;
		negro.r = negro.g = negro.b = 0;
	} else if (j->turno == 1 && j->inicio == 1 && j->hint > 6 && j->hint < 13) {
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

static int juego_move_stones (Ventana *v) {
	Juego *j;
	SDL_Surface *surface;
	MancalaStone *stone, *current_stone, **prev_stone;
	float loc8;
	int g, h;
	
	j = (Juego *) window_get_data (v);
	surface = window_get_surface (v);
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
				current_stone->next = NULL;
				
				/* Soltar la piedra en la taza */
				stone = j->tablero[j->move_next_cup];
				current_stone->frame = 6;
				j->mapa[j->move_next_cup]++;
				if (stone == NULL) {
					j->tablero[j->move_next_cup] = current_stone;
				} else {
					while (stone->next != NULL) {
						stone = stone->next;
					}
					
					stone->next = current_stone;
				}
				
				if (j->move_next_cup == 6 || j->move_next_cup == 13) {
					/* Reproducir sonido al 50 % */
				} else {
					/* Reproducir sonido al 20 % */
				}
				
				g = j->move_next_cup;
				
				/* Re-acomodar la piedra con un nuevo X,Y */
				loc8 = 6.283185 * rand() / (RAND_MAX + 1.0);
				if (g == 6 || g == 13) {
					current_stone->x = ((float) tazas_pos[g][0]) + sin (loc8) * (10.0 * rand() / (RAND_MAX + 1.0));
					current_stone->y = ((float) tazas_pos[g][1]) + cos (loc8) * (25.0 * rand() / (RAND_MAX + 1.0));
				} else {
					current_stone->x = ((float) tazas_pos[g][0]) + sin (loc8) * (7.5 * rand() / (RAND_MAX + 1.0));
					current_stone->y = ((float) tazas_pos[g][1]) + cos (loc8) * (7.5 * rand() / (RAND_MAX + 1.0));
				}
				
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
			j->anim = ANIM_NONE;
			return FALSE;
		}
	}
	
	return TRUE;
}

Juego *crear_juego (int top_window) {
	Juego *j, *lista;
	int correct;
	int color;
	int g, h;
	MancalaStone *stone, *s_next;
	float loc8;
	
	/* Crear un nuevo objeto Juego */
	j = (Juego *) malloc (sizeof (Juego));
	
	/* Crear una ventana */
	j->ventana = window_create (308, 248, top_window);
	window_set_data (j->ventana, j);
	
	window_register_mouse_events (j->ventana, juego_mouse_down, juego_mouse_motion, juego_mouse_up);
	window_register_buttons (j->ventana, JUEGO_NUM_BUTTONS, juego_button_frame, juego_button_event);
	
	/* Valores propios del juego */
	j->mapa[0] = j->mapa[1] = j->mapa[2] = j->mapa[3] = j->mapa[4] = j->mapa[5] = 4;
	j->mapa[7] = j->mapa[8] = j->mapa[9] = j->mapa[10] = j->mapa[11] = j->mapa[12] = 4;
	
	/*j->mapa[0] = 18;
	j->mapa[1] = 13;
	j->mapa[2] = 10;
	j->mapa[3] = 3;
	j->mapa[5] = 5;
	
	j->mapa[7] = 7;
	j->mapa[8] = 8;
	j->mapa[9] = 9;
	j->mapa[10] = 10;
	j->mapa[11] = 11;
	j->mapa[12] = 12;*/
	
	j->mapa[6] = j->mapa[13] = 0;
	j->hint = -1;
	j->hint_frame = 0;
	j->anim = ANIM_NONE;
	
	j->mano = NULL;
	
	j->inicio = 1;
	j->turno = 1;
	
	/* Armar el tablero */
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
	
	if (x >= 64 && x >= 102 && x < 206) {
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
		} else if (new_hint == -1) {
			//printf ("Desactivando hint\n");
			j->hint = -1;
			window_disable_timer (v);
			
			/* Redibujar el tablero para borrar los hints */
			juego_draw_board (j);
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
	
	move = -1;
	
	if (j->turno == 1 && j->inicio == 1 && y >= 100 && y < 130) {
		if (x >= 69 && x < 97) { // 99
			move = 12;
		} else if (x >= 97 && x < 125) { // 127
			move = 11;
		} else if (x >= 125 && x < 153) { //155
			/* Hint de la taza 2 */
			move = 10;
		} else if (x >= 153 && x < 183) {
			move = 9;
		} else if (x >= 183 && x < 211) {
			move = 8;
		} else if (x >= 212 && x < 239) {
			move = 7;
		}
	} else if (j->turno == 0 && j->inicio == 0 && y >= 130 && y < 159) {
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
	
	if (j->anim == ANIM_NONE && move != -1 && j->mapa[move] > 0) {
		j->anim = ANIM_RAISE_STONE;
		
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

