/* render.c - LoD and shader graphics for nsolar.
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

#include <stdbool.h>
#include <raylib.h>

#include "sim.h"
#include "vec.h"

#define RADIANS(deg) (deg*PI/180.0)

double elevation = 0.0, azimuth = 0.0, radius = 500.0;
bool grid = true, fps = false;
Model sphere_model;

static Camera3D camera = {
    .position = { 0.0, 0.0, 500.0 },
    .target   = { 0.0, 0.0, 0.0 },
    .up       = { 0.0, 1.0, 0.0 },
    .fovy     = 90.0,
    .projection = CAMERA_PERSPECTIVE,
};

/* basic camera math to track bodies and turn elevation/azimuth into x/y/z */
static void update_camera(struct simulation *sim)
{
    Vector3 tracked_pos;

    switch (sim->tracking_type) {
    case BODY:
        tracked_pos =
            vec3_conv(sim->bodies[sim->tracked_object].position);
        break;
    case SATELLITE:
        tracked_pos =
            vec3_conv(sim->satellites[sim->tracked_object].position);
        break;
    case NONE:
        tracked_pos = (Vector3) {
            0.0, 0.0, 0.0
        };
        break;
    }

    double ground = radius*cos(RADIANS(elevation));
    camera.position.x = tracked_pos.x + ground*cos(RADIANS(azimuth));
    camera.position.z = tracked_pos.z + ground*sin(RADIANS(azimuth));
    camera.position.y = tracked_pos.y + radius*sin(RADIANS(elevation));
    camera.target = tracked_pos;
}

/* draw each body while in Mode3D for raylib */
static void draw_bodies(struct simulation *sim)
{
    for (int i = 0; i < sim->body_count; i++)
        DrawModel(sphere_model,
                  vec3_conv(sim->bodies[i].position),
                  sim->bodies[i].radius / RENDER_SCALE,
                  sim->bodies[i].color);
}

void render(struct simulation *sim)
{
    update_camera(sim);

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(camera);
    draw_bodies(sim);

    if (grid)
        DrawGrid(20.0f, 50.0f);

    EndMode3D();

    if (fps)
        DrawFPS(0, 0);

    EndDrawing();
}
