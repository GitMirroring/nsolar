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
#include <rlgl.h>

#include "sim.h"
#include "vec.h"

#include "../config.h"

#define RADIANS(deg) (deg*PI/180.0)

double elevation = 0.0, azimuth = 0.0, radius = 50.0;
bool grid = true, fps = false;

#define GLSL(...) "#version 100\n" #__VA_ARGS__

static const char *vertex_shader =
    GLSL(
        attribute vec3 vertexPosition;
        attribute vec3 vertexNormal;
        uniform mat4 mvp;
        uniform mat4 matNormal;
        varying vec3 fragNormal;
        void main()
{
    fragNormal = vec3(matNormal * vec4(vertexNormal, 0.0));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
});

static const char *fragment_shader =
    GLSL(
        precision mediump float;
        varying vec3 fragNormal;
        uniform vec4 colDiffuse;
        uniform vec3 lightDir;
        void main()
{
    vec3 albedo = pow(colDiffuse.rgb, vec3(2.2));
    float d = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 lit = albedo * (0.03 + (0.97) * d);
    gl_FragColor = vec4(pow(lit, vec3(1.0/2.2)), colDiffuse.a);
});

static Model sphere_model;
static Shader lit_shader, unlit_shader;
static int light_dir_loc;
static double render_scale = 6.957e5;
static vec3 render_offset;
static Camera3D camera = {
    .position = { 0.0, 0.0, 500.0 },
    .target   = { 0.0, 0.0, 0.0 },
    .up       = { 0.0, 1.0, 0.0 },
    .fovy     = 70.0,
    .projection = CAMERA_PERSPECTIVE,
};

/* basic camera math to track bodies and turn elevation/azimuth into x/y/z */
static void update_camera(struct simulation *sim)
{
    double ground = radius*cos(RADIANS(elevation));
    camera.position.x = ground*cos(RADIANS(azimuth));
    camera.position.z = ground*sin(RADIANS(azimuth));
    camera.position.y = radius*sin(RADIANS(elevation));
}

/* convert to a Vector3 and scale down for rendering */
static Vector3 vec3_conv(vec3 a)
{
    return vec3_cast(vec3_div(a, render_scale));
}

static void grid_vertex(vec3 p)
{
    Vector3 v = vec3_conv(p);
    rlVertex3f(v.x, v.y, v.z);
}

/* Reference plane relative to sim 0, 0, 0
   Adapted from raylib/src/rmodels.c */
static void reference_plane()
{
    int half = 100;
    double extent = half*2e7;

    rlBegin(RL_LINES);
    for (int i = -half; i < half; i++) {
        double s = i * 2e7;

        rlColor3f(0.75f, 0.75f, 0.75f);

        grid_vertex(vec3_sub(VEC3(s, 0.0f, -extent), render_offset));
        grid_vertex(vec3_sub(VEC3(s, 0.0f, extent), render_offset));

        grid_vertex(vec3_sub(VEC3(-extent, 0.0f, s), render_offset));
        grid_vertex(vec3_sub(VEC3(extent, 0.0f, s), render_offset));
    }
    rlEnd();

}

/* draw each body while in Mode3D for raylib */
static void draw_bodies(struct simulation *sim)
{
    render_offset = VEC3(0,0,0);

    if (sim->tracking_type == BODY) {
        render_offset = sim->bodies[sim->tracked_object].position;
        render_scale = sim->bodies[sim->tracked_object].radius;
    } else if (sim->tracking_type == SATELLITE) {
        render_offset = sim->satellites[sim->tracked_object].position;
        render_scale = 16.0;
    } else {
        render_scale = 6.957e5;
    }

    /* update clip planes */
    double far = 0.0;

    for (int i = 0; i < sim->body_count; i++) {
        double d = vec3_len(vec3_sub(sim->bodies[i].position, render_offset))
                   / render_scale;
        if (d > far)
            far = d;
    }

    rlSetClipPlanes(0.01*radius, 1.1*(far+radius));

    if (grid)
        reference_plane();

    vec3 sun = sim->bodies[0].position;

    for (int i = 0; i < sim->body_count; i++) {
        if (i == 0) {
            sphere_model.materials[0].shader = unlit_shader;
        } else {
            Vector3 l = vec3_cast(vec3_norm(vec3_sub(sun,
                sim->bodies[i].position)));

            SetShaderValue(lit_shader, light_dir_loc, &l, SHADER_UNIFORM_VEC3);
            sphere_model.materials[0].shader = lit_shader;
        }

        DrawModel(sphere_model,
                  vec3_conv(vec3_sub(sim->bodies[i].position, render_offset)),
                  sim->bodies[i].radius / render_scale,
                  sim->bodies[i].color);
    }

    sphere_model.materials[0].shader = unlit_shader;
}

void render(struct simulation *sim)
{
    update_camera(sim);

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(camera);
    draw_bodies(sim);

    EndMode3D();

    if (fps)
        DrawFPS(0, 0);

    EndDrawing();
}

void render_init()
{
    /* raylib initialization */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(800, 600, REL_NAME);
    SetTargetFPS(60);
    sphere_model = LoadModelFromMesh(GenMeshSphere(1.0f, 64, 64));

    /* shader init */
    unlit_shader = sphere_model.materials[0].shader;
    lit_shader = LoadShaderFromMemory(vertex_shader, fragment_shader);
    light_dir_loc = GetShaderLocation(lit_shader, "lightDir");
}

void render_deinit()
{
    /* graceful exit */
    sphere_model.materials[0].shader = unlit_shader;
    UnloadShader(lit_shader);

    UnloadModel(sphere_model);
    CloseWindow();
}
