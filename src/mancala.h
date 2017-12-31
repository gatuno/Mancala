/*
 * findfour.h
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FINDFOUR_H__
#define __FINDFOUR_H__

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#define NICK_SIZE 16

#define RANDOM(x) ((int) (x ## .0 * rand () / (RAND_MAX + 1.0)))

#ifndef FALSE
#	define FALSE 0
#endif

#ifndef TRUE
#	define TRUE !FALSE
#endif

/* Enumerar las imágenes */
enum {
	IMG_WINDOW,
	
	IMG_BOARD,
	
	IMG_PLAYER_1_NORMAL,
	IMG_PLAYER_1_HIGHLIGHT,
	
	IMG_PLAYER_2_NORMAL,
	IMG_PLAYER_2_HIGHLIGHT,
	
	IMG_STONE_1_0,
	IMG_STONE_1_1,
	IMG_STONE_1_2,
	IMG_STONE_1_3,
	IMG_STONE_1_4,
	IMG_STONE_1_5,
	IMG_STONE_1_6,
	IMG_STONE_1_7,
	
	IMG_STONE_2_0,
	IMG_STONE_2_1,
	IMG_STONE_2_2,
	IMG_STONE_2_3,
	IMG_STONE_2_4,
	IMG_STONE_2_5,
	IMG_STONE_2_6,
	IMG_STONE_2_7,
	
	IMG_STONE_3_0,
	IMG_STONE_3_1,
	IMG_STONE_3_2,
	IMG_STONE_3_3,
	IMG_STONE_3_4,
	IMG_STONE_3_5,
	IMG_STONE_3_6,
	IMG_STONE_3_7,
	
	IMG_STONE_4_0,
	IMG_STONE_4_1,
	IMG_STONE_4_2,
	IMG_STONE_4_3,
	IMG_STONE_4_4,
	IMG_STONE_4_5,
	IMG_STONE_4_6,
	IMG_STONE_4_7,
	
	IMG_STONE_5_0,
	IMG_STONE_5_1,
	IMG_STONE_5_2,
	IMG_STONE_5_3,
	IMG_STONE_5_4,
	IMG_STONE_5_5,
	IMG_STONE_5_6,
	IMG_STONE_5_7,
	
	NUM_IMAGES
};

enum {
	//SND_DROP,
	//SND_WIN,
	
	NUM_SOUNDS
};

extern SDL_Surface * images [NUM_IMAGES];
extern char nick_global[NICK_SIZE];
extern SDL_Surface *nick_image, *nick_image_blue;
extern SDL_Surface * text_waiting;

extern int use_sound;
extern Mix_Chunk * sounds[NUM_SOUNDS];

extern TTF_Font *ttf16_burbank_medium, *ttf14_facefront, *ttf16_comiccrazy, *ttf20_comiccrazy;
void nueva_conexion (void *ib, const char *texto);

//void findfour_default_keyboard_handler (SDL_KeyboardEvent *key);

#endif /* __FINDFOUR_H__ */

