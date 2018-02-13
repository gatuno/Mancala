/*
 * mancala.c
 * This file is part of Mancala
 *
 * Copyright (C) 2013 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ----------------
 * LEGAL NOTICE
 * ----------------
 *
 * This game is NOT related to Club Penguin in any way. Also,
 * this game is not intended to infringe copyrights, the graphics and
 * sounds used are Copyright of Disney.
 *
 * The new SDL code is written by Gatuno, and is released under
 * the term of the GNU General Public License.
 */

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

/* Para el tipo de dato uint8_t */
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "path.h"

#include "ventana.h"
#include "mancala.h"
#include "background.h"
#include "juego.h"
//#include "netplay.h"
//#include "chat.h"
//#include "inputbox.h"
//#include "utf8.h"
#include "draw-text.h"
#include "resolv.h"
#include "zoom.h"

#define FPS (1000/24)

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

const char *images_names[NUM_IMAGES] = {
	"images/mancala_window.png",
	
	"images/board.png",
	
	"images/player_1_background.png",
	"images/player_1_highlight.png",
	"images/player_2_background.png",
	"images/player_2_highlight.png",
	
	"images/grey_screen.png",
	
	"images/stone1_0.png",
	"images/stone1_1.png",
	"images/stone1_2.png",
	"images/stone1_3.png",
	"images/stone1_4.png",
	"images/stone1_5.png",
	"images/stone1_6.png",
	"images/stone1_7.png",
	"images/stone2_0.png",
	"images/stone2_1.png",
	"images/stone2_2.png",
	"images/stone2_3.png",
	"images/stone2_4.png",
	"images/stone2_5.png",
	"images/stone2_6.png",
	"images/stone2_7.png",
	"images/stone3_0.png",
	"images/stone3_1.png",
	"images/stone3_2.png",
	"images/stone3_3.png",
	"images/stone3_4.png",
	"images/stone3_5.png",
	"images/stone3_6.png",
	"images/stone3_7.png",
	"images/stone4_0.png",
	"images/stone4_1.png",
	"images/stone4_2.png",
	"images/stone4_3.png",
	"images/stone4_4.png",
	"images/stone4_5.png",
	"images/stone4_6.png",
	"images/stone4_7.png",
	"images/stone5_0.png",
	"images/stone5_1.png",
	"images/stone5_2.png",
	"images/stone5_3.png",
	"images/stone5_4.png",
	"images/stone5_5.png",
	"images/stone5_6.png",
	"images/stone5_7.png",
	
	"images/flecha.png"
};

const char *sound_names[NUM_SOUNDS] = {
	//"sounds/drop.wav",
	//"sounds/win.wav"
};

/* Codigos de salida */
enum {
	GAME_NONE = 0, /* No usado */
	GAME_CONTINUE,
	GAME_QUIT
};

/* Prototipos de función */
int game_loop (void);
void setup (void);
SDL_Surface * set_video_mode(unsigned);

/* Variables globales */
SDL_Surface * screen;
SDL_Surface * images [NUM_IMAGES];
SDL_Surface * text_waiting;
SDL_Surface * nick_image = NULL;
SDL_Surface * nick_image_blue = NULL;
SDL_Surface * free_turn_text[4] = {NULL, NULL, NULL, NULL};
SDL_Surface * go_again_text[4] = {NULL, NULL, NULL, NULL};
SDL_Surface * capture_text[4] = {NULL, NULL, NULL, NULL};

int use_sound;
Mix_Chunk * sounds[NUM_SOUNDS];

//static InputBox *connect_window;

int server_port;
char nick_global[NICK_SIZE];
static int nick_default;

TTF_Font *ttf16_burbank_small;

void render_nick (void) {
	SDL_Color blanco, negro;
	
	if (nick_image != NULL) SDL_FreeSurface (nick_image);
	if (nick_image_blue != NULL) SDL_FreeSurface (nick_image_blue);
	
	negro.r = negro.g = negro.b = 0;
	blanco.r = blanco.g = blanco.b = 255;
	//nick_image = draw_text_with_shadow (ttf16_comiccrazy, 2, nick_global, blanco, negro);
	
	blanco.r = 0xD5; blanco.g = 0xF1; blanco.b = 0xff;
	negro.r = 0x33; negro.g = 0x66; negro.b = 0x99;
	//nick_image_blue = draw_text_with_shadow (ttf16_comiccrazy, 2, nick_global, blanco, negro);
}

#if 0
void change_nick (InputBox *ib, const char *texto) {
	int len, g;
	int chars;
	if (strcmp (texto, "") != 0) {
		len = strlen (texto);
		
		if (len > 15) {
			chars = 0;
			while (chars < u8_strlen ((char *)texto)) {
				if (u8_offset ((char *) texto, chars + 1) < 16) {
					chars++;
				} else {
					break;
				}
			}
			
			len = u8_offset ((char *)texto, chars);
		}
		
		/* Copiar el texto a la variable de nick */
		strncpy (nick_global, texto, len + 1);
		for (g = len; g < 16; g++) {
			nick_global[g] = 0;
		}
		
		render_nick ();
		/* Reenviar el broadcast */
		enviar_broadcast_game (nick_global);
	}
	
	/* Eliminar esta ventana de texto */
	eliminar_inputbox (ib);
}

void late_connect (const char *hostname, int port, const struct addrinfo *res, int error, const char *errstr) {
	if (res == NULL) {
		message_add (MSG_ERROR, "OK", "Error al resolver nombre '%s':\n%s", hostname, errstr);
		return;
	}
	
	Ventana *ventana;
	
	ventana = (Ventana *) crear_juego (TRUE);
		
	/* Agregar la conexión a las partidas recientes */
	conectar_con_sockaddr ((Juego *) ventana, nick_global, res->ai_addr, res->ai_addrlen);
}

void nueva_conexion (void *d, const char *texto) {
	InputBox *ib = (InputBox *) d;
	int valido, puerto;
	char *hostname;
	Ventana *ventana;
	
	if (strcmp (texto, "") == 0) {
		/* Texto vacio, ignorar */
		if (ib != NULL) {
			/* Eliminar esta ventana de texto */
			eliminar_inputbox (ib);
			connect_window = NULL;
		}
		return;
	}
	hostname = strdup (texto);
	
	valido = analizador_hostname_puerto (texto, hostname, &puerto, 3300);
	
	if (valido) {
		/* Ejecutar la resolución de nombres primero, conectar después */
		do_query (hostname, puerto, late_connect);
		
		buddy_list_recent_add (texto);
	} else {
		/* Mandar un mensaje de error */
	}
	
	free (hostname);
	
	if (ib != NULL) {
		/* Eliminar esta ventana de texto */
		eliminar_inputbox (ib);
		connect_window = NULL;
	}
}

void cancelar_conexion (InputBox *ib, const char *texto) {
	connect_window = NULL;
}

void findfour_default_keyboard_handler (SDL_KeyboardEvent *key) {
	if (key->keysym.sym == SDLK_F5) {
		if (connect_window != NULL) {
			/* Levantar la ventana de conexión al frente */
			window_raise (connect_window->ventana);
		} else {
			/* Crear un input box para conectar */
			connect_window = crear_inputbox ((InputBoxFunc) nueva_conexion, "Dirección a conectar:", "", cancelar_conexion);
		}
	} else if (key->keysym.sym == SDLK_F8) {
		show_chat ();
	}
}
#endif

int main (int argc, char *argv[]) {
	int r;

	init_resolver ();
	
	initSystemPaths (argv[0]);
	
	memset (nick_global, 0, sizeof (nick_global));
	
	setup ();
	
	/* Generar o leer el nick del archivo de configuración */
	r = RANDOM (65536);
	sprintf (nick_global, "Player%i", r);
	render_nick ();
	
	nick_default = 1;
	
	server_port = 3300;
	
	do {
		if (game_loop () == GAME_QUIT) break;
	} while (1 == 0);
	
	destroy_resolver ();
	
	SDL_Quit ();
	return EXIT_SUCCESS;
}

int game_loop (void) {
	int done = 0;
	SDL_Event event;
	SDLKey key;
	Uint32 last_time, now_time;
	SDL_Rect rect, rect2;
	
	int g, h;
	int start = 0;
	int x, y;
	
	Juego *j;
	
	//SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
	#if 0
	if (findfour_netinit (server_port) < 0) {
		fprintf (stderr, "Falló al inicializar la red\n");
		
		return GAME_QUIT;
	}
	#endif
	
	draw_background (screen, NULL);
	//inicializar_chat ();
	
	/*if (nick_default) {
		crear_inputbox ((InputBoxFunc) change_nick, "Ingrese su nombre de jugador:", nick_global, NULL);
	}*/
	
	SDL_Flip (screen);
	
	SDL_EnableUNICODE (1);
	
	crear_juego (TRUE);
	
	do {
		last_time = SDL_GetTicks ();
		
		/* Antes de procesar los eventos locales, procesar la red */
		//process_netevent ();
		
		while (SDL_PollEvent(&event) > 0) {
			if (event.type == SDL_QUIT) {
				/* Vamos a cerrar la aplicación */
				done = GAME_QUIT;
				break;
			} else if (event.type == SDL_MOUSEBUTTONDOWN ||
			           event.type == SDL_MOUSEMOTION ||
			           event.type == SDL_KEYDOWN ||
			           event.type == SDL_MOUSEBUTTONUP) {
				window_manager_event (event);
			}
		}
		
		window_manager_timer ();
		
		//printf ("Dibujar: \n");
		window_manager_draw (screen);
		
		/* Dibujar los mensajes en pantalla
		if (list_msg != NULL) {
			drawfuzz (0, 0, 760, 480);
			message_display (screen);
		}*/
		
		now_time = SDL_GetTicks ();
		if (now_time < last_time + FPS) SDL_Delay(last_time + FPS - now_time);
		
	} while (!done);
	SDL_EnableUNICODE (0);
	
	#if 0
	/* Recorrer los juegos */
	j = get_game_list ();
	while (j != NULL) {
		if (j->estado != NET_CLOSED && j->estado != NET_WAIT_CLOSING) {
			j->last_fin = NET_USER_QUIT;
			j->retry = 0;
			enviar_fin (j);
		}
		
		j = j->next;
	}
	
	findfour_netclose ();
	#endif
	return done;
}

/* Set video mode: */
/* Mattias Engdegard <f91-men@nada.kth.se> */
SDL_Surface * set_video_mode (unsigned flags) {
	/* Prefer 16bpp, but also prefer native modes to emulated 16bpp. */
	int depth;

	depth = SDL_VideoModeOK (760, 480, 16, flags);
	return depth ? SDL_SetVideoMode (760, 480, depth, flags) : NULL;
}

void setup (void) {
	SDL_Surface * image;
	TTF_Font *font;
	SDL_Color blanco, negro;
	int g;
	char buffer_file[8192];
	float sw, sh;
	int resw, resh;
	
	/* Inicializar el Video SDL */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf (stderr,
			"Error: Can't initialize the video subsystem\n"
			"The error returned by SDL is:\n"
			"%s\n", SDL_GetError());
		exit (1);
	}
	
	sprintf (buffer_file, "%simages/icon.png", systemdata_path);
	image = IMG_Load (buffer_file);
	if (image) {
		SDL_WM_SetIcon (image, NULL);
		SDL_FreeSurface (image);
	}
	SDL_WM_SetCaption ("Mancala", "Mancala");
	
	/* Crear la pantalla de dibujado */
	screen = set_video_mode (0);
	
	if (screen == NULL) {
		fprintf (stderr,
			"Error: Can't setup 760x480 video mode.\n"
			"The error returned by SDL is:\n"
			"%s\n", SDL_GetError());
		exit (1);
	}
	
	use_sound = 1;
	if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0) {
		fprintf (stdout,
			"Warning: Can't initialize the audio subsystem\n"
			"Continuing...\n");
		use_sound = 0;
	}
	
	if (use_sound) {
		/* Inicializar el sonido */
		if (Mix_OpenAudio (22050, AUDIO_S16, 2, 4096) < 0) {
			fprintf (stdout,
				"Warning: Can't initialize the SDL Mixer library\n");
			use_sound = 0;
		} else {
			Mix_AllocateChannels (3);
		}
	}
	
	for (g = 0; g < NUM_IMAGES; g++) {
		sprintf (buffer_file, "%s%s", systemdata_path, images_names[g]);
		image = IMG_Load (buffer_file);
		
		if (image == NULL) {
			fprintf (stderr,
				"Failed to load data file:\n"
				"%s\n"
				"The error returned by SDL is:\n"
				"%s\n", buffer_file, SDL_GetError());
			SDL_Quit ();
			exit (1);
		}
		
		images[g] = image;
		/* TODO: Mostrar la carga de porcentaje */
	}
	
	if (use_sound) {
		for (g = 0; g < NUM_SOUNDS; g++) {
			sprintf (buffer_file, "%s%s", systemdata_path, sound_names[g]);
			sounds[g] = Mix_LoadWAV (buffer_file);
			
			if (sounds[g] == NULL) {
				fprintf (stderr,
					"Failed to load data file:\n"
					"%s\n"
					"The error returned by SDL is:\n"
					"%s\n", buffer_file, SDL_GetError ());
				SDL_Quit ();
				exit (1);
			}
			Mix_VolumeChunk (sounds[g], MIX_MAX_VOLUME / 2);
		}
	}
	
	/* Cargar las tipografias */
	if (TTF_Init () < 0) {
		fprintf (stderr,
			"Error: Can't initialize the SDL TTF library\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	sprintf (buffer_file, "%s%s", systemdata_path, "burbanksb.ttf");
	ttf16_burbank_small = TTF_OpenFont (buffer_file, 16);
	
	if (!ttf16_burbank_small) {
		fprintf (stderr,
			"Failed to load font file 'Burbank Small Bold'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	#if 0
	sprintf (buffer_file, "%s%s", systemdata_path, "ccfacefront.ttf");
	ttf14_facefront = TTF_OpenFont (buffer_file, 14);
	if (!ttf14_facefront) {
		fprintf (stderr,
			"Failed to load font file 'CC Face Front'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	sprintf (buffer_file, "%s%s", systemdata_path, "comicrazy.ttf");
	ttf16_comiccrazy = TTF_OpenFont (buffer_file, 16);
	if (!ttf16_comiccrazy) {
		fprintf (stderr,
			"Failed to load font file 'Comic Crazy'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	#endif
	
	sprintf (buffer_file, "%s%s", systemdata_path, "comicrazy.ttf");
	font = TTF_OpenFont (buffer_file, 34);
	if (!font) {
		fprintf (stderr,
			"Failed to load font file 'Comic Crazy'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	/* Dibujar el texto de "Free Turn" */
	// #FBC827
	blanco.r = 0xFB; blanco.g = 0xC8; blanco.b = 0x27;
	negro.r = 0x00; negro.g = 0x00; negro.b = 0x00;
	free_turn_text[3] = draw_text_with_shadow (font, 4, "FREE TURN", blanco, negro);
	capture_text[3] = draw_text_with_shadow (font, 4, "CAPTURE", blanco, negro);
	
	TTF_CloseFont (font);
	
	/* Generar los 3 textos escalados */
	sw = 0.5 * ((float) free_turn_text[3]->w);
	sh = 0.5 * ((float) free_turn_text[3]->h);
	
	resw = (int) (sw + 0.5);
	resh = (int) (sh + 0.5);
	
	free_turn_text[0] = rotozoomSurfaceXY (free_turn_text[3], resw, resh, 1);
	
	sw = 1.1 * ((float) free_turn_text[3]->w);
	sh = 1.2 * ((float) free_turn_text[3]->h);
	
	resw = (int) (sw + 0.5);
	resh = (int) (sh + 0.5);
	
	free_turn_text[1] = rotozoomSurfaceXY (free_turn_text[3], resw, free_turn_text[3]->h, 1);
	free_turn_text[2] = rotozoomSurfaceXY (free_turn_text[3], free_turn_text[3]->w, resh, 1);
	
	/* Generar los 3 textos escalados */
	sw = 0.5 * ((float) capture_text[3]->w);
	sh = 0.5 * ((float) capture_text[3]->h);
	
	resw = (int) (sw + 0.5);
	resh = (int) (sh + 0.5);
	
	capture_text[0] = rotozoomSurfaceXY (capture_text[3], resw, resh, 1);
	
	sw = 1.1 * ((float) capture_text[3]->w);
	sh = 1.2 * ((float) capture_text[3]->h);
	
	resw = (int) (sw + 0.5);
	resh = (int) (sh + 0.5);
	
	capture_text[1] = rotozoomSurfaceXY (capture_text[3], resw, capture_text[3]->h, 1);
	capture_text[2] = rotozoomSurfaceXY (capture_text[3], capture_text[3]->w, resh, 1);
	
	sprintf (buffer_file, "%s%s", systemdata_path, "comicrazy.ttf");
	font = TTF_OpenFont (buffer_file, 24);
	if (!font) {
		fprintf (stderr,
			"Failed to load font file 'Comic Crazy'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	/* Dibujar el texto de "Go Again!" */
	blanco.r = 0xFF; blanco.g = 0xFF; blanco.b = 0xFF;
	negro.r = 0x00; negro.g = 0x00; negro.b = 0x00;
	go_again_text[3] = draw_text_with_shadow (font, 4, "GO AGAIN!", blanco, negro);
	
	TTF_CloseFont (font);
	
	/* Generar los 3 textos escalados */
	sw = 0.5 * ((float) go_again_text[3]->w);
	sh = 0.5 * ((float) go_again_text[3]->h);
	
	resw = (int) (sw + 0.5);
	resh = (int) (sh + 0.5);
	
	go_again_text[0] = rotozoomSurfaceXY (go_again_text[3], resw, resh, 1);
	/* Generar los 3 textos escalados */
	sw = 1.1 * ((float) go_again_text[3]->w);
	sh = 1.2 * ((float) go_again_text[3]->h);
	
	resw = (int) (sw + 0.5);
	resh = (int) (sh + 0.5);
	
	go_again_text[1] = rotozoomSurfaceXY (go_again_text[3], resw, go_again_text[3]->h, 1);
	go_again_text[2] = rotozoomSurfaceXY (go_again_text[3], go_again_text[3]->w, resh, 1);
	
	setup_background ();
	srand (SDL_GetTicks ());
}

