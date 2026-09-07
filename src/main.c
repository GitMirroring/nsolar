#include <stdio.h>

#include <libguile.h>
#include <raylib.h>
#include <rlgl.h>

#include "sim.h"

#define PKG_NAME "nsolar v0.0.1"

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
    Model sphere_model = LoadModelFromMesh(GenMeshSphere(1.0f, 64, 64));

    struct simulation *sim = sim_init();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
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
