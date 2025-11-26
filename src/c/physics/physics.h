#pragma once

#include <pebble.h>

// Debug drawing controls
#define DEBUG_DRAW_ELEMENTS 0  // Set to 1 to show frame box and point mass circles, 0 to hide

// Physics optimization constants
#define POINT_SLEEP_VELOCITY_THRESHOLD 4.0f  // Velocity threshold below which points go to sleep
#define POINT_SLEEP_DISTANCE_THRESHOLD 1.0f  // Distance threshold for snapping to frame when sleeping

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

    // Scale animation configuration
    float start_scale;        // Initial scale factor (e.g., 0.5f for half size)
    float target_scale;       // Target scale factor (e.g., 1.0f for normal size)
    float scale_speed;        // Scale change per frame (e.g., 0.1f)

    // Drawing optimization - cached GPath and points array
    GPath *draw_path;         // Cached GPath for drawing the body
    GPoint *draw_points;      // Cached GPoint array for the path

    // Performance optimization - active point tracking
    char *point_active;       // Array tracking which points are actively being updated
    int active_point_count;   // Number of currently active points
    char is_sleeping;         // Whether the entire softbody is sleeping (all points inactive)
    GPoint *prev_positions_1; // Array storing positions from 1 update ago (for sleep detection)
    GPoint *prev_positions_2; // Array storing positions from 2 updates ago (for sleep detection)
};

// Physics constants
#define DAMPING_DEFAULT 0.80f
#define FRAME_SPRING_STIFFNESS_DEFAULT 1200  // Base stiffness for frame-to-body springs
#define FRAME_SPRING_STIFFNESS_RANDOM_FACTOR 0.3f  // Randomization factor: ±30% variation (0.7x to 1.3x)
#define FRAME_SPRING_DAMPING_DEFAULT 0.80f  // Damping for frame-to-body springs
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
void shape_frame_init(ShapeFrame *frame, GPoint *positions, int point_count, float start_scale);
void shape_frame_destroy(ShapeFrame *frame);
void shape_frame_set_position(ShapeFrame *frame, GPoint position);
void shape_frame_set_rotation(ShapeFrame *frame, float angle_radians, GPoint center);
void shape_frame_set_scale(ShapeFrame *frame, float scale);
void shape_frame_translate(ShapeFrame *frame, GPoint offset);

// Soft body functions
void soft_body_init(SoftBody *body, GPoint *positions, int point_count, int mass, float damping, float start_scale, float target_scale, float scale_speed);
void soft_body_destroy(SoftBody *body);
void soft_body_apply_spring_forces(SoftBody *body);
void soft_body_apply_damping(SoftBody *body, float damping);
void soft_body_update(SoftBody *body, float dt);
void soft_body_wake_all_points(SoftBody *body);
void soft_body_wake(SoftBody *body);
void soft_body_update_draw_path(SoftBody *body);
void soft_body_draw(GContext *ctx, SoftBody *body);
void soft_body_draw_frame(GContext *ctx, SoftBody *body);

