#ifndef NSOLAR_SIM_H_
#define NSOLAR_SIM_H_

#include <stdbool.h>
#include <pthread.h>

#include <raylib.h>

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
    enum tracking_type tracking_type;
    bool paused, should_exit;
};

struct simulation *sim_init();
void sim_deinit(struct simulation *sim);
void sim_copy(struct simulation *sim1, struct simulation *sim2);

void sim_pause(struct simulation *sim);
void sim_unpause(struct simulation *sim);

void sim_set_tracking_mode(struct simulation *sim, enum tracking_type type);
void sim_increment_tracked(struct simulation *sim);
void sim_decrement_tracked(struct simulation *sim);

void sim_add_body(struct simulation *sim, struct body body);
void sim_add_satellite(struct simulation *sim, struct satellite sat);

#endif /* NSOLAR_SIM_H_ */
