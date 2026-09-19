/* main.c - main program and render loop for nsolar.
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

#include <stdio.h>
#include <unistd.h>

#include <libguile.h>
#include <raylib.h>
#include <rlgl.h>

#include "sim.h"

#define PKG_NAME "nsolar v0.0.1"

#define RADIANS(deg) (deg*PI/180.0)

static Model sphere_model;
static double elevation = 0.0, azimuth = 0.0, radius = 500.0;
static bool grid = true, fps = false;
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

/* handle keypress */
static void handle_input(struct simulation *sim)
{
    switch (GetCharPressed()) {
    case '-':
        radius *= 1.2;
        break;
    case '=':
    case '+':
        radius /= 1.2;
        break;
    case 'g':
        grid = !grid;
        break;
    case 'f':
        fps = !fps;
        break;
    case 'd':
        azimuth -= 20.0;
        if (azimuth < 0.0)
            azimuth += 360.0;
        break;
    case 'a':
        azimuth += 20.0;
        if (azimuth > 360.0)
            azimuth -= 360.0;
        break;
    case 'w':
        elevation += 10.0;
        if (elevation > 89.99)
            elevation = 89.99;
        break;
    case 's':
        elevation -= 10.0;
        if (elevation < -89.99)
            elevation = -89.99;
        break;
    case ',':
        sim_decrement_tracked(sim);
        break;
    case '.':
        sim_increment_tracked(sim);
        break;
    }
}

/* "system" scheme libraries are loaded first, and can be overridden.  */
static void load_guile()
{
    char *load_paths[4] = {
        "/usr/share/nsolar/main.scm",
        "/usr/local/share/nsolar/main.scm",
        "./scm/main.scm",
        getenv("NSOLAR_LOAD"),
    };

    for (int i = 0; i < 4; i++)
        if (access(load_paths[i], F_OK) == 0)
            scm_c_primitive_load(load_paths[i]);
}

#define NUM_DEFAULT_BODIES 12
static struct body default_bodies[] = {
    { "Sol", 1.988e30, 6.955e5, VEC3(0,0,0), VEC3(0,0,0), WHITE },
    { "Mercury", 3.3e23, 2439, VEC3(0, 0, 5.791e7), VEC3(47.36, 0, 0), LIGHTGRAY },
    { "Venus", 4.867e24, 6051, VEC3(0, 0, 1.082e8), VEC3(35.02, 0, 0), YELLOW },
    { "Earth", 5.972e24, 6371, VEC3(0, 0, 1.496e8), VEC3(29.78, 0, 0), SKYBLUE },
    { "Luna", 7.346e22, 1736, VEC3(-3.84e5, 0, 1.496e8), VEC3(29.78, 0, 1), LIGHTGRAY },
    { "Mars", 6.417e23, 3389, VEC3(0, 0, 2.279e8), VEC3(24.07, 0, 0), RED },
    { "Phobos", 1.064e16, 11.1, VEC3(-9376, 0, 2.279e8), VEC3(24.07, 0, 2.14), LIGHTGRAY },
    { "Deimos", 1.51e15,  6.27, VEC3(-23463, 0, 2.279e8), VEC3(24.07, 0, 1.35), LIGHTGRAY },
    { "Jupiter", 1.898e27, 69886, VEC3(0, 0, 7.784e8), VEC3(13.06, 0, 0), ORANGE },
    { "Saturn", 5.683e26, 58232, VEC3(0, 0, 1.433e9), VEC3(9.68, 0, 0), BEIGE },
    { "Uranus", 8.681e25, 25362, VEC3(0, 0, 2.871e9), VEC3(6.80, 0, 0), BLUE },
    { "Neptune", 1.024e26, 24633, VEC3(0, 0, 4.5e9), VEC3(5.45, 0, 0), DARKBLUE },
};

/* inner main required so that guile knows where to gc */
static void inner_main(void *data, int argc, char **argv)
{
    /* silence compiler warnings */
    (void)data;
    (void)argc;
    (void)argv;

    puts(PKG_NAME);

    /* raylib initialization */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(800, 600, PKG_NAME);
    SetTargetFPS(60);
    rlSetClipPlanes(0.01f, 5000.0f);
    sphere_model = LoadModelFromMesh(GenMeshSphere(1.0f, 64, 64));

    struct simulation *render_sim = sim_init(), *sim = sim_init();

    for (int i = 0; i < NUM_DEFAULT_BODIES; i++)
        sim_add_body(sim, default_bodies[i]);

    sim_copy(sim, render_sim);
    sim->tracking_type = BODY;

    load_guile();

    sim_unpause(sim);

    while (!WindowShouldClose()) {
        handle_input(sim);
        update_camera(render_sim);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        draw_bodies(render_sim);

        if (grid)
            DrawGrid(20.0f, 50.0f);

        EndMode3D();

        if (fps)
            DrawFPS(0, 0);

        EndDrawing();

        sim_pause(sim);
        sim_copy(sim, render_sim);
        sim_unpause(sim);
    }

    sim_deinit(sim);

    /* graceful exit */
    UnloadModel(sphere_model);
    CloseWindow();
}

int main(int argc, char **argv)
{
    scm_boot_guile(argc, argv, inner_main, 0);
    /* unreachable */
}
