#include "physics.h"

void spring_init(Spring *spring, PointMass *p1, PointMass *p2, int rest_length, int stiffness, int damping) {
    spring->point1 = p1;
    spring->point2 = p2;
    spring->rest_length = rest_length;
    spring->stiffness = stiffness;
    spring->damping = damping;
}

void spring_apply_forces(Spring *spring) {
    PointMass *p1 = spring->point1;
    PointMass *p2 = spring->point2;
    
    // Calculate distance vector
    int dx = p2->position.x - p1->position.x;
    int dy = p2->position.y - p1->position.y;
    
    // Calculate current length
    int distance_squared = dx * dx + dy * dy;
    if (distance_squared == 0) return; // Avoid division by zero
    
    int distance = physics_sqrt(distance_squared);
    
    // Calculate displacement from rest length
    int displacement = distance - spring->rest_length;
    
    // Spring force magnitude (Hooke's law: F = -kx)
    int force_magnitude = spring->stiffness * displacement;
    
    // Normalize direction vector
    GPoint direction = GPoint(
        (dx * force_magnitude) / distance,
        (dy * force_magnitude) / distance
    );
    
    // Apply spring force
    point_mass_apply_force(p1, direction);
    point_mass_apply_force(p2, GPoint(-direction.x, -direction.y));
    
    // Apply damping based on relative velocity
    GPoint relative_velocity = GPoint(
        p2->velocity.x - p1->velocity.x,
        p2->velocity.y - p1->velocity.y
    );
    
    int damping_force = (relative_velocity.x * dx + relative_velocity.y * dy) / distance;
    damping_force *= spring->damping;
    
    GPoint damping_direction = GPoint(
        (dx * damping_force) / distance,
        (dy * damping_force) / distance
    );
    
    point_mass_apply_force(p1, damping_direction);
    point_mass_apply_force(p2, GPoint(-damping_direction.x, -damping_direction.y));
}

