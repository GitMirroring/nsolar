/* sim.c - functions and bindings for an n-body simulation.
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

#include "sim.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <libguile.h>

#define GRAVITY 6.6743e-20

#define nth_dbl(vec, pos) scm_to_double(scm_c_vector_ref(vec, pos))

#define scm_type_assert(type, var, index, subr) \
  SCM_ASSERT_TYPE(scm_is_##type(var), var, index, subr, "Expected " #type)

/* may not always be in <unistd.h> */
int usleep(unsigned long usec);

static SCM simulation_type, body_type, satellite_type;

SCM_DEFINE(scm_make_body, "make-body", 5, 0, 0,
           (SCM name, SCM mass, SCM radius, SCM pos, SCM vel),
           "Create a new planetary body with the provided attributes")
{
    vec3 p, v;
    struct body *b = scm_gc_malloc(sizeof(*b), "body");

    scm_type_assert(string, name, 0, "make-body");
    scm_type_assert(real, mass, 1, "make-body");
    scm_type_assert(real, radius, 2, "make-body");
    scm_type_assert(vector, pos, 3, "make-body");
    scm_type_assert(vector, vel, 4, "make-body");
    /* TODO: make it possible to pass colors to this function */

    p.x = nth_dbl(pos, 0), p.y = nth_dbl(pos, 1), p.z = nth_dbl(pos, 2);
    v.x = nth_dbl(vel, 0), v.y = nth_dbl(vel, 1), v.z = nth_dbl(vel, 2);

    b->mass = scm_to_double(mass);
    b->radius = scm_to_double(mass);
    b->position = p;
    b->velocity = v;

    /* string juggling to make it GC'd */
    char *cname = scm_to_locale_string(name);
    size_t namelen = strlen(cname);
    b->name = scm_gc_malloc_pointerless(namelen+1, "body name");
    strcpy(b->name, cname);
    free(cname);

    return scm_make_foreign_object_1(body_type, b);
}

SCM_DEFINE(scm_make_satellite, "make-satellite", 5, 0, 0,
           (SCM name, SCM pos, SCM vel),
           "Create a new satellite with the provided attributes")
{
    vec3 p, v;
    struct satellite *s = scm_gc_malloc(sizeof(*s), "satellite");

    scm_type_assert(string, name, 0, "make-body");
    scm_type_assert(vector, pos, 3, "make-body");
    scm_type_assert(vector, vel, 4, "make-body");

    p.x = nth_dbl(pos, 0), p.y = nth_dbl(pos, 1), p.z = nth_dbl(pos, 2);
    v.x = nth_dbl(vel, 0), v.y = nth_dbl(vel, 1), v.z = nth_dbl(vel, 2);

    s->position = p;
    s->velocity = v;

    char *cname = scm_to_locale_string(name);
    size_t namelen = strlen(cname);
    s->name = scm_gc_malloc_pointerless(namelen+1, "body name");
    strcpy(s->name, cname);
    free(cname);

    return scm_make_foreign_object_1(satellite_type, s);
}

/* update two bodies using equation for gravitational attraction */
static void update_bodies(struct body *a, struct body *b, double speed)
{
    double F, r;
    vec3 d, A, B;

    d = vec3_sub(b->position, a->position);
    r = vec3_len(d);
    F = (GRAVITY*a->mass*b->mass*speed)/(r*r);

    d = vec3_norm(d);
    A = vec3_mul(d, F / a->mass);
    B = vec3_mul(vec3_neg(d), F / b->mass);

    a->velocity = vec3_add(a->velocity, A);
    b->velocity = vec3_add(b->velocity, B);
}

/* update a satellite based on the gravitation attraction of a body */
static void update_sat(struct satellite *a, struct body *b, double speed)
{
    double F, r;
    vec3 d, S;

    d = vec3_sub(b->position, a->position);
    r = vec3_len(d);
    /* bit of a revelation here, gravity on an object does not
       care about the mass of the object, only the mass of the attractor */
    F = (GRAVITY*b->mass*speed)/(r*r);

    d = vec3_norm(d);
    S = vec3_mul(d, F);
    a->velocity = vec3_add(a->velocity, S);
}

static void sim_tick(struct simulation *self)
{
    double speed = self->speed;
    self->time += speed;

    /* velocity updates */
    for (int i = 0; i < self->body_count-1; i++)
        for (int j = i+1; j < self->body_count; j++)
            update_bodies(&self->bodies[i], &self->bodies[j], speed);

    for (int i = 0; i < self->satellite_count; i++)
        for (int j = 0; j < self->body_count; j++)
            update_sat(&self->satellites[i], &self->bodies[j], speed);

    /* position updates */
    for (int i = 0; i < self->body_count; i++)
        self->bodies[i].position = vec3_add(
                                       self->bodies[i].position,
                                       vec3_mul(
                                           self->bodies[i].velocity,
                                           speed));

    for (int i = 0; i < self->satellite_count; i++)
        self->satellites[i].position = vec3_add(
                                           self->satellites[i].position,
                                           vec3_mul(
                                               self->satellites[i].velocity,
                                               speed));
}

/* main loop of the simulation */
static void *sim_loop(void *sim_struct)
{
    struct simulation *self = sim_struct;

    pthread_mutex_lock(&self->mutex);
    while (true) {
        while (self->paused && !self->should_exit)
            pthread_cond_wait(&self->cond, &self->mutex);

        if (self->should_exit)
            break;

        if (self->target > self->time)
            sim_tick(self);

        pthread_mutex_unlock(&self->mutex);
        usleep(16666/(int)self->speed);
        pthread_mutex_lock(&self->mutex);
    }

    pthread_mutex_unlock(&self->mutex);
    return NULL;
}

/* initialize the struct for the simulation and create a new thread */
struct simulation *sim_init()
{
    struct simulation *sim = malloc(sizeof(struct simulation));
    sim->time = 0.0f;
    sim->target = INFINITY;
    sim->speed = 1.0f;
    sim->body_count = 0;
    sim->satellite_count = 0;
    sim->tracked_object = 0;
    sim->bodies = malloc(0);
    sim->satellites = malloc(0);
    sim->paused = true;
    sim->should_exit = false;
    sim->tracking_type = NONE;

    pthread_mutex_init(&sim->mutex, NULL);
    pthread_cond_init(&sim->cond, NULL);
    pthread_create(&sim->thread, NULL, &sim_loop, sim);

    return sim;
}

SCM scm_from_sim(struct simulation *sim)
{
    return scm_make_foreign_object_1(simulation_type, sim);
}

SCM_DEFINE(scm_sim_init, "sim-init", 0, 0, 0,
           (),
           "Initialize a new simulation.")
{
    return scm_from_sim(sim_init());
}

/* set a simulation to the initial state */
void sim_reset(struct simulation *sim)
{
    sim_pause(sim);
    sim->should_exit = true;
    sim_unpause(sim);

    pthread_join(sim->thread, NULL);

    free(sim->bodies);
    free(sim->satellites);

    sim->time = 0.0f;
    sim->target = INFINITY;
    sim->speed = 1.0f;
    sim->body_count = 0;
    sim->satellite_count = 0;
    sim->tracked_object = 0;
    sim->bodies = malloc(0);
    sim->satellites = malloc(0);
    sim->paused = true;
    sim->should_exit = false;
    sim->tracking_type = NONE;

    pthread_mutex_init(&sim->mutex, NULL);
    pthread_cond_init(&sim->cond, NULL);
    pthread_create(&sim->thread, NULL, &sim_loop, sim);
}

SCM_DEFINE(scm_sim_reset, "sim-reset", 1, 0, 0,
           (SCM sim_scm),
           "Reset a simulation to the initial state.")
{
    scm_assert_foreign_object_type(simulation_type, sim_scm);

    struct simulation *sim = scm_foreign_object_ref(sim_scm, 0);
    sim_reset(sim);

    return scm_make_foreign_object_1(simulation_type, sim);
}

/* Deinitialize sim and free all tied-up memory */
void sim_deinit(struct simulation *sim)
{
    sim_pause(sim);
    sim->should_exit = true;
    sim_unpause(sim);

    pthread_join(sim->thread, NULL);

    free(sim->bodies);
    free(sim->satellites);
    free(sim);
}

SCM_DEFINE(scm_sim_deinit, "sim-deinit", 1, 0, 0,
           (SCM sim_scm),
           "Deinitialize sim and free all tied-up memory.")
{
    scm_assert_foreign_object_type(simulation_type, sim_scm);

    struct simulation *sim = scm_foreign_object_ref(sim_scm, 0);
    sim_deinit(sim);

    return SCM_UNSPECIFIED;
}

/* copy the state of sim1 to sim2, assume both are paused */
void sim_copy(struct simulation *sim1, struct simulation *sim2)
{
    size_t satellites_size = sim1->satellite_count*sizeof(struct satellite);
    size_t bodies_size = sim1->body_count*sizeof(struct body);

    memcpy(sim2, sim1, 3*sizeof(double) + 3*sizeof(size_t));
    sim2->tracking_type = sim1->tracking_type;
    sim2->paused = sim1->paused;
    sim2->should_exit = sim1->should_exit;
    sim2->tracked_object = sim1->tracked_object;

    sim2->bodies = realloc(sim2->bodies, bodies_size);
    sim2->satellites = realloc(sim2->satellites, satellites_size);

    memcpy(sim2->bodies, sim1->bodies, bodies_size);
    memcpy(sim2->satellites, sim1->satellites, satellites_size);
}

SCM_DEFINE(scm_sim_copy, "sim-copy", 2, 0, 0,
           (SCM sim1_scm, SCM sim2_scm),
           "Copy the state of the first simulation to the second one.")
{
    scm_assert_foreign_object_type(simulation_type, sim1_scm);
    scm_assert_foreign_object_type(simulation_type, sim1_scm);

    struct simulation *sim1 = scm_foreign_object_ref(sim1_scm, 0);
    struct simulation *sim2 = scm_foreign_object_ref(sim2_scm, 0);

    sim_copy(sim1, sim2);

    return SCM_UNSPECIFIED;
}

/* pause and unpause the sim by blocking it on a mutex */
void sim_pause(struct simulation *sim)
{
    pthread_mutex_lock(&sim->mutex);
    sim->paused = true;
    pthread_mutex_unlock(&sim->mutex);
}

SCM_DEFINE(scm_sim_pause, "sim-pause", 1, 0, 0,
           (SCM sim_scm),
           "Pause the simulation.")
{
    scm_assert_foreign_object_type(simulation_type, sim_scm);

    struct simulation *sim = scm_foreign_object_ref(sim_scm, 0);

    sim_pause(sim);

    return SCM_UNSPECIFIED;
}

void sim_unpause(struct simulation *sim)
{
    pthread_mutex_lock(&sim->mutex);
    sim->paused = false;
    pthread_mutex_unlock(&sim->mutex);
    pthread_cond_signal(&sim->cond);
}

SCM_DEFINE(scm_sim_unpause, "sim-unpause", 1, 0, 0,
           (SCM sim_scm),
           "Unpause the simulation.")
{
    scm_assert_foreign_object_type(simulation_type, sim_scm);

    struct simulation *sim = scm_foreign_object_ref(sim_scm, 0);

    sim_unpause(sim);

    return SCM_UNSPECIFIED;
}

/* add bodies and satellites to the sim */
void sim_add_body(struct simulation *sim, struct body body)
{
    sim->body_count++;
    sim->bodies = realloc(sim->bodies, sim->body_count*sizeof(struct body));
    memcpy(&sim->bodies[sim->body_count-1], &body, sizeof(body));
}

SCM_DEFINE(scm_sim_add_body, "sim-add-body", 1, 0, 0,
           (SCM sim_scm, SCM body_scm),
           "Add a body to a simulation.")
{
    scm_assert_foreign_object_type(body_type, body_scm);
    scm_assert_foreign_object_type(simulation_type, sim_scm);

    struct simulation *sim = scm_foreign_object_ref(sim_scm, 0);
    struct body *body = scm_foreign_object_ref(body_scm, 0);

    sim_add_body(sim, *body);

    return SCM_UNSPECIFIED;
}

void sim_add_satellite(struct simulation *sim, struct satellite sat)
{
    sim->satellite_count++;
    sim->satellites = realloc(sim->satellites,
                              sim->satellite_count*sizeof(struct satellite));
    memcpy(&sim->satellites[sim->satellite_count-1], &sat, sizeof(sat));
}

SCM_DEFINE(scm_sim_add_satellite, "sim-add-satellite", 1, 0, 0,
           (SCM sim_scm, SCM satellite_scm),
           "Add a satellite to a simulation.")
{
    scm_assert_foreign_object_type(satellite_type, satellite_scm);
    scm_assert_foreign_object_type(simulation_type, sim_scm);

    struct simulation *sim = scm_foreign_object_ref(sim_scm, 0);
    struct satellite *satellite = scm_foreign_object_ref(satellite_scm, 0);

    sim_add_satellite(sim, *satellite);

    return SCM_UNSPECIFIED;
}

/* increment and decrement the tracked object */
void sim_increment_tracked(struct simulation *sim)
{
    if (sim->tracking_type == BODY) {
        if (sim->tracked_object < sim->body_count-1)
            sim->tracked_object++;
    } else if (sim->tracking_type == SATELLITE) {
        if (sim->tracked_object < sim->satellite_count-1)
            sim->tracked_object++;
    }
}

SCM_DEFINE(scm_sim_increment_tracked, "sim-increment-tracked", 1, 0, 0,
           (SCM sim_scm),
           "Increment the tracked object in a sim.")
{
    scm_assert_foreign_object_type(simulation_type, sim_scm);
    sim_increment_tracked(scm_foreign_object_ref(sim_scm, 0));
    return SCM_UNSPECIFIED;
}

void sim_decrement_tracked(struct simulation *sim)
{
    if (sim->tracked_object > 0)
        sim->tracked_object--;
}

SCM_DEFINE(scm_sim_decrement_tracked, "sim-decrement-tracked", 1, 0, 0,
           (SCM sim_scm),
           "Decrement the tracked object in a sim.")
{
    scm_assert_foreign_object_type(simulation_type, sim_scm);
    sim_decrement_tracked(scm_foreign_object_ref(sim_scm, 0));
    return SCM_UNSPECIFIED;
}

void sim_guile_prep()
{
    SCM name, slots;
    name = scm_from_utf8_symbol("simulation");
    slots = scm_list_1(scm_from_utf8_symbol("data"));

    simulation_type = scm_make_foreign_object_type(name, slots, NULL);

    name = scm_from_utf8_symbol("body");
    slots = scm_list_1(scm_from_utf8_symbol("data"));

    body_type = scm_make_foreign_object_type(name, slots, NULL);

    name = scm_from_utf8_symbol("satellite");
    slots = scm_list_1(scm_from_utf8_symbol("data"));

    satellite_type = scm_make_foreign_object_type(name, slots, NULL);

#ifndef SCM_MAGIC_SNARFER
#include "sim.x"
#endif
}
