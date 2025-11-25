#pragma once

#include <pebble.h>

// Forward declarations
typedef struct PointMass PointMass;
typedef struct Spring Spring;
typedef struct SoftBody SoftBody;

// Point mass structure
struct PointMass {
    GPoint position;
    GPoint velocity;
    GPoint force;
    int mass;
};

// Spring structure connecting two point masses
struct Spring {
    PointMass *point1;
    PointMass *point2;
    int rest_length;
    int stiffness;
    int damping;
};

// Soft body structure containing point masses and springs
struct SoftBody {
    PointMass *points;
    int point_count;
    Spring *springs;
    int spring_count;
};

// Physics constants
#define GRAVITY_DEFAULT 900
#define DAMPING_DEFAULT 0.99f
#define SPRING_STIFFNESS_DEFAULT 1000
#define SPRING_DAMPING_DEFAULT 1

// Integer square root function (replaces math.h sqrt)
static inline int physics_sqrt(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    
    // Binary search for integer square root
    int low = 1;
    int high = n;
    int result = 0;
    
    while (low <= high) {
        int mid = (low + high) / 2;
        int square = mid * mid;
        
        if (square == n) {
            return mid;
        } else if (square < n) {
            low = mid + 1;
            result = mid;
        } else {
            high = mid - 1;
        }
    }
    
    return result;
}

// Point mass functions
void point_mass_init(PointMass *pm, GPoint position, int mass);
void point_mass_apply_force(PointMass *pm, GPoint force);
void point_mass_update(PointMass *pm, float damping, float dt);
void point_mass_reset_force(PointMass *pm);

// Spring functions
void spring_init(Spring *spring, PointMass *p1, PointMass *p2, int rest_length, int stiffness, int damping);
void spring_apply_forces(Spring *spring);

// Soft body functions
void soft_body_init(SoftBody *body, GPoint *positions, int point_count, int mass, int stiffness, int damping);
void soft_body_destroy(SoftBody *body);
void soft_body_apply_gravity(SoftBody *body, int gravity);
void soft_body_apply_spring_forces(SoftBody *body);
void soft_body_apply_damping(SoftBody *body, float damping);
void soft_body_update(SoftBody *body, int gravity, float dt);
void soft_body_draw(GContext *ctx, SoftBody *body);
bool soft_body_contains_point(SoftBody *body, GPoint point);
void soft_body_draw_with_collisions(GContext *ctx, SoftBody *body, SoftBody *other_bodies[], int other_body_count);

// Collision detection functions
void collision_check_bounds(SoftBody *body, GRect bounds);
bool collision_point_rect(GPoint point, GRect rect);
void collision_resolve_bounds(PointMass *pm, GRect bounds);

// Inter-object collision detection functions
void collision_check_all_bodies(SoftBody *bodies, int body_count);
void collision_check_body_pair(SoftBody *body1, SoftBody *body2);
bool collision_point_point(PointMass *p1, PointMass *p2, int min_distance);
void collision_resolve_point_point(PointMass *p1, PointMass *p2, int min_distance);
bool collision_point_spring(PointMass *point, Spring *spring, int min_distance);
void collision_resolve_point_spring(PointMass *point, Spring *spring, int min_distance);

