#include <stdio.h>

#include <libguile.h>
#include <raylib.h>
#include <rlgl.h>

#include "sim.h"

#define PKG_NAME "nsolar v0.0.1"

#define RADIANS(deg) (deg*PI/180.0)
#define BODY(name, mass, radius, position, velocity, color) \
sim_add_body(sim, (struct body){name,mass,radius,position,velocity,color})

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
        azimuth += 20.0;
        break;
    case 'a':
        azimuth -= 20.0;
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

static void inner_main(void *data, int argc, char **argv)
{
    /* silence compiler warnings */
    (void)data;
    (void)argc;
    (void)argv;

    /* raylib initialization */
    puts(PKG_NAME);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(800, 600, PKG_NAME);
    SetTargetFPS(60);
    rlSetClipPlanes(0.01f, 5000.0f);
    sphere_model = LoadModelFromMesh(GenMeshSphere(1.0f, 64, 64));

    struct simulation *render_sim = sim_init(), *sim = sim_init();

    BODY("Sol",1.988e30,6.955e5,VEC3(0,0,0),VEC3(0,0,0),WHITE);
    BODY("Earth",5.972e24,6371,VEC3(0,0,1.496e8),VEC3(29.78,0,0),SKYBLUE);
    BODY("Luna",7.346e22,1736,VEC3(-3.84e5,0,1.496e8),VEC3(29.78,0,1),LIGHTGRAY);
    sim_copy(sim, render_sim);

    sim->tracking_type = BODY;

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
