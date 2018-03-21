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

