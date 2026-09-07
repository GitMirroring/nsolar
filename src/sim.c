#include "sim.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static void sim_tick(struct simulation *self)
{
    (void)self;
}

/* main loop of the simulation */
static void *sim_loop(void *sim_struct)
{
    struct simulation *self = sim_struct;

    /* sim starts off locked */
    pthread_mutex_lock(&self->mutex);

    while (true) {
        pthread_mutex_lock(&self->mutex);

        sim_tick(self);

        pthread_mutex_unlock(&self->mutex);

        if (self->should_exit)
            break;
    }

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
    sim->bodies = malloc(0);
    sim->satellites = malloc(0);
    sim->paused = true;
    sim->should_exit = false;

    pthread_mutex_init(&sim->mutex, NULL);
    pthread_create(&sim->thread, NULL, &sim_loop, sim);

    return sim;
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

/* copy the state of sim1 to sim2, assume both are paused */
void sim_copy(struct simulation *sim1, struct simulation *sim2)
{
    size_t bodies_size = sim1->body_count*sizeof(struct body);
    size_t satellites_size = sim1->satellite_count*sizeof(struct satellite);

    memcpy(sim2, sim1, 3*sizeof(double) + 2*sizeof(size_t));

    sim2->bodies = realloc(sim2->bodies, bodies_size);
    sim2->satellites = realloc(sim2->satellites, satellites_size);
    memcpy(sim2->bodies, sim1->bodies, bodies_size);
    memcpy(sim2->satellites, sim1->satellites, satellites_size);
}

/* pause and unpause the sim by blocking it on a mutex */
void sim_pause(struct simulation *sim)
{
    if (sim->paused) return;

    pthread_mutex_lock(&sim->mutex);
    sim->paused = true;
}

void sim_unpause(struct simulation *sim)
{
    if (!sim->paused) return;

    sim->paused = false;
    pthread_mutex_unlock(&sim->mutex);
}
