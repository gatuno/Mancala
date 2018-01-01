/*
 * juego.h
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

typedef struct _MancalaStone {
	int color;
	
	float x, y;
	
	struct _MancalaStone *next;
} MancalaStone;

/* Estructuras */
typedef struct _Juego {
	Ventana *ventana;
	int local;
	
	int mapa[14];
	
	MancalaStone *tablero[14];
	
	int hint;
	int hint_frame;
	
	struct _Juego *next;
} Juego;

/* Funciones públicas */
Juego *get_game_list (void);
Juego *crear_juego (int top_window);
void eliminar_juego (Juego *);
void recibir_movimiento (Juego *, int turno, int col, int fila);
void buscar_ganador (Juego *j);
void recibir_nick (Juego *, const char *);

#endif /* __JUEGO_H__ */

