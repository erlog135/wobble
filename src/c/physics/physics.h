#pragma once

#include <pebble.h>

// Forward declarations
typedef struct PointMass PointMass;
typedef struct Spring Spring;
typedef struct ShapeFrame ShapeFrame;
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

// Shape frame structure - represents the ideal/target shape
// Frame points are PointMass objects with very high mass (effectively fixed)
// Their positions can be updated directly to move/rotate/transform the shape
struct ShapeFrame {
    PointMass *frame_points;  // Ideal shape positions (can be moved directly)
    GPoint *original_positions;  // Original shape positions (for scaling)
    int point_count;
    float current_scale;  // Current scale factor (1.0 = normal size)
};

// Soft body structure containing point masses and springs
// Springs connect: (1) body points in outline loop, (2) frame points to body points
struct SoftBody {
    PointMass *points;        // Physics points (actual body)
    int point_count;
    Spring *springs;
    int spring_count;
    ShapeFrame *frame;        // Reference shape frame that drives the body
};

// Physics constants
#define DAMPING_DEFAULT 0.95f
#define SPRING_STIFFNESS_DEFAULT 1000
#define SPRING_DAMPING_DEFAULT 0.95f
#define FRAME_SPRING_STIFFNESS_DEFAULT 200  // Stiffer springs to frame for shape matching
#define FRAME_MASS 10000  // Very high mass for frame points (effectively fixed)

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

// Shape frame functions
void shape_frame_init(ShapeFrame *frame, GPoint *positions, int point_count);
void shape_frame_destroy(ShapeFrame *frame);
void shape_frame_set_position(ShapeFrame *frame, GPoint position);
void shape_frame_set_rotation(ShapeFrame *frame, float angle_radians, GPoint center);
void shape_frame_set_scale(ShapeFrame *frame, float scale);
void shape_frame_translate(ShapeFrame *frame, GPoint offset);

// Soft body functions
void soft_body_init(SoftBody *body, GPoint *positions, int point_count, int mass, int stiffness, int damping);
void soft_body_destroy(SoftBody *body);
void soft_body_apply_spring_forces(SoftBody *body);
void soft_body_apply_damping(SoftBody *body, float damping);
void soft_body_update(SoftBody *body, float dt);
void soft_body_draw(GContext *ctx, SoftBody *body);
void soft_body_draw_frame(GContext *ctx, SoftBody *body);

