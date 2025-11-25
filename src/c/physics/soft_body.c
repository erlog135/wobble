#include "physics.h"
#include <stdlib.h>

void soft_body_init(SoftBody *body, GPoint *positions, int point_count, int mass, int stiffness, int damping) {
    body->point_count = point_count;
    body->points = (PointMass *)malloc(sizeof(PointMass) * point_count);

    // Initialize point masses
    for (int i = 0; i < point_count; i++) {
        point_mass_init(&body->points[i], positions[i], mass);
    }

    // Calculate number of corner/diagonal springs to add
    int corner_spring_count = 0;
    if (point_count == 4) {
        // Square: add 2 diagonal springs
        corner_spring_count = 2;
    } else if (point_count >= 6) {
        // Hexagon and larger shapes: add cross-bracing springs
        // Connect each point to the point 2 positions away (skipping one)
        corner_spring_count = point_count;
    }

    // Total springs = outline springs + corner springs
    body->spring_count = point_count + corner_spring_count;
    body->springs = (Spring *)malloc(sizeof(Spring) * body->spring_count);

    // Initialize outline springs (connect each point to the next, forming a closed loop)
    for (int i = 0; i < point_count; i++) {
        int next = (i + 1) % point_count;

        // Calculate rest length as distance between initial positions
        int dx = positions[next].x - positions[i].x;
        int dy = positions[next].y - positions[i].y;
        int rest_length = physics_sqrt(dx * dx + dy * dy);

        spring_init(&body->springs[i],
                   &body->points[i],
                   &body->points[next],
                   rest_length,
                   stiffness,
                   damping);
    }

    // Initialize corner/diagonal springs
    int spring_index = point_count; // Start after outline springs
    if (point_count == 4) {
        // Square: add diagonals (0-2 and 1-3)
        for (int i = 0; i < 2; i++) {
            int p1 = i;
            int p2 = (i + 2) % 4;

            int dx = positions[p2].x - positions[p1].x;
            int dy = positions[p2].y - positions[p1].y;
            int rest_length = physics_sqrt(dx * dx + dy * dy);

            spring_init(&body->springs[spring_index++],
                       &body->points[p1],
                       &body->points[p2],
                       rest_length,
                       stiffness,
                       damping);
        }
    } else if (point_count >= 6) {
        // Hexagon and larger: add springs connecting each point to the point 2 positions away
        for (int i = 0; i < point_count; i++) {
            int p2 = (i + 2) % point_count;

            int dx = positions[p2].x - positions[i].x;
            int dy = positions[p2].y - positions[i].y;
            int rest_length = physics_sqrt(dx * dx + dy * dy);

            spring_init(&body->springs[spring_index++],
                       &body->points[i],
                       &body->points[p2],
                       rest_length,
                       stiffness,
                       damping);
        }
    }
}

void soft_body_destroy(SoftBody *body) {
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

void soft_body_apply_gravity(SoftBody *body, int gravity) {
    for (int i = 0; i < body->point_count; i++) {
        GPoint gravity_force = GPoint(0, gravity * body->points[i].mass);
        point_mass_apply_force(&body->points[i], gravity_force);
    }
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

void soft_body_update(SoftBody *body, int gravity, float dt) {
    // Reset all forces
    for (int i = 0; i < body->point_count; i++) {
        point_mass_reset_force(&body->points[i]);
    }
    
    // Apply gravity
    soft_body_apply_gravity(body, gravity);
    
    // Apply spring forces
    soft_body_apply_spring_forces(body);
    
    // Update all point masses
    for (int i = 0; i < body->point_count; i++) {
        point_mass_update(&body->points[i], DAMPING_DEFAULT, dt);
    }
}

// Check if a point is inside the soft body using raycasting
bool soft_body_contains_point(SoftBody *body, GPoint point) {
    if (body->point_count < 3) return false;

    // First check bounding box for quick rejection
    int min_x = body->points[0].position.x;
    int max_x = body->points[0].position.x;
    int min_y = body->points[0].position.y;
    int max_y = body->points[0].position.y;

    for (int i = 1; i < body->point_count; i++) {
        GPoint pos = body->points[i].position;
        if (pos.x < min_x) min_x = pos.x;
        if (pos.x > max_x) max_x = pos.x;
        if (pos.y < min_y) min_y = pos.y;
        if (pos.y > max_y) max_y = pos.y;
    }

    // If point is outside bounding box, it's definitely outside
    if (point.x < min_x || point.x > max_x || point.y < min_y || point.y > max_y) {
        return false;
    }

    // Raycast from point to the right (along positive x-axis)
    // Count crossings of polygon edges
    int crossings = 0;
    for (int i = 0; i < body->point_count; i++) {
        GPoint p1 = body->points[i].position;
        GPoint p2 = body->points[(i + 1) % body->point_count].position;

        // Check if ray intersects this edge
        if ((p1.y <= point.y && p2.y > point.y) || (p1.y > point.y && p2.y <= point.y)) {
            // Calculate x-coordinate of intersection
            float t = (float)(point.y - p1.y) / (p2.y - p1.y);
            float intersect_x = p1.x + t * (p2.x - p1.x);

            // If intersection is to the right of our point, count it
            if (intersect_x > point.x) {
                crossings++;
            }
        }
    }

    // Even crossings = outside, odd crossings = inside
    return (crossings % 2) == 1;
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

    // Draw internal springs as thin lines
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_context_set_stroke_width(ctx, 1);
    int outline_spring_count = body->point_count; // First N springs are outline
    for (int i = outline_spring_count; i < body->spring_count; i++) {
        Spring *spring = &body->springs[i];
        GPoint p1 = spring->point1->position;
        GPoint p2 = spring->point2->position;
        graphics_draw_line(ctx, p1, p2);
    }

    // Draw magenta bounding box
    if (body->point_count > 0) {
        // Initialize bounding box with first point
        int min_x = body->points[0].position.x;
        int max_x = body->points[0].position.x;
        int min_y = body->points[0].position.y;
        int max_y = body->points[0].position.y;

        // Find min/max coordinates by iterating through all points
        for (int i = 1; i < body->point_count; i++) {
            GPoint pos = body->points[i].position;
            if (pos.x < min_x) min_x = pos.x;
            if (pos.x > max_x) max_x = pos.x;
            if (pos.y < min_y) min_y = pos.y;
            if (pos.y > max_y) max_y = pos.y;
        }

        // Draw magenta bounding box
        graphics_context_set_stroke_color(ctx, GColorMagenta);
        graphics_context_set_stroke_width(ctx, 2);
        GRect bounding_box = GRect(min_x, min_y, max_x - min_x, max_y - min_y);
        graphics_draw_rect(ctx, bounding_box);
    }

    // Draw point masses as colored dots (red if colliding, white if not)
    // Note: This version doesn't check collisions - see soft_body_draw_with_collisions for that
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int i = 0; i < body->point_count; i++) {
        graphics_fill_circle(ctx, body->points[i].position, 3);
    }

    // Cleanup
    gpath_destroy(path);
    free(points);
}

// Draw soft body with collision visualization - points turn red if inside other bodies
void soft_body_draw_with_collisions(GContext *ctx, SoftBody *body, SoftBody *other_bodies[], int other_body_count) {
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

    // Draw internal springs as thin lines
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_context_set_stroke_width(ctx, 1);
    int outline_spring_count = body->point_count; // First N springs are outline
    for (int i = outline_spring_count; i < body->spring_count; i++) {
        Spring *spring = &body->springs[i];
        GPoint p1 = spring->point1->position;
        GPoint p2 = spring->point2->position;
        graphics_draw_line(ctx, p1, p2);
    }

    // Draw magenta bounding box
    if (body->point_count > 0) {
        // Initialize bounding box with first point
        int min_x = body->points[0].position.x;
        int max_x = body->points[0].position.x;
        int min_y = body->points[0].position.y;
        int max_y = body->points[0].position.y;

        // Find min/max coordinates by iterating through all points
        for (int i = 1; i < body->point_count; i++) {
            GPoint pos = body->points[i].position;
            if (pos.x < min_x) min_x = pos.x;
            if (pos.x > max_x) max_x = pos.x;
            if (pos.y < min_y) min_y = pos.y;
            if (pos.y > max_y) max_y = pos.y;
        }

        // Draw magenta bounding box
        graphics_context_set_stroke_color(ctx, GColorMagenta);
        graphics_context_set_stroke_width(ctx, 2);
        GRect bounding_box = GRect(min_x, min_y, max_x - min_x, max_y - min_y);
        graphics_draw_rect(ctx, bounding_box);
    }

    // Draw point masses as colored dots - red if colliding with other bodies, white if not
    for (int i = 0; i < body->point_count; i++) {
        GPoint point_pos = body->points[i].position;
        bool is_colliding = false;

        // Check if this point is inside any other body
        for (int j = 0; j < other_body_count; j++) {
            if (soft_body_contains_point(other_bodies[j], point_pos)) {
                is_colliding = true;
                break;
            }
        }

        // Set color based on collision state
        if (is_colliding) {
            graphics_context_set_fill_color(ctx, GColorRed);
        } else {
            graphics_context_set_fill_color(ctx, GColorWhite);
        }

        // Draw the point as a small filled circle (3px radius)
        graphics_fill_circle(ctx, point_pos, 3);
    }

    // Cleanup
    gpath_destroy(path);
    free(points);
}

