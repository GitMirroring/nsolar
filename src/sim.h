/* sim.h - data structures for an n-body simulation.
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

#ifndef NSOLAR_SIM_H_
#define NSOLAR_SIM_H_

#include <stdbool.h>
#include <pthread.h>

#include <raylib.h>
#include <libguile.h>

#include "vec.h"

struct body {
    char *name;
    double mass;
    double radius;
    vec3 position;
    vec3 velocity;
    Color color;
};

struct satellite {
    char *name;
    vec3 position;
    vec3 velocity;
};

enum tracking_type {
    NONE,
    BODY,
    SATELLITE,
};

struct simulation {
    double time, target, speed;
    size_t body_count, satellite_count, tracked_object;
    struct body *bodies;
    struct satellite *satellites;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int pause_counter;
    enum tracking_type tracking_type;
    bool paused, should_exit;
};

struct simulation *sim_init();
void sim_deinit(struct simulation *sim);
void sim_copy(struct simulation *sim1, struct simulation *sim2);

void sim_guile_prep();
SCM scm_from_sim(struct simulation *sim);

void sim_pause(struct simulation *sim);
void sim_unpause(struct simulation *sim);

void sim_set_tracking_mode(struct simulation *sim, enum tracking_type type);
void sim_increment_tracked(struct simulation *sim);
void sim_decrement_tracked(struct simulation *sim);

void sim_add_body(struct simulation *sim, struct body body);
void sim_add_satellite(struct simulation *sim, struct satellite sat);

#endif /* NSOLAR_SIM_H_ */
