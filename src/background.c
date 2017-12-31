/*
 * background.c
 * This file is part of Find Four
 *
 * Copyright (C) 2016 - Félix Arreola Rodríguez
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
 * along with Find Four; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

/* Manejador del fondo */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <SDL_image.h>

#include "mancala.h"
#include "ventana.h"
#include "path.h"

/* Cuántos fondos diferentes hay */
enum {
	BACKGROUND_BOOKROOM_OLD,
	
	NUM_BACKGROUNDS
};

/* Fondo Logde normal */
enum {
	IMG_BOOKROOM_OLD,
	//IMG_BOOKROOM_OLD_FIRE,
	//IMG_BOOKROOM_OLD_CANDLE,
	
	NUM_BOOKROOM_OLD
};

static const char *bookroom_old_images_names [NUM_BOOKROOM_OLD] = {
	"images/bookroom.png",
	//"images/lodge_fire.png",
	//"images/lodge_candle.png",
};

static int background_current;
static int background_counters[5];

static SDL_Surface **background_images = NULL;

void setup_background (void) {
	SDL_Surface * image;
	int g;
	char buffer_file[8192];
	
	int top;
	const char **names;
	
	background_current = BACKGROUND_BOOKROOM_OLD;
	
	if (background_current == BACKGROUND_BOOKROOM_OLD) {
		top = NUM_BOOKROOM_OLD;
		names = bookroom_old_images_names;
		//background_counters[0] = background_counters[1] = 0;
		/* background_counters[0] = fire
		 * background_counters[1] = candle
		 */
	}
	
	background_images = (SDL_Surface **) malloc (sizeof (SDL_Surface *) * top);
	if (background_images == NULL) {
		fprintf (stderr,
			"Failed to reserve memory\n");
		SDL_Quit ();
		exit (1);
	}
	
	for (g = 0; g < top; g++) {
		sprintf (buffer_file, "%s%s", systemdata_path, names[g]);
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
		
		background_images[g] = image;
		/* TODO: Mostrar la carga de porcentaje */
	}
}

void draw_background (SDL_Surface *screen, SDL_Rect *update_rect) {
	SDL_Rect rect, rect2, origen, whole_rect;
	int g;
	
	if (update_rect == NULL) {
		whole_rect.x = whole_rect.y = 0;
		whole_rect.w = 760;
		whole_rect.h = 480;
		
		update_rect = &whole_rect;
	}
	
	SDL_BlitSurface (background_images[IMG_BOOKROOM_OLD], update_rect, screen, update_rect);
}

void update_background (SDL_Surface *screen) {
	
}

void background_mouse_motion (int x, int y) {
	if (background_current == BACKGROUND_BOOKROOM_OLD) {
		/* Si el movimiento del mouse no entró a alguna ventana, es para el fondo */
		/*if (x >= 253 && x < 272 && y >= 117 && y < 179 && background_counters[1] < 14) {
			background_counters[1] = 14;
		}*/
	}
}
