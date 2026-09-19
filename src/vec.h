/* vec.h - inline functions for 3-dimensional vectors.
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

#ifndef NSOLAR_VEC_H_
#define NSOLAR_VEC_H_

#include <math.h>

#include <raylib.h>

#define RENDER_SCALE (6.957e5)
#define VEC3(x, y, z) ((vec3){x,y,z})

typedef struct {
    double x, y, z;
} vec3;

static inline vec3 vec3_add(vec3 a, vec3 b)
{
    return VEC3(a.x+b.x, a.y+b.y, a.z+b.z);
}

static inline vec3 vec3_sub(vec3 a, vec3 b)
{
    return VEC3(a.x-b.x, a.y-b.y, a.z-b.z);
}

static inline vec3 vec3_mul(vec3 a, double c)
{
    return VEC3(c*a.x, c*a.y, c*a.z);
}

static inline vec3 vec3_div(vec3 a, double c)
{
    return VEC3(a.x/c, a.y/c, a.z/c);
}

static inline vec3 vec3_neg(vec3 a)
{
    return vec3_sub(VEC3(0,0,0), a);
}

static inline double vec3_len(vec3 a)
{
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

static inline vec3 vec3_norm(vec3 a)
{
    return vec3_div(a, vec3_len(a));
}

/* convert to raylib's low-precision Vector3 */
static inline Vector3 vec3_cast(vec3 a)
{
    return (Vector3) {
        a.x, a.y, a.z
    };
}

/* convert to a Vector3 and scale down for rendering */
static inline Vector3 vec3_conv(vec3 a)
{
    return vec3_cast(vec3_div(a, RENDER_SCALE));
}

#endif /* NSOLAR_VEC_H_ */
