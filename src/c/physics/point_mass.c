#include "physics.h"

void point_mass_init(PointMass *pm, GPoint position, int mass) {
    pm->position = position;
    pm->velocity = GPoint(0, 0);
    pm->force = GPoint(0, 0);
    pm->mass = mass;
}

void point_mass_apply_force(PointMass *pm, GPoint force) {
    pm->force.x += force.x;
    pm->force.y += force.y;
}

void point_mass_update(PointMass *pm, float damping, float dt) {
    // Apply acceleration (F = ma, so a = F/m)
    GPoint acceleration = GPoint(
        pm->force.x / pm->mass,
        pm->force.y / pm->mass
    );
    
    // Update velocity
    pm->velocity.x += acceleration.x * dt;
    pm->velocity.y += acceleration.y * dt;
    
    // Apply damping
    pm->velocity.x *= damping;
    pm->velocity.y *= damping;
    
    // Update position
    pm->position.x += (int)(pm->velocity.x * dt);
    pm->position.y += (int)(pm->velocity.y * dt);
}

void point_mass_reset_force(PointMass *pm) {
    pm->force = GPoint(0, 0);
}

