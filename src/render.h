/* render.h - functions for LoD and shader graphics for nsolar.
   Copyright (C) 2026 Segen Stoutamire

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#ifndef NSOLAR_RENDER_H_
#define NSOLAR_RENDER_H_

#include <stdbool.h>

#include <raylib.h>

#include "sim.h"

extern double elevation, azimuth, radius;
extern bool grid, fps;
extern Model sphere_model;

void render(struct simulation *sim);

#endif
