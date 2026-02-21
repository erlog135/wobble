#include "physics.h"

void point_mass_init(PointMass *pm, GPoint position, int mass) {
    pm->position = position;
    pm->velocity = (Vec2){0, 0};
    pm->force = (Vec2){0, 0};
    pm->mass = mass;
}

void point_mass_apply_force(PointMass *pm, Vec2 force) {
    pm->force.x += force.x;
    pm->force.y += force.y;
}

void point_mass_update(PointMass *pm, float damping, float dt) {
    // Apply acceleration (F = ma, so a = F/m)
    int32_t accel_x = pm->force.x / pm->mass;
    int32_t accel_y = pm->force.y / pm->mass;

    // Update velocity
    pm->velocity.x += (int32_t)(accel_x * dt);
    pm->velocity.y += (int32_t)(accel_y * dt);

    // Apply damping
    pm->velocity.x = (int32_t)(pm->velocity.x * damping);
    pm->velocity.y = (int32_t)(pm->velocity.y * damping);

    // Update position (cast to int16_t — screen coords always fit)
    pm->position.x += (int16_t)(pm->velocity.x * dt);
    pm->position.y += (int16_t)(pm->velocity.y * dt);
}

void point_mass_reset_force(PointMass *pm) {
    pm->force = (Vec2){0, 0};
}

