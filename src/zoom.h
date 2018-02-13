/*
 * zoom.h
 * This file is part of Mancala
 *
 * Copyright (C) 2016 - Félix Arreola Rodríguez
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

#ifndef __ZOOM_H__
#define __ZOOM_H__

#include <SDL.h>

SDL_Surface *rotozoomSurfaceXY(SDL_Surface * src, int dstwidth, int dstheight, int smooth);

#endif /* __ZOOM_H__ */
