#include "physics.h"
#include <stdlib.h>

// Shape frame functions
void shape_frame_init(ShapeFrame *frame, GPoint *positions, int point_count) {
    frame->point_count = point_count;
    frame->frame_points = (PointMass *)malloc(sizeof(PointMass) * point_count);
    frame->original_positions = (GPoint *)malloc(sizeof(GPoint) * point_count);
    
    // Store original positions
    for (int i = 0; i < point_count; i++) {
        frame->original_positions[i] = positions[i];
    }
    
    // Calculate center of original shape
    GPoint center = GPoint(0, 0);
    for (int i = 0; i < point_count; i++) {
        center.x += positions[i].x;
        center.y += positions[i].y;
    }
    center.x /= point_count;
    center.y /= point_count;
    
    // Initialize frame at 2x scale
    frame->current_scale = 2.0f;
    for (int i = 0; i < point_count; i++) {
        // Scale from center: position = center + (original - center) * scale
        int dx = positions[i].x - center.x;
        int dy = positions[i].y - center.y;
        GPoint scaled_pos = GPoint(
            center.x + (int)(dx * frame->current_scale + 0.5f),
            center.y + (int)(dy * frame->current_scale + 0.5f)
        );
        
        point_mass_init(&frame->frame_points[i], scaled_pos, FRAME_MASS);
        // Frame points have zero velocity (they're controlled directly)
        frame->frame_points[i].velocity = GPoint(0, 0);
    }
}

void shape_frame_destroy(ShapeFrame *frame) {
    if (frame->frame_points) {
        free(frame->frame_points);
        frame->frame_points = NULL;
    }
    if (frame->original_positions) {
        free(frame->original_positions);
        frame->original_positions = NULL;
    }
    frame->point_count = 0;
    frame->current_scale = 1.0f;
}

void shape_frame_set_position(ShapeFrame *frame, GPoint position) {
    if (frame->point_count == 0) return;
    
    // Calculate center of frame
    GPoint center = GPoint(0, 0);
    for (int i = 0; i < frame->point_count; i++) {
        center.x += frame->frame_points[i].position.x;
        center.y += frame->frame_points[i].position.y;
    }
    center.x /= frame->point_count;
    center.y /= frame->point_count;
    
    // Translate all points by the offset
    GPoint offset = GPoint(position.x - center.x, position.y - center.y);
    shape_frame_translate(frame, offset);
}

void shape_frame_set_rotation(ShapeFrame *frame, float angle_radians, GPoint center) {
    // Convert radians to Pebble's fixed-point angle representation
    // TRIG_MAX_ANGLE (0x10000) = 360 degrees = 2π radians
    int32_t angle_pebble = (int32_t)((angle_radians / (2.0f * 3.14159265359f)) * TRIG_MAX_ANGLE);
    
    // Get sin and cos from lookup tables (values are scaled by TRIG_MAX_RATIO)
    int32_t sin_val = sin_lookup(angle_pebble);
    int32_t cos_val = cos_lookup(angle_pebble);
    
    for (int i = 0; i < frame->point_count; i++) {
        // Translate to origin
        int dx = frame->frame_points[i].position.x - center.x;
        int dy = frame->frame_points[i].position.y - center.y;
        
        // Rotate using fixed-point math
        // sin and cos are scaled by TRIG_MAX_RATIO, so divide by it to get actual values
        int new_x = (dx * cos_val - dy * sin_val) / TRIG_MAX_RATIO;
        int new_y = (dx * sin_val + dy * cos_val) / TRIG_MAX_RATIO;
        
        // Translate back
        frame->frame_points[i].position.x = center.x + new_x;
        frame->frame_points[i].position.y = center.y + new_y;
    }
}

void shape_frame_set_scale(ShapeFrame *frame, float scale) {
    if (frame->point_count == 0 || !frame->original_positions) return;
    
    frame->current_scale = scale;
    
    // Calculate center of original shape
    GPoint center = GPoint(0, 0);
    for (int i = 0; i < frame->point_count; i++) {
        center.x += frame->original_positions[i].x;
        center.y += frame->original_positions[i].y;
    }
    center.x /= frame->point_count;
    center.y /= frame->point_count;
    
    // Scale all points from center
    for (int i = 0; i < frame->point_count; i++) {
        int dx = frame->original_positions[i].x - center.x;
        int dy = frame->original_positions[i].y - center.y;
        frame->frame_points[i].position.x = center.x + (int)(dx * scale + 0.5f);
        frame->frame_points[i].position.y = center.y + (int)(dy * scale + 0.5f);
    }
}

void shape_frame_translate(ShapeFrame *frame, GPoint offset) {
    for (int i = 0; i < frame->point_count; i++) {
        frame->frame_points[i].position.x += offset.x;
        frame->frame_points[i].position.y += offset.y;
    }
    // Also update original positions so scaling still works correctly
    if (frame->original_positions) {
        for (int i = 0; i < frame->point_count; i++) {
            frame->original_positions[i].x += offset.x;
            frame->original_positions[i].y += offset.y;
        }
    }
}

void soft_body_init(SoftBody *body, GPoint *positions, int point_count, int mass, float damping) {
    body->point_count = point_count;
    
    // Allocate memory for body points
    body->points = (PointMass *)malloc(sizeof(PointMass) * point_count);

    // Initialize body point masses (start at same positions as frame)
    for (int i = 0; i < point_count; i++) {
        point_mass_init(&body->points[i], positions[i], mass);
    }

    // Create shape frame
    body->frame = (ShapeFrame *)malloc(sizeof(ShapeFrame));
    shape_frame_init(body->frame, positions, point_count);

    // Only frame-to-body springs (no connections between body points)
    body->spring_count = point_count;
    body->springs = (Spring *)malloc(sizeof(Spring) * body->spring_count);

    // Initialize frame-to-body springs (connect each frame point to corresponding body point)
    for (int i = 0; i < point_count; i++) {
        // Calculate random stiffness multiplier: 1.0 ± FRAME_SPRING_STIFFNESS_RANDOM_FACTOR
        // e.g., if factor is 0.3, stiffness ranges from 0.7x to 1.3x
        float random_factor = 1.0f + ((float)(rand() % 2001 - 1000) / 1000.0f) * FRAME_SPRING_STIFFNESS_RANDOM_FACTOR;
        int random_stiffness = (int)(FRAME_SPRING_STIFFNESS_DEFAULT * random_factor + 0.5f);
        
        // Rest length is 0 (frame and body start at same position)
        // Frame points are at index i, body points are at index i
        spring_init(&body->springs[i],
                   &body->frame->frame_points[i],
                   &body->points[i],
                   0,  // Rest length 0 - frame drives body to its position
                   random_stiffness,  // Randomized stiffness for shape matching
                   damping);
    }
}

void soft_body_destroy(SoftBody *body) {
    if (body->frame) {
        shape_frame_destroy(body->frame);
        free(body->frame);
        body->frame = NULL;
    }
    if (body->points) {
        free(body->points);
        body->points = NULL;
    }
    if (body->springs) {
        free(body->springs);
        body->springs = NULL;
    }
    body->point_count = 0;
    body->spring_count = 0;
}

void soft_body_apply_spring_forces(SoftBody *body) {
    for (int i = 0; i < body->spring_count; i++) {
        spring_apply_forces(&body->springs[i]);
    }
}

void soft_body_apply_damping(SoftBody *body, float damping) {
    // Damping is applied during point_mass_update, but this can be used for global damping
    for (int i = 0; i < body->point_count; i++) {
        body->points[i].velocity.x *= damping;
        body->points[i].velocity.y *= damping;
    }
}

void soft_body_update(SoftBody *body, float dt) {
    // Reset all forces on body points
    for (int i = 0; i < body->point_count; i++) {
        point_mass_reset_force(&body->points[i]);
    }
    
    // Reset forces on frame points (they shouldn't move from physics, but springs apply forces)
    if (body->frame) {
        for (int i = 0; i < body->frame->point_count; i++) {
            point_mass_reset_force(&body->frame->frame_points[i]);
        }
    }
    
    // Apply spring forces (this will apply forces to both body and frame points)
    soft_body_apply_spring_forces(body);
    
    // Update only body point masses (frame points are controlled directly, not by physics)
    for (int i = 0; i < body->point_count; i++) {
        point_mass_update(&body->points[i], DAMPING_DEFAULT, dt);
    }
    
    // Frame points don't get updated - they maintain their positions unless moved directly
}


void soft_body_draw(GContext *ctx, SoftBody *body) {
    if (body->point_count < 3) return; // Need at least 3 points for a polygon
    
    // Create GPoint array for the path
    GPoint *points = (GPoint *)malloc(sizeof(GPoint) * body->point_count);
    for (int i = 0; i < body->point_count; i++) {
        points[i] = body->points[i].position;
    }
    
    // Create path info structure
    GPathInfo path_info = {
        .num_points = body->point_count,
        .points = points
    };
    
    // Create and draw the path
    GPath *path = gpath_create(&path_info);
    
    // Draw filled polygon
    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, path);
    
    // Draw outline
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 2);
    gpath_draw_outline(ctx, path);

    // Draw point masses as white dots
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int i = 0; i < body->point_count; i++) {
        graphics_fill_circle(ctx, body->points[i].position, 3);
    }

    // Cleanup
    gpath_destroy(path);
    free(points);
}

void soft_body_draw_frame(GContext *ctx, SoftBody *body) {
    if (!body->frame || body->frame->point_count < 3) return;
    
    // Draw frame outline as 1px green line
    graphics_context_set_stroke_color(ctx, GColorGreen);
    graphics_context_set_stroke_width(ctx, 1);
    
    // Draw frame outline
    for (int i = 0; i < body->frame->point_count; i++) {
        int next = (i + 1) % body->frame->point_count;
        graphics_draw_line(ctx, 
                          body->frame->frame_points[i].position,
                          body->frame->frame_points[next].position);
    }
}


