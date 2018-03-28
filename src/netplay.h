/*
 * netplay.h
 * This file is part of Mancala
 *
 * Copyright (C) 2018 - Félix Arreola Rodríguez
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

#ifndef __NETPLAY_H__
#define __NETPLAY_H__

#include "juego.h"

#define NET_CONN_TIMER 3400
#define NET_MCAST_TIMER 12000
#define NET_MCAST_TIMEOUT 120000

enum {
	TYPE_SYN = 1,
	TYPE_RES_SYN,
	TYPE_MOV,
	TYPE_MOV_ACK,
	/*TYPE_TRN_ACK_GAME,
	TYPE_KEEP_ALIVE,
	TYPE_KEEP_ALIVE_ACK,
	
	TYPE_MCAST_ANNOUNCE = 32,
	TYPE_MCAST_FIN,
	
	TYPE_FIN = 64,
	TYPE_FIN_ACK*/
};

/* Estructura para el mensaje de red */
typedef struct {
	uint8_t version;
	uint8_t type;
	uint16_t local, remote;
	char nick[NICK_SIZE];
	union {
		struct {
			uint8_t initial;
			uint8_t type_game;
		};
		struct {
			MancalaMov movement;
			uint8_t has_nick;
		};
	};
} MCMessageNet;

/* Funciones públicas */
int findfour_netinit (int);
void findfour_netclose (void);

#ifdef __MINGW32__
SOCKET findfour_get_socket4 (void);
#else
int findfour_get_socket4 (void);
#endif

void process_netevent (void);
void conectar_con_sockaddr (Juego *juego, const char *nick, struct sockaddr *peer, socklen_t peer_socklen);
void enviar_movimiento (Juego *juego, int mov, int cup, int effect);
void enviar_mov_ack (Juego *juego);

#endif /* __NETPLAY_H__ */
