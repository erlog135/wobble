#include <pebble.h>
#include "physics/physics.h"

#define MAX_SOFT_BODIES 10
#define PHYSICS_TIMER_MS 33  // ~30 FPS

static Window *s_window;
static Layer *s_canvas_layer;
static SoftBody s_soft_bodies[MAX_SOFT_BODIES];
static int s_soft_body_count = 0;
static AppTimer *s_physics_timer = NULL;
static bool s_physics_running = false;
static GRect s_bounds;

// Shape definitions (copied from objects.h since they're static)
static const GPoint square_points[] = {
    {0, 0}, {50, 0}, {50, 50}, {0, 50}
};
static const GPoint triangle_points[] = {
    {25, 0}, {0, 50}, {50, 50}
};
static const GPoint hexagon_points[] = {
    {25, 0}, {50, 12}, {50, 37}, {25, 50}, {0, 37}, {0, 12}
};
// 80px tall number '2' as an outline polygon (clockwise)
// Coordinates chosen to look like a '2' digit, top left at (10,10), height ~80px.
static const GPoint blob_points[] = {
    {15, 15},   // Top left curve
    {55, 15},   // Top right
    {65, 25},   // Curve right
    {55, 45},   // Upper belly
    {25, 60},   // Middle transition (curl down)
    {15, 85},   // Bottom left knee
    {25, 95},   // Bottom
    {55, 95},   // Bottom right
    {65, 85},   // Bottom right curve
    {57, 80},   // Curl up into base
    {35, 70},   // Inside base curve
    {55, 55},   // Pinch middle
    {65, 40},   // Top tip
    {65, 25},   // Top right
    {15, 15}    // Closing point (matches first for polygon)
};

// 5-pointed star shape with manually positioned points for optimal physics
static const GPoint star_points[] = {
    {25, 0},    // Top point
    {32, 18},   // Top right inner
    {50, 18},   // Right point
    {37, 30},   // Bottom right inner
    {40, 50},   // Bottom right point
    {25, 38},   // Bottom inner
    {10, 50},   // Bottom left point
    {13, 30},   // Bottom left inner
    {0, 18},    // Left point
    {18, 18}    // Top left inner
};

typedef struct {
    const GPoint *points;
    int count;
} ShapeDef;

static const ShapeDef s_shapes[] = {
    {square_points, sizeof(square_points) / sizeof(square_points[0])},
    {triangle_points, sizeof(triangle_points) / sizeof(triangle_points[0])},
    {hexagon_points, sizeof(hexagon_points) / sizeof(hexagon_points[0])},
    {blob_points, sizeof(blob_points) / sizeof(blob_points[0])},
    {star_points, sizeof(star_points) / sizeof(star_points[0])}
};
#define SHAPE_COUNT (sizeof(s_shapes) / sizeof(s_shapes[0]))

static void prv_add_random_shape(void) {
    if (s_soft_body_count >= MAX_SOFT_BODIES) {
        return; // Max bodies reached
    }
    
    // Pick random shape
    int shape_idx = rand() % SHAPE_COUNT;
    const ShapeDef *shape = &s_shapes[shape_idx];
    
    // Random position (keep shape within bounds)
    int max_x = s_bounds.size.w - 50;
    int max_y = s_bounds.size.h / 2 - 50;
    int offset_x = (rand() % (max_x > 0 ? max_x : s_bounds.size.w)) + 10;
    int offset_y = (rand() % (max_y > 0 ? max_y : s_bounds.size.h)) + 10;
    
    // Create GPoint array with offset
    GPoint *positions = (GPoint *)malloc(sizeof(GPoint) * shape->count);
    for (int i = 0; i < shape->count; i++) {
        positions[i] = GPoint(
            shape->points[i].x + offset_x,
            shape->points[i].y + offset_y
        );
    }
    
    // Initialize soft body
    SoftBody *body = &s_soft_bodies[s_soft_body_count];
    soft_body_init(body, positions, shape->count, 1, FRAME_SPRING_DAMPING_DEFAULT, 1.5f, 1.0f, 0.1f);
    s_soft_body_count++;
    
    free(positions);
    
    // Mark layer dirty to show the new shape
    layer_mark_dirty(s_canvas_layer);
}

static void prv_physics_timer_callback(void *data) {
    // Update physics for all soft bodies

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Physics timer callback");
    for (int i = 0; i < s_soft_body_count; i++) {
        SoftBody *body = &s_soft_bodies[i];

        // Animate frame scale from start_scale to target_scale
        if (body->frame && body->frame->current_scale != body->target_scale) {
            // Move towards target scale using configurable speed
            float scale_diff = body->target_scale - body->frame->current_scale;
            float scale_step = (scale_diff > 0 ? body->scale_speed : -body->scale_speed);
            float new_scale = body->frame->current_scale + scale_step;

            // Clamp to target scale to prevent overshooting
            if ((scale_diff > 0 && new_scale > body->target_scale) ||
                (scale_diff < 0 && new_scale < body->target_scale)) {
                new_scale = body->target_scale;
            }

            shape_frame_set_scale(body->frame, new_scale);
        }

        // Update physics (includes spring forces and position updates)
        soft_body_update(body, 0.016f); // ~16ms timestep
    }

    // Mark layer dirty to trigger redraw
    layer_mark_dirty(s_canvas_layer);

    // Schedule next physics update (repeating timer pattern)
    if (s_physics_running) {
        s_physics_timer = app_timer_register(PHYSICS_TIMER_MS, prv_physics_timer_callback, NULL);
    }
}

static void prv_canvas_update_proc(Layer *layer, GContext *ctx) {
    // Clear background
    graphics_context_set_fill_color(ctx, GColorJazzberryJam);
    graphics_fill_rect(ctx, s_bounds, 0, GCornerNone);

    // Draw all soft bodies
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_draw(ctx, &s_soft_bodies[i]);
#if DEBUG_DRAW_ELEMENTS
        // Draw frame outline on top
        soft_body_draw_frame(ctx, &s_soft_bodies[i]);
#endif
    }
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
    prv_add_random_shape();
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Clear all bodies
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_destroy(&s_soft_bodies[i]);
    }
    s_soft_body_count = 0;
    layer_mark_dirty(s_canvas_layer);
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Remove last body
    if (s_soft_body_count > 0) {
        s_soft_body_count--;
        soft_body_destroy(&s_soft_bodies[s_soft_body_count]);
        layer_mark_dirty(s_canvas_layer);
    }
}

static void prv_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    s_bounds = layer_get_bounds(window_layer);
    
    // Create canvas layer for physics rendering
    s_canvas_layer = layer_create(s_bounds);
    layer_set_update_proc(s_canvas_layer, prv_canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);
    
    // Initialize random seed (use a simple seed)
    //srand(42);
    
    // Mark layer dirty for initial draw
    layer_mark_dirty(s_canvas_layer);
    
    // Start physics timer (repeating timer pattern)
    s_physics_running = true;
    s_physics_timer = app_timer_register(PHYSICS_TIMER_MS, prv_physics_timer_callback, NULL);
}

static void prv_window_unload(Window *window) {
    // Stop physics timer
    s_physics_running = false;
    if (s_physics_timer) {
        app_timer_cancel(s_physics_timer);
        s_physics_timer = NULL;
    }
    
    // Clean up all soft bodies
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_destroy(&s_soft_bodies[i]);
    }
    
    layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
    s_window = window_create();
    window_set_click_config_provider(s_window, prv_click_config_provider);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload,
    });
    const bool animated = true;
    window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
    window_destroy(s_window);
}

int main(void) {
    prv_init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

    app_event_loop();
    prv_deinit();
}
