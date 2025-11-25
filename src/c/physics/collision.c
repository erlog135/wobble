#include "physics.h"

void collision_check_bounds(SoftBody *body, GRect bounds) {
    for (int i = 0; i < body->point_count; i++) {
        collision_resolve_bounds(&body->points[i], bounds);
    }
}

bool collision_point_rect(GPoint point, GRect rect) {
    return (point.x >= rect.origin.x &&
            point.x <= rect.origin.x + rect.size.w &&
            point.y >= rect.origin.y &&
            point.y <= rect.origin.y + rect.size.h);
}

void collision_resolve_bounds(PointMass *pm, GRect bounds) {
    int min_x = bounds.origin.x;
    int max_x = bounds.origin.x + bounds.size.w;
    int min_y = bounds.origin.y;
    int max_y = bounds.origin.y + bounds.size.h;

    // Check and resolve X boundaries
    if (pm->position.x < min_x) {
        pm->position.x = min_x;
        pm->velocity.x *= -0.5f; // Bounce with energy loss
    } else if (pm->position.x > max_x) {
        pm->position.x = max_x;
        pm->velocity.x *= -0.5f;
    }

    // Check and resolve Y boundaries
    if (pm->position.y < min_y) {
        pm->position.y = min_y;
        pm->velocity.y *= -0.5f; // Bounce with energy loss
    } else if (pm->position.y > max_y) {
        pm->position.y = max_y;
        pm->velocity.y *= -0.5f;
    }
}

// Check collisions between all soft bodies
void collision_check_all_bodies(SoftBody *bodies, int body_count) {
    for (int i = 0; i < body_count; i++) {
        for (int j = i + 1; j < body_count; j++) {
            collision_check_body_pair(&bodies[i], &bodies[j]);
        }
    }
}

// Check collisions between two specific soft bodies
void collision_check_body_pair(SoftBody *body1, SoftBody *body2) {
    // Check point-to-point collisions
    for (int i = 0; i < body1->point_count; i++) {
        for (int j = 0; j < body2->point_count; j++) {
            if (collision_point_point(&body1->points[i], &body2->points[j], 5)) {
                collision_resolve_point_point(&body1->points[i], &body2->points[j], 5);
            }
        }
    }

    // Check point-to-spring collisions for body1 points vs body2 springs
    for (int i = 0; i < body1->point_count; i++) {
        for (int j = 0; j < body2->spring_count; j++) {
            if (collision_point_spring(&body1->points[i], &body2->springs[j], 3)) {
                collision_resolve_point_spring(&body1->points[i], &body2->springs[j], 3);
            }
        }
    }

    // Check point-to-spring collisions for body2 points vs body1 springs
    for (int i = 0; i < body2->point_count; i++) {
        for (int j = 0; j < body1->spring_count; j++) {
            if (collision_point_spring(&body2->points[i], &body1->springs[j], 3)) {
                collision_resolve_point_spring(&body2->points[i], &body1->springs[j], 3);
            }
        }
    }
}

// Check if two points are colliding (within minimum distance)
bool collision_point_point(PointMass *p1, PointMass *p2, int min_distance) {
    int dx = p1->position.x - p2->position.x;
    int dy = p1->position.y - p2->position.y;
    int distance_squared = dx * dx + dy * dy;
    int min_distance_squared = min_distance * min_distance;

    return distance_squared < min_distance_squared;
}

// Resolve collision between two points by pushing them apart
void collision_resolve_point_point(PointMass *p1, PointMass *p2, int min_distance) {
    int dx = p1->position.x - p2->position.x;
    int dy = p1->position.y - p2->position.y;

    // Calculate distance
    int distance = physics_sqrt(dx * dx + dy * dy);
    if (distance == 0) {
        // Points are at same position, separate them arbitrarily
        dx = min_distance;
        dy = 0;
        distance = min_distance;
    }

    // Calculate overlap
    int overlap = min_distance - distance;
    if (overlap <= 0) return;

    // Normalize direction vector
    float nx = (float)dx / distance;
    float ny = (float)dy / distance;

    // Move points apart (equal mass assumption)
    float separation_x = nx * overlap * 0.5f;
    float separation_y = ny * overlap * 0.5f;

    p1->position.x += (int)(separation_x + 0.5f);
    p1->position.y += (int)(separation_y + 0.5f);
    p2->position.x -= (int)(separation_x + 0.5f);
    p2->position.y -= (int)(separation_y + 0.5f);

    // Exchange velocities (simple elastic collision)
    GPoint temp_vel = p1->velocity;
    p1->velocity = p2->velocity;
    p2->velocity = temp_vel;

    // Apply damping to prevent excessive bouncing
    p1->velocity.x = (int)(p1->velocity.x * 0.8f);
    p1->velocity.y = (int)(p1->velocity.y * 0.8f);
    p2->velocity.x = (int)(p2->velocity.x * 0.8f);
    p2->velocity.y = (int)(p2->velocity.y * 0.8f);
}

// Check if a point is colliding with a spring (line segment)
bool collision_point_spring(PointMass *point, Spring *spring, int min_distance) {
    GPoint p1 = spring->point1->position;
    GPoint p2 = spring->point2->position;
    GPoint p = point->position;

    // Vector from p1 to p2
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;

    // Vector from p1 to p
    int dx_p = p.x - p1.x;
    int dy_p = p.y - p1.y;

    // Length squared of spring
    int len_squared = dx * dx + dy * dy;
    if (len_squared == 0) {
        // Spring has zero length, check distance to endpoint
        int dist_x = p.x - p1.x;
        int dist_y = p.y - p1.y;
        return (dist_x * dist_x + dist_y * dist_y) < (min_distance * min_distance);
    }

    // Project point onto line segment
    float t = (float)(dx_p * dx + dy_p * dy) / len_squared;

    // Clamp t to [0, 1] to stay within segment
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    // Find closest point on segment
    int closest_x = (int)(p1.x + t * dx + 0.5f);
    int closest_y = (int)(p1.y + t * dy + 0.5f);

    // Calculate distance from point to closest point on segment
    int dist_x = p.x - closest_x;
    int dist_y = p.y - closest_y;
    int distance_squared = dist_x * dist_x + dist_y * dist_y;

    return distance_squared < (min_distance * min_distance);
}

// Resolve collision between a point and a spring
void collision_resolve_point_spring(PointMass *point, Spring *spring, int min_distance) {
    GPoint p1 = spring->point1->position;
    GPoint p2 = spring->point2->position;
    GPoint p = point->position;

    // Vector from p1 to p2
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;

    // Vector from p1 to p
    int dx_p = p.x - p1.x;
    int dy_p = p.y - p1.y;

    // Length squared of spring
    int len_squared = dx * dx + dy * dy;
    if (len_squared == 0) {
        // Spring has zero length, treat as point collision with p1
        collision_resolve_point_point(point, spring->point1, min_distance);
        return;
    }

    // Project point onto line segment
    float t = (float)(dx_p * dx + dy_p * dy) / len_squared;

    // Clamp t to [0, 1] to stay within segment
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    // Find closest point on segment
    int closest_x = (int)(p1.x + t * dx + 0.5f);
    int closest_y = (int)(p1.y + t * dy + 0.5f);

    // Calculate distance and direction from point to closest point
    int dist_x = p.x - closest_x;
    int dist_y = p.y - closest_y;
    int distance = physics_sqrt(dist_x * dist_x + dist_y * dist_y);

    if (distance == 0) {
        // Point is exactly on the spring, push it away perpendicularly
        // Use spring direction as reference
        float spring_length = physics_sqrt(len_squared);
        float nx = (float)dy / spring_length;  // Perpendicular to spring
        float ny = -(float)dx / spring_length;

        point->position.x += (int)(nx * min_distance + 0.5f);
        point->position.y += (int)(ny * min_distance + 0.5f);
        return;
    }

    // Calculate overlap
    int overlap = min_distance - distance;
    if (overlap <= 0) return;

    // Normalize direction vector (from closest point to collision point)
    float nx = (float)dist_x / distance;
    float ny = (float)dist_y / distance;

    // Move point away from spring
    point->position.x += (int)(nx * overlap + 0.5f);
    point->position.y += (int)(ny * overlap + 0.5f);

    // Apply velocity reflection with damping
    // Reflect velocity relative to spring normal
    float dot_product = point->velocity.x * nx + point->velocity.y * ny;
    point->velocity.x = (int)(point->velocity.x - 2 * dot_product * nx);
    point->velocity.y = (int)(point->velocity.y - 2 * dot_product * ny);

    // Apply damping
    point->velocity.x = (int)(point->velocity.x * 0.7f);
    point->velocity.y = (int)(point->velocity.y * 0.7f);
}

