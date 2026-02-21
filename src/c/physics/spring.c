#include "physics.h"

void spring_init(Spring *spring, PointMass *p1, PointMass *p2, int rest_length, int stiffness, float damping) {
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
    int32_t dx = p2->position.x - p1->position.x;
    int32_t dy = p2->position.y - p1->position.y;
    
    // Calculate current length
    int32_t distance_squared = dx * dx + dy * dy;
    if (distance_squared == 0) return;
    
    int32_t distance = physics_sqrt(distance_squared);
    
    // Calculate displacement from rest length
    int32_t displacement = distance - spring->rest_length;
    
    // Spring force magnitude (Hooke's law: F = -kx)
    int32_t force_magnitude = spring->stiffness * displacement;
    
    // Normalize direction vector — keep as Vec2 to avoid int16_t truncation
    Vec2 direction = {
        (dx * force_magnitude) / distance,
        (dy * force_magnitude) / distance
    };
    
    // Apply spring force
    point_mass_apply_force(p1, direction);
    point_mass_apply_force(p2, (Vec2){-direction.x, -direction.y});
    
    // Apply damping based on relative velocity
    Vec2 relative_velocity = {
        p2->velocity.x - p1->velocity.x,
        p2->velocity.y - p1->velocity.y
    };
    
    float damping_force = ((float)(relative_velocity.x * dx + relative_velocity.y * dy) / distance) * spring->damping;

    Vec2 damping_direction = {
        (int32_t)(dx * damping_force / distance),
        (int32_t)(dy * damping_force / distance)
    };
    
    point_mass_apply_force(p1, damping_direction);
    point_mass_apply_force(p2, (Vec2){-damping_direction.x, -damping_direction.y});
}

