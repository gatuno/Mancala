/*
 * juego.h
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

#ifndef __JUEGO_H__
#define __JUEGO_H__

/* Para el manejo de red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif
#include <sys/types.h>

#include <SDL.h>

#include "mancala.h"
#include "ventana.h"

/* Los posibles estados en los que se encuentra la partida */
enum {
	/* Acabamos de enviar un SYN para conexión inicial, esperamos por la respuesta RES + SYN */
	NET_SYN_SENT = 0,
	
	NET_READY,
	
	/* Espero la confirmación de movimiento */
	NET_WAIT_ACK,
	
	#if 0
	/* Espero la confirmación de turno y que el otro admita que gané (o empatamos) */
	NET_WAIT_WINNER,
	
	/* Envié un fin, espero la confirmación de FIN */
	NET_WAIT_CLOSING,
	#endif
	/* Conexión cerrada, para mostrar cuando alguien gana */
	NET_CLOSED,
	
	NUM_NETSTATE
};

typedef struct _MancalaStone {
	int color;
	
	float x, y;
	
	int frame;
	struct _MancalaStone *next;
} MancalaStone;

typedef struct _MancalaMov {
	int cup_sent;
	int mov;
	int effect;
	int cups_diffs[28];
	
	struct _MancalaMov *next;
} MancalaMov;

/* Estructuras */
typedef struct _Juego {
	Ventana *ventana;
	
	int mapa[14];
	
	MancalaStone *tablero[14];
	
	/* Los scores */
	int score, score_other;
	
	/* Quién inicia el juego y quién está jugando */
	int inicio;
	int turno;
	int loading_timer;
	int mov;
	
	/* Guardar el último movimiento enviado */
	MancalaMov last;
	
	/* Cola de movimientos pendientes por animar */
	MancalaMov *queue_movs;
	
	int anim;
	int move_counter;
	int move_next_cup;
	int move_last_cup;
	
	MancalaStone *mano;
	
	int hint;
	int hint_frame;
	
	/* Estado del protocolo de red */
	int estado;
	int retry;
	
	/* La dirección del otro peer */
	struct sockaddr_storage peer;
	socklen_t peer_socklen;
	
	/* El nick del otro jugador */
	char nick_remoto[NICK_SIZE];
	SDL_Surface *nick_remoto_image, *nick_remoto_image_blue;
	
	Uint32 last_response;
	uint16_t local, remote;
	
	struct _Juego *next;
} Juego;

/* Funciones públicas */
Juego *get_game_list (void);
Juego *crear_juego (int top_window);
void eliminar_juego (Juego *);
void recibir_nick (Juego *j, const char *nick);
void juego_start (Juego *j);
int juego_simulate (Juego *j, int cup, int *diffs);
void recibir_movimiento (Juego *j, int cup, int mov, int effect, int *cups_diffs);

#endif /* __JUEGO_H__ */

