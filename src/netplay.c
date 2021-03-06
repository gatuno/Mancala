/*
 * netplay.c
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

#include <string.h>
#include <stdio.h>

/* Para el manejo de red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#endif
#include <sys/types.h>

/**
 * IPV6_ADD_MEMBERSHIP/IPV6_DROP_MEMBERSHIP may not be defined on OSX
 */
#ifdef MACOSX
#	ifndef IPV6_ADD_MEMBERSHIP
#		define IPV6_ADD_MEMBERSHIP     IPV6_JOIN_GROUP
#		define IPV6_DROP_MEMBERSHIP    IPV6_LEAVE_GROUP
#	endif
#endif

/* Para los sockets no-bloqueantes */
#include <unistd.h>
#include <fcntl.h>

/* Para SDL_GetTicks */
#include <SDL.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include "gettext.h"
#define _(string)  (string)

#if HAVE_GETIFADDRS
#include <ifaddrs.h>
#include <net/if.h>
#endif

#include "mancala.h"
#include "utf8.h"
//#include "chat.h"
#include "juego.h"
#include "netplay.h"
//#include "stun.h"
//#include "message.h"
#include "resolv.h"
#include "ventana.h"

#define MULTICAST_IPV4_GROUP "224.0.0.133"
#define MULTICAST_IPV6_GROUP "FF02::224:0:0:133"

/* Nuestro socket de red */
#ifdef __MINGW32__
static SOCKET fd_socket6, fd_socket4;
#else
static int fd_socket6, fd_socket4;
#endif

/* El sockaddr para la dirección IPv4 y IPv6 multicast
struct sockaddr_in mcast_addr;
struct sockaddr_in6 mcast_addr6;

Uint32 multicast_timer;*/

void conectar_juego (Juego *juego, const char *nick);

int sockaddr_cmp (struct sockaddr *x, struct sockaddr *y) {
#define CMP(a, b) if (a != b) return a < b ? -1 : 1

	CMP(x->sa_family, y->sa_family);

	if (x->sa_family == AF_INET) {
		struct sockaddr_in *xin = (void*)x, *yin = (void*)y;
		CMP(ntohl(xin->sin_addr.s_addr), ntohl(yin->sin_addr.s_addr));
		CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));
	} else if (x->sa_family == AF_INET6) {
		struct sockaddr_in6 *xin6 = (void*)x, *yin6 = (void*)y;
		int r = memcmp (xin6->sin6_addr.s6_addr, yin6->sin6_addr.s6_addr, sizeof(xin6->sin6_addr.s6_addr));
		if (r != 0) return r;
		CMP(ntohs(xin6->sin6_port), ntohs(yin6->sin6_port));
		CMP(xin6->sin6_flowinfo, yin6->sin6_flowinfo);
		CMP(xin6->sin6_scope_id, yin6->sin6_scope_id);
	} else {
		return -1; /* Familia desconocida */
	}

#undef CMP
	return 0;
}

#ifdef __MINGW32__
int findfour_try_netinit4 (int puerto) {
	struct addrinfo hints, *resultados;
	struct ip_mreq mcast_req;
	char buff_p[10];
	unsigned char g;
	int fd;
	
	fd = socket (AF_INET, SOCK_DGRAM, 0);
	
	if (fd == INVALID_SOCKET) {
		fprintf (stderr, "Failed to create AF_INET socket\n");
		return -1;
	}
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		fprintf (stderr, "WSA Error: %i\n", WSAGetLastError());
		close (fd);
		return -1;
	}
	
	/* Asociar el socket con el puerto */
	if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) < 0) {
		/* Mostrar ventana de error */
		fprintf (stderr, "WSA Error: %i\n", WSAGetLastError());
		fprintf (stderr, "Bind error\n");
		return -1;
	}
	
	freeaddrinfo (resultados);
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	u_long flags = 1;
	ioctlsocket (fd, FIONBIO, &flags);
	
	#if 0
	/* Hacer join a los grupos multicast */
	/* Primero join al IPv4 */
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons (puerto);
	mcast_addr.sin_addr.s_addr = inet_addr (MULTICAST_IPV4_GROUP);
	
	mcast_req.imr_multiaddr.s_addr = inet_addr (MULTICAST_IPV4_GROUP);
	mcast_req.imr_interface.s_addr = INADDR_ANY;
	
	if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mcast_req, sizeof(mcast_req)) < 0) {
		fprintf (stderr, "WSA Error: %i\n", WSAGetLastError());
		fprintf (stderr, "Error executing IPv4 ADD_MEMBERSHIP Multicast\n");
	}
	
	g = 0;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_LOOP, &g, sizeof(g));
	g = 1;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_TTL, &g, sizeof(g));
	#endif
	return fd;
}

int findfour_try_netinit6 (int puerto) {
	struct addrinfo hints, *resultados;
	struct ipv6_mreq mcast_req6;
	unsigned int h;
	char buff_p[10];
	int fd;
	
	fd = socket (AF_INET6, SOCK_DGRAM, 0);
	
	if (fd == INVALID_SOCKET) {
		fprintf (stderr, "Failed to create AF_INET6 socket\n");
		return -1;
	}
	
	h = 1;
	setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&h, sizeof(h));
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET6;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		perror ("Getaddrinfo IPv6");
		close (fd);
		return -1;
	}
	
	/* Asociar el socket con el puerto */
	if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) < 0) {
		/* Mostrar ventana de error */
		fprintf (stderr, "WSA Error: %i\n", WSAGetLastError());
		fprintf (stderr, "Bind error\n");
		return -1;
	}
	
	freeaddrinfo (resultados);
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	u_long flags = 1;
	ioctlsocket (fd, FIONBIO, &flags);
	
	#if 0
	/* Intentar el join al grupo IPv6 */
	mcast_addr6.sin6_family = AF_INET6;
	mcast_addr6.sin6_port = htons (puerto);
	mcast_addr6.sin6_flowinfo = 0;
	mcast_addr6.sin6_scope_id = 0; /* Cualquier interfaz */
	
	inet_pton (AF_INET6, MULTICAST_IPV6_GROUP, &mcast_addr6.sin6_addr);
	memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
	mcast_req6.ipv6mr_interface = 0;
	
	if (setsockopt (fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&mcast_req6, sizeof(mcast_req6)) < 0) {
		fprintf (stderr, "WSA Error: %i\n", WSAGetLastError());
		fprintf (stderr, "Error executing IPv6 IPV6_ADD_MEMBERSHIP Multicast\n");
	}
	
	h = 0;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *)&h, sizeof (h));
	h = 64;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&h, sizeof (h));
	#endif
	return fd;
}

int findfour_netinit (int puerto) {
	struct addrinfo hints, *resultados;
	struct ip_mreq mcast_req;
	struct ipv6_mreq mcast_req6;
	unsigned char g;
	unsigned int h;
	char buff_p[10];
	
	WSADATA wsaData;
	int nRet = WSAStartup( MAKEWORD(2,2), &wsaData );
	if (nRet == SOCKET_ERROR ) {
		fprintf (stderr, "Failed to init Winsock library\n");
		return -1;
	}
	
	fd_socket4 = findfour_try_netinit4 (puerto);
	
	fd_socket6 = findfour_try_netinit6 (puerto);
	
	if (fd_socket4 < 0 && fd_socket6 < 0) {
		/* Mostrar la ventana de error */
		/* Cerrar el socket que se haya podido abrir */
		if (fd_socket4 >= 0) close (fd_socket4);
		if (fd_socket6 >= 0) close (fd_socket6);
		return -1;
	}
	
	/* Intentar el binding request */
	#if 0
	try_stun_binding ("stun.ekiga.net");
	
	enviar_broadcast_game (nick_global);
	multicast_timer = SDL_GetTicks ();
	#endif
	/* Ningún error */
	return 0;
}

void findfour_netclose (void) {
	/* Enviar el multicast de retiro de partida */
	//enviar_end_broadcast_game ();
	
	/* Y cerrar el socket */
	if (fd_socket4 >= 0) closesocket (fd_socket4);
	if (fd_socket6 >= 0) closesocket (fd_socket6);
	
	WSACleanup();
}

SOCKET findfour_get_socket4 (void) {
	return fd_socket4;
}
#else

int findfour_try_netinit4 (int puerto) {
	struct addrinfo hints, *resultados, *ressave;
	struct ip_mreq mcast_req;
	char buff_p[10];
	unsigned char g;
	int fd;
	
	fd = socket (AF_INET, SOCK_DGRAM, 0);
	
	if (fd < 0) {
		perror ("Socket AF_INET");
		return -1;
	}
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		perror ("Getaddrinfo IPv4");
		close (fd);
		return -1;
	}
	
	ressave = resultados;
	/* Recorrer los resultados hasta que uno haga bind */
	while (resultados != NULL) {
		if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) >= 0) {
			/* Por fin un bind */
			break;
		}
		resultados = resultados->ai_next;
	}
	
	freeaddrinfo (ressave);
	if (resultados == NULL) {
		fprintf (stderr, "Bind error\n");
		close (fd);
		return -1;
	}
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	int flags;
	flags = fcntl (fd, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	fcntl (fd, F_SETFL, flags);
	
	#if 0
	inet_pton (AF_INET, MULTICAST_IPV4_GROUP, &mcast_addr.sin_addr.s_addr);
	memset (&mcast_req, 0, sizeof (mcast_req));
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons (puerto);
	/* Hacer join a los grupos multicast */
#if HAVE_GETIFADDRS
	/* Listar todas las interfaces para hacer join en todas */
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	int rc;
	
	rc = getifaddrs (&ifap);
	
	if (rc == 0) { /* Sin error */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr != NULL &&
			    ifa->ifa_addr->sa_family == AF_INET &&
				(ifa->ifa_flags & IFF_LOOPBACK) == 0 && /* No queremos las interfaces Loopback */
				(ifa->ifa_flags & IFF_MULTICAST)) { /* Y solo las que soportan multicast */
				sa = (struct sockaddr_in *) ifa->ifa_addr;
				
				mcast_req.imr_multiaddr.s_addr = mcast_addr.sin_addr.s_addr;
				mcast_req.imr_interface.s_addr = sa->sin_addr.s_addr;
				
				if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) < 0) {
					perror ("Error executing IPv4 ADD_MEMBERSHIP Multicast");
				}
			}
		}
		freeifaddrs(ifap);
	} else {
#endif
	/* Join a la interfaz 0.0.0.0 */
	mcast_req.imr_multiaddr.s_addr = mcast_addr.sin_addr.s_addr;
	mcast_req.imr_interface.s_addr = INADDR_ANY;
	
	if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) < 0) {
		perror ("Error executing IPv4 ADD_MEMBERSHIP Multicast");
	}
#if HAVE_GETIFADDRS
	}
#endif
	
	g = 0;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_LOOP, &g, sizeof(g));
	g = 1;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_TTL, &g, sizeof(g));
	#endif
	return fd;
}

int findfour_try_netinit6 (int puerto) {
	struct addrinfo hints, *resultados, *ressave;
	struct ipv6_mreq mcast_req6;
	unsigned int h;
	char buff_p[10];
	int fd;
	
	fd = socket (AF_INET6, SOCK_DGRAM, 0);
	
	if (fd < 0) {
		perror ("Socket AF_INET6");
		return -1;
	}
	
	h = 1;
	setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&h, sizeof(h));
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET6;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		perror ("Getaddrinfo IPv6");
		close (fd);
		return -1;
	}
	
	ressave = resultados;
	/* Recorrer los resultados hasta que uno haga bind */
	while (resultados != NULL) {
		if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) >= 0) {
			/* Por fin un bind */
			break;
		}
		resultados = resultados->ai_next;
	}
	
	freeaddrinfo (ressave);
	if (resultados == NULL) {
		fprintf (stderr, "Bind error on IPv6\n");
		close (fd);
		return -1;
	}
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	fcntl (fd, F_SETFL, O_NONBLOCK);
	
	#if 0
	/* Intentar el join al grupo IPv6 */
	inet_pton (AF_INET6, MULTICAST_IPV6_GROUP, &mcast_addr6.sin6_addr);
	mcast_addr6.sin6_family = AF_INET6;
	mcast_addr6.sin6_port = htons (puerto);
	mcast_addr6.sin6_flowinfo = 0;
	mcast_addr6.sin6_scope_id = 0;
	
#if HAVE_GETIFADDRS
	/* Listar todas las interfaces para hacer join en todas */
	struct ifaddrs *ifap, *ifa;
	int rc;
	rc = getifaddrs (&ifap);
	
	if (rc == 0) { /* Sin error */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr != NULL &&
			    ifa->ifa_addr->sa_family == AF_INET6 &&
				(ifa->ifa_flags & IFF_LOOPBACK) == 0 && /* No queremos las interfaces Loopback */
				(ifa->ifa_flags & IFF_MULTICAST)) { /* Y solo las que soportan multicast */
				memset (&mcast_req6, 0, sizeof (mcast_req6));
				
				memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
				mcast_req6.ipv6mr_interface = if_nametoindex (ifa->ifa_name);
				
				if (mcast_req6.ipv6mr_interface != 0) {
					if (setsockopt (fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mcast_req6, sizeof(mcast_req6)) < 0) {
						perror ("Error executing IPv6 IPV6_ADD_MEMBERSHIP Multicast");
					}
				}
			}
		}
		freeifaddrs(ifap);
	} else {
#endif
	memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
	mcast_req6.ipv6mr_interface = 0; /* Cualquier interfaz */
	
	if (setsockopt (fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mcast_req6, sizeof(mcast_req6)) < 0) {
		perror ("Error executing IPv6 IPV6_ADD_MEMBERSHIP Multicast");
	}
#if HAVE_GETIFADDRS
	}
#endif
	
	h = 0;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &h, sizeof (h));
	h = 64;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &h, sizeof (h));
	#endif
	return fd;
}

int findfour_netinit (int puerto) {
	/* Crear, iniciar el socket de IPv4 */
	fd_socket4 = findfour_try_netinit4 (puerto);
	
	fd_socket6 = findfour_try_netinit6 (puerto);
	
	if (fd_socket4 < 0 && fd_socket6 < 0) {
		/* Mostrar la ventana de error */
		/* Cerrar el socket que se haya podido abrir */
		if (fd_socket4 >= 0) close (fd_socket4);
		if (fd_socket6 >= 0) close (fd_socket6);
		return -1;
	}
	
	/* Intentar el binding request */
	#if 0
	try_stun_binding ("stun.ekiga.net");
	
	enviar_broadcast_game (nick_global);
	multicast_timer = SDL_GetTicks ();
	#endif
	
	/* Ningún error */
	return 0;
}

void findfour_netclose (void) {
	/* Enviar el multicast de retiro de partida */
	//enviar_end_broadcast_game ();
	
	/* Cerrar los sockets */
	if (fd_socket4 >= 0) close (fd_socket4);
	if (fd_socket6 >= 0) close (fd_socket6);
}

int findfour_get_socket4 (void) {
	return fd_socket4;
}
#endif

void conectar_con_sockaddr (Juego *juego, const char *nick, struct sockaddr *peer, socklen_t peer_socklen) {
	memcpy (&juego->peer, peer, peer_socklen);
	juego->peer_socklen = peer_socklen;
	
	/* Sortear el turno inicial */
	juego->inicio = RANDOM(2);
	
	conectar_juego (juego, nick);
}

void conectar_juego (Juego *juego, const char *nick) {
	char buffer_send[128];
	uint16_t temp;
	
	/* Rellenar con la firma del protocolo MC */
	buffer_send[0] = 'M';
	buffer_send[1] = 'C';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 0;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_SYN;
	
	/* Número local */
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	/* Número remoto */
	buffer_send[6] = buffer_send[7] = 0;
	
	/* El nick local */
	strncpy (&buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	buffer_send[7 + NICK_SIZE] = '\0';
	
	buffer_send[8 + NICK_SIZE] = (juego->inicio == 1 ? 255 : 0);
	
	/* Ruleset */
	buffer_send[9 + NICK_SIZE] = 0;
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 10 + NICK_SIZE, 0, (struct sockaddr *) &juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié un SYN inicial. Mi local port: %i\n", juego->local);
	juego->estado = NET_SYN_SENT;
}

void enviar_res_syn (Juego *juego, const char *nick) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo MC */
	buffer_send[0] = 'M';
	buffer_send[1] = 'C';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 0;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_RES_SYN;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	/* El nick local */
	strncpy (&buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	buffer_send[7 + NICK_SIZE] = '\0';
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 8 + NICK_SIZE, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envie un RES_SYN. Estoy listo para jugar. Mi local prot: %i\n", juego->local);
	//juego->resend_nick = 0;
	//juego->wait_nick_ack = 0;
	juego->estado = NET_READY;
}

void enviar_movimiento (Juego *juego, int mov, int cup, int effect) {
	char buffer_send[128];
	uint16_t temp;
	int pos;
	int g;
	
	/* Rellenar con la firma del protocolo MC */
	buffer_send[0] = 'M';
	buffer_send[1] = 'C';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 0;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_MOV;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	buffer_send[8] = mov;
	buffer_send[9] = cup;
	buffer_send[10] = effect;
	
	pos = 12;
	g = 0;
	while (juego->last.cups_diffs[g] != -1) {
		buffer_send[pos] = juego->last.cups_diffs[g];
		buffer_send[pos + 1] = juego->last.cups_diffs[g + 1];
		g = g + 2;
		pos = pos + 2;
	}
	
	buffer_send[11] = (g / 2);
	
	buffer_send[pos] = 0; /* Envio de nick nuevo */
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, pos + 1, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié un movimiento.\n");
	
	juego->estado = NET_WAIT_ACK;
}

void enviar_mov_ack (Juego *juego) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo MC */
	buffer_send[0] = 'M';
	buffer_send[1] = 'C';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 0;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_MOV_ACK;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	buffer_send[8] = juego->mov;
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 9, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié una confirmación de movimiento.\n");
}

int unpack (MCMessageNet *msg, char *buffer, size_t len) {
	uint16_t temp;
	int g, h, pos;
	
	/* Vaciar la estructura */
	memset (msg, 0, sizeof (MCMessageNet));
	
	if (len < 4) return -1;
	
	if (buffer[0] != 'M' || buffer[1] != 'C') {
		fprintf (stderr, "Protocol Mismatch!\n");
		
		return -1;
	}
	
	if (buffer[2] != 0) {
		fprintf (stderr, "Version mismatch. Expecting 0\n");
		
		return -1;
	}
	
	msg->type = buffer[3];
	
	if (msg->type == TYPE_SYN) {
		if (len < (10 + NICK_SIZE)) return -1; /* Oops, tamaño incorrecto */
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* No hay puerto remoto, aún */
		
		/* Copiar el nick */
		strncpy (msg->nick, &buffer[8], sizeof (char) * NICK_SIZE);
		msg->nick[NICK_SIZE - 1] = 0;
		
		/* Copiar quién empieza */
		msg->initial = buffer[8 + NICK_SIZE];
		
		msg->type_game = buffer[9 + NICK_SIZE];
		
		/* Validar el nick */
		if (!is_utf8 (msg->nick)) {
			strncpy (msg->nick, _("No name"), sizeof (char) * NICK_SIZE);
		}
	} else if (msg->type == TYPE_RES_SYN) {
		if (len < (8 + NICK_SIZE)) return -1; /* Oops, tamaño incorrecto */
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		/* Copiar el nick */
		strncpy (msg->nick, &buffer[8], sizeof (char) * NICK_SIZE);
		msg->nick[NICK_SIZE - 1] = 0;
		
		/* Validar el nick */
		if (!is_utf8 (msg->nick)) {
			strncpy (msg->nick, _("No name"), sizeof (char) * NICK_SIZE);
		}
	} else if (msg->type == TYPE_MOV) {
		if (len < 12) return -1; /* Tamaño mínimo incorrecto */
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		msg->movement.mov = buffer[8];
		msg->movement.cup_sent = buffer[9];
		msg->movement.effect = buffer[10];
		
		/* Cantidad de tazas cambiadas */
		h = buffer[11];
		
		if (len < 12 + (h * 2)) return -1;
		for (g = 0, pos = 12; g < h; g = g + 1, pos = pos + 2) {
			msg->movement.cups_diffs[g * 2] = buffer[pos];
			msg->movement.cups_diffs[g * 2 + 1] = buffer[pos + 1];
		}
		
		/* Cerrar las tazas */
		msg->movement.cups_diffs[g * 2] = -1;
		
		/* Leer si hubo un cambio de nick */
		msg->has_nick = buffer[pos];
		
		if (msg->has_nick) {
			if (len < (pos + NICK_SIZE)) return -1;
			strncpy (msg->nick, &buffer[pos], sizeof (char) * NICK_SIZE);
		
			msg->nick[NICK_SIZE - 1] = 0;
		
			/* Validar el nick */
			if (!is_utf8 (msg->nick)) {
				strncpy (msg->nick, _("No name"), sizeof (char) * NICK_SIZE);
			}
		} else {
			msg->nick[0] = 0;
		}
	} else if (msg->type == TYPE_MOV_ACK) {
		if (len < 9) return -1; /* Oops, tamaño incorrecto */
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		msg->movement.mov = buffer[8];
	} else {
		return -1;
	}
	
	return 0;
}

int do_read (void *buffer, size_t buffer_len, struct sockaddr *src_addr, socklen_t *addrlen) {
	int len;
	if (fd_socket4 >= 0) {
		/* Intentar una lectura del socket de IPv4 */
		len = recvfrom (fd_socket4, buffer, buffer_len, 0, src_addr, addrlen);
		
		if (len >= 0) return len;
	}
	
	if (fd_socket6 >= 0) {
		/* Intentar una lectura del socket de IPv6 */
		len = recvfrom (fd_socket6, buffer, buffer_len, 0, src_addr, addrlen);
		
		if (len >= 0) return len;
	}
	
	return -1;
}

void process_netevent (void) {
	char buffer [256];
	Juego *juego, *next;
	MCMessageNet message;
	struct sockaddr_storage peer;
	socklen_t peer_socklen;
	int len;
	int manejado;

#ifdef HAVE_WORKING_FORK
	pending_query ();
#endif
	
	do {
		peer_socklen = sizeof (peer);
		
		/* Intentar hacer otra lectura para procesamiento */
		len = do_read (buffer, sizeof (buffer), (struct sockaddr *) &peer, &peer_socklen);
		
		if (len < 0) break;
		
		/* Detectar si este es un paquete STUN */
		#if 0
		if (buffer[0] == 0x01) {
			parse_stun_message (buffer, len);
			continue;
		}
		#endif
		if (unpack (&message, buffer, len) < 0) {
			fprintf (stderr, "Wrong packet format\n");
			continue;
		}
		#if 0
		if (message.type == TYPE_MCAST_ANNOUNCE) {
			/* Multicast de anuncio de partida de red */
			buddy_list_mcast_add (message.nick, (struct sockaddr *) &peer, peer_socklen);
			continue;
		} else if (message.type == TYPE_MCAST_FIN) {
			/* Multicast de eliminación de partida */
			buddy_list_mcast_remove ((struct sockaddr *) &peer, peer_socklen);
			continue;
		}
		#endif
		/* Si es una conexión inicial (SYN), verificar que ésta no sea una repetición de algo que ya hayamos enviado */
		if (message.type == TYPE_SYN) {
			juego = get_game_list ();
			
			manejado = FALSE;
			while (juego != NULL) {
				if (message.local == juego->remote && sockaddr_cmp ((struct sockaddr *) &juego->peer, (struct sockaddr *) &peer) == 0) {
					/* Coincidencia por puerto remoto y dirección, es una repetición */
					juego->retry++;
					enviar_res_syn (juego, nick_global);
					manejado = TRUE;
					break;
				}
				juego = (Juego *) juego->next;
			}
			
			if (manejado) continue;
			
			/* Si no fué manejado es conexión nueva */
			//printf ("Nueva conexión entrante\n");
			if (message.type_game != 0) {
				fprintf (stderr, "Wrong game format\n");
				continue;
			}
			
			juego = crear_juego (FALSE);
			
			/* Copiar la dirección IP del peer */
			memcpy (&juego->peer, &peer, peer_socklen);
			juego->peer_socklen = peer_socklen;
			
			/* Copiar el puerto remoto */
			juego->remote = message.local;
			
			/* Copiar quién empieza */
			if (message.initial == 0) {
				juego->inicio = 1;
			} else if (message.initial == 255) {
				juego->inicio = 0;
			}
			
			juego->estado = NET_READY;
			recibir_nick (juego, message.nick);
			enviar_res_syn (juego, nick_global);
			juego_start (juego);
			
			continue;
		}
		
		/* Buscar el puerto local que coincide con el mensaje */
		juego = get_game_list ();
		while (juego != NULL) {
			if (message.remote == juego->local) {
				break;
			}
			juego = juego->next;
		}
		
		if (juego == NULL) {
			/* TODO: Ningún juego coincide con el puerto mencionado, enviar un paquete RESET */
			continue;
		}
		
		#if 0
		if (message.type == TYPE_FIN) {
			if (message.fin == NET_USER_QUIT && juego->nick_remoto[0] != 0) {
				message_add (MSG_NORMAL, _("Ok"), _("%s has closed the game"), juego->nick_remoto);
			} else {
				message_add (MSG_ERROR, _("Ok"), _("The game has been closed\nErr: %i"), message.fin);
			}
			enviar_fin_ack (juego);
			
			/* Eliminar este juego */
			eliminar_juego (juego);
		} else if (message.type == TYPE_KEEP_ALIVE) {
			/* Keep alive en cualquier momento se responde con Keep Alive ACK */
			enviar_keep_alive_ack (juego);
		} else if (message.type == TYPE_SYN_NICK) {
			/* Si se recibe un cambio de nick en cualquier momento se recibe y se actualiza */
			enviar_syn_nick_ack (juego);
			
			recibir_nick (juego, message.nick);
		} else if (message.type == TYPE_SYN_NICK_ACK) {
			juego->wait_nick_ack = 0;
		} else if ((message.type == TYPE_RES_SYN && juego->estado != NET_SYN_SENT) ||
		    (message.type == TYPE_TRN && (juego->estado != NET_READY && juego->estado != NET_WAIT_ACK)) ||
		    (message.type == TYPE_TRN_ACK && juego->estado != NET_WAIT_ACK) ||
		    (message.type == TYPE_TRN_ACK_GAME && juego->estado != NET_WAIT_WINNER) ||
		    (message.type == TYPE_FIN_ACK && juego->estado != NET_WAIT_CLOSING) ||
		    (message.type == TYPE_KEEP_ALIVE_ACK && juego->estado != NET_READY)) {
			fprintf (stderr, "Wrong packet - Out of sync\n");
		#endif
		if ((message.type == TYPE_RES_SYN && juego->estado != NET_SYN_SENT)) {
			fprintf (stderr, "Wrong packet - Out of sync\n");
		} else if (message.type == TYPE_RES_SYN) {
			/* Copiar el nick del otro jugador */
			juego->estado = NET_READY;
			recibir_nick (juego, message.nick);
			
			/* Copiar el puerto remoto */
			juego->remote = message.local;
			
			//printf ("Recibí RES SYN. Su puerto remoto es: %i\n", juego->remote);
			juego_start (juego);
		} else if (message.type == TYPE_MOV) {
			/* Si estaba en el estado WAIT_ACK, y recibo un movimiento,
			 * eso confirma el turno que estaba esperando y pasamos a recibir el movimiento */
			if (juego->estado == NET_WAIT_ACK && message.movement.mov == juego->mov) {
				printf ("Movimiento de turno cuando esperaba confirmación\n");
				juego->estado = NET_READY;
			}
			
			/* Recibir el movimiento */
			recibir_movimiento (juego, message.movement.cup_sent, message.movement.mov, message.movement.effect, message.movement.cups_diffs);
		} else if (message.type == TYPE_MOV_ACK) {
			/* Verificar que el turno confirmado sea el local */
			
			if (message.movement.mov == juego->mov) {
				juego->estado = NET_READY;
			} else {
				//printf ("FIXME: ???\n");
			}
		}
		#if 0
		} else if (message.type == TYPE_TRN_ACK_GAME) {
			/* Última confirmación de turno */
			juego->estado = NET_CLOSED;
		} else if (message.type == TYPE_FIN_ACK) {
			/* La última confirmación que necesitaba */
			/* Eliminar este objeto juego */
			eliminar_juego (juego);
		} else if (message.type == TYPE_KEEP_ALIVE_ACK) {
			juego->retry = 0;
		}
		#endif
	} while (1);
	
	//check_for_retry ();
}
