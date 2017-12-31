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
#include "cp-button.h"
#include "draw-text.h"
//#include "message.h"
#include "ventana.h"

#define ANIM_VEL 2

enum {
	JUEGO_BUTTON_CLOSE = 0,
	
	JUEGO_NUM_BUTTONS
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

static Juego *network_game_list = NULL;

Juego *get_game_list (void) {
	return network_game_list;
}

void juego_first_time_draw (Juego  *j) {
	SDL_Surface *surface;
	SDL_Rect rect;
	int g, h;
	MancalaStone *stone;
	
	surface = window_get_surface (j->ventana);
	
	SDL_FillRect (surface, NULL, 0); /* Transparencia total */
	
	SDL_SetAlpha (images[IMG_WINDOW], 0, 0);
	
	SDL_BlitSurface (images[IMG_WINDOW], NULL, surface, NULL);
	
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
	for (g = 0; g < 14; g++) {
		stone = j->tablero[g];
		
		while (stone != NULL) {
			rect.x = 14.0 + stone->x + stones_offsets[stone->color][0];
			rect.y = 30.5 + stone->y + stones_offsets[stone->color][1];
			h = IMG_STONE_1_0 + (stone->color * 8);
			rect.w = images[h]->w;
			rect.h = images[h]->h;
			
			SDL_BlitSurface (images[h], NULL, surface, &rect);
			
			stone = stone->next;
		}
	}
	window_flip (j->ventana);
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
	
	j->mapa[6] = j->mapa[13] = 10;
	
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
	int old_resalte;
	#if 0
	j = (Juego *) window_get_data (v);
	
	/* Primero, quitar el resalte */
	old_resalte = j->resalte;
	
	j->resalte = -1;
	
	/* Si es nuestro turno, hacer resalte */
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio && j->estado == NET_READY) {
		/* Está dentro del tablero */
		if (x >= 32 && x < 56 && j->tablero[0][0] == 0) {
			/* Primer fila de resalte */
			j->resalte = 0;
		} else if (x >= 56 && x < 81 && j->tablero[0][1] == 0) {
			j->resalte = 1;
		} else if (x >= 81 && x < 105 && j->tablero[0][2] == 0) {
			j->resalte = 2;
		} else if (x >= 105 && x < 129 && j->tablero[0][3] == 0) {
			j->resalte = 3;
		} else if (x >= 129 && x < 153 && j->tablero[0][4] == 0) {
			j->resalte = 4;
		} else if (x >= 153 && x < 178 && j->tablero[0][5] == 0) {
			j->resalte = 5;
		} else if (x >= 178 && x < 206 && j->tablero[0][6] == 0) {
			j->resalte = 6;
		}
	}
	
	if (old_resalte != j->resalte) {
		juego_dibujar_resalte (j);
	}
	
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
	#if 0
	Juego *j;
	j = (Juego *) window_get_data (v);
	
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio && j->estado == NET_READY && j->num_a == 0) {
		/* Está dentro del tablero */
		h = -1;
		if (x >= 32 && x < 56 && j->tablero[0][0] == 0) {
			/* Primer fila de resalte */
			h = 0;
		} else if (x >= 56 && x < 81 && j->tablero[0][1] == 0) {
			h = 1;
		} else if (x >= 81 && x < 105 && j->tablero[0][2] == 0) {
			h = 2;
		} else if (x >= 105 && x < 129 && j->tablero[0][3] == 0) {
			h = 3;
		} else if (x >= 129 && x < 153 && j->tablero[0][4] == 0) {
			h = 4;
		} else if (x >= 153 && x < 178 && j->tablero[0][5] == 0) {
			h = 5;
		} else if (x >= 178 && x < 206 && j->tablero[0][6] == 0) {
			h = 6;
		}
		
		if (h >= 0) {
			g = 5;
			while (g > 0 && j->tablero[g][h] != 0) g--;
			
			j->tablero[g][h] = (j->turno % 2) + 1;
			
			if (use_sound) Mix_PlayChannel (-1, sounds[SND_DROP], 0);
			
			/* Enviar el turno */
			j->retry = 0;
			enviar_movimiento (j, j->turno, h, g);
			j->last_col = h;
			j->last_fila = g;
			j->turno++;
			j->resalte = -1;
			
			/* Si es un movimiento ganador, cambiar el estado a WAIT_WINNER */
			buscar_ganador (j);
			
			if (j->win != 0 || j->turno == 42) {
				j->estado = NET_WAIT_WINNER;
				/*if (j->win != 0) {
					// Somos ganadores
					message_add (MSG_NORMAL, "Has ganado la partida");
				} else {
					message_add (MSG_NORMAL, "%s y tú han empatado", j->nick_remoto);
				}*/
			}
			
			/* Borrar la ficha del tablero y meterla a la cola de animación */
			j->animaciones[j->num_a].fila = g;
			j->animaciones[j->num_a].col = h;
			j->animaciones[j->num_a].frame = 0;
			j->animaciones[j->num_a].color = j->tablero[g][h];
			j->num_a++;
			
			j->tablero[g][h] = 0;
			
			/* TODO: Redibujar aquí el tablero e invocar a window_update */
			juego_dibujar_tablero (j);
			window_register_timer_events (j->ventana, juego_timer_callback_anim);
			window_enable_timer (j->ventana);
		}
	}
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

