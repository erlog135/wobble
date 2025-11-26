#include <pebble.h>
#include "physics/physics.h"
#include "physics/numerals.h"

#define MAX_SOFT_BODIES 10
#define PHYSICS_TIMER_MS 33  // ~30 FPS
#define SHAPE_APPEAR_DELAY_MS 50  // Delay between shapes appearing (milliseconds)

static Window *s_window;
static Layer *s_canvas_layer;
static SoftBody s_soft_bodies[MAX_SOFT_BODIES];
static int s_soft_body_count = 0;
static AppTimer *s_physics_timer = NULL;
static bool s_physics_running = false;
static GRect s_bounds;

typedef struct {
    const GPoint *points;
    int count;
} ShapeDef;

// All numeral shapes from numerals.h
static const ShapeDef s_shapes[] = {
    {zero_points, ZERO_POINT_COUNT},
    {one_points, ONE_POINT_COUNT},
    {two_points, TWO_POINT_COUNT},
    {three_points, THREE_POINT_COUNT},
    {four_points, FOUR_POINT_COUNT},
    {five_points, FIVE_POINT_COUNT},
    {six_points, SIX_POINT_COUNT},
    {seven_points, SEVEN_POINT_COUNT},
    {eight_points, EIGHT_POINT_COUNT},
    {nine_points, NINE_POINT_COUNT}
};
#define SHAPE_COUNT (sizeof(s_shapes) / sizeof(s_shapes[0]))

// Shared scaling options for all shapes
#define SHAPE_START_SCALE 0.4f
#define SHAPE_TARGET_SCALE 0.8f
#define SHAPE_SCALE_SPEED 0.8f

// Structure to pass quadrant information to timer callback
typedef struct {
    int shape_idx;
    GPoint position;
} QuadrantData;

static void prv_add_shape_at_position(const ShapeDef *shape, GPoint center_pos) {
    if (s_soft_body_count >= MAX_SOFT_BODIES) {
        return; // Max bodies reached
    }
    
    // Calculate shape bounding box to center it properly
    int min_x = shape->points[0].x;
    int max_x = shape->points[0].x;
    int min_y = shape->points[0].y;
    int max_y = shape->points[0].y;
    
    for (int i = 1; i < shape->count; i++) {
        if (shape->points[i].x < min_x) min_x = shape->points[i].x;
        if (shape->points[i].x > max_x) max_x = shape->points[i].x;
        if (shape->points[i].y < min_y) min_y = shape->points[i].y;
        if (shape->points[i].y > max_y) max_y = shape->points[i].y;
    }
    
    // Calculate shape center
    int shape_center_x = (min_x + max_x) / 2;
    int shape_center_y = (min_y + max_y) / 2;
    
    // Calculate offset to place shape center at desired position
    int offset_x = center_pos.x - shape_center_x;
    int offset_y = center_pos.y - shape_center_y;
    
    // Create GPoint array with offset
    GPoint *positions = (GPoint *)malloc(sizeof(GPoint) * shape->count);
    for (int i = 0; i < shape->count; i++) {
        positions[i] = GPoint(
            shape->points[i].x + offset_x,
            shape->points[i].y + offset_y
        );
    }
    
    // Initialize soft body with shared scaling options
    SoftBody *body = &s_soft_bodies[s_soft_body_count];
    soft_body_init(body, positions, shape->count, 1, FRAME_SPRING_DAMPING_DEFAULT, 
                   SHAPE_START_SCALE, SHAPE_TARGET_SCALE, SHAPE_SCALE_SPEED);
    s_soft_body_count++;
    
    free(positions);
    
    // Mark layer dirty to show the new shape
    layer_mark_dirty(s_canvas_layer);
}

static void prv_add_random_shape(void) {
    // Pick random shape
    int shape_idx = rand() % SHAPE_COUNT;
    const ShapeDef *shape = &s_shapes[shape_idx];
    
    // Random position (keep shape within bounds)
    int max_x = s_bounds.size.w - 50;
    int max_y = s_bounds.size.h / 2 - 50;
    int offset_x = (rand() % (max_x > 0 ? max_x : s_bounds.size.w)) + 10;
    int offset_y = (rand() % (max_y > 0 ? max_y : s_bounds.size.h)) + 10;
    
    GPoint random_pos = GPoint(offset_x, offset_y);
    prv_add_shape_at_position(shape, random_pos);
}

static void prv_physics_timer_callback(void *data) {
    // Update physics for all soft bodies

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Physics timer callback");
    for (int i = 0; i < s_soft_body_count; i++) {
        SoftBody *body = &s_soft_bodies[i];

        // Animate frame scale from start_scale to target_scale
        if (body->frame && body->frame->current_scale != body->target_scale) {
            // Wake the softbody if it's sleeping (scaling will disturb it)
            if (body->is_sleeping) {
                soft_body_wake(body);
            }

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

        // Update physics only for non-sleeping softbodies
        if (!body->is_sleeping) {
            soft_body_update(body, 0.016f); // ~16ms timestep
        }
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

// Timer callback to add a shape at a specific quadrant
static void prv_add_shape_timer_callback(void *data) {
    QuadrantData *quad_data = (QuadrantData *)data;
    
    // Add the shape at the specified position
    prv_add_shape_at_position(&s_shapes[quad_data->shape_idx], quad_data->position);
    
    // Free the allocated memory
    free(quad_data);
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Clear all bodies
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_destroy(&s_soft_bodies[i]);
    }
    s_soft_body_count = 0;
    
    // Calculate quadrant centers
    int quad_center_x = s_bounds.size.w / 4;
    int quad_center_y = s_bounds.size.h / 4;
    int quad_width = s_bounds.size.w / 2;
    int quad_height = s_bounds.size.h / 2;
    
    // Schedule 4 random shapes to appear with delays
    // Top-left quadrant (immediate)
    QuadrantData *quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = rand() % SHAPE_COUNT;
    quad_data->position = GPoint(quad_center_x, quad_center_y);
    app_timer_register(0, prv_add_shape_timer_callback, quad_data);
    
    // Top-right quadrant (after delay)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = rand() % SHAPE_COUNT;
    quad_data->position = GPoint(quad_center_x + quad_width, quad_center_y);
    app_timer_register(SHAPE_APPEAR_DELAY_MS, prv_add_shape_timer_callback, quad_data);
    
    // Bottom-left quadrant (after 2x delay)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = rand() % SHAPE_COUNT;
    quad_data->position = GPoint(quad_center_x, quad_center_y + quad_height);
    app_timer_register(SHAPE_APPEAR_DELAY_MS * 2, prv_add_shape_timer_callback, quad_data);
    
    // Bottom-right quadrant (after 3x delay)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = rand() % SHAPE_COUNT;
    quad_data->position = GPoint(quad_center_x + quad_width, quad_center_y + quad_height);
    app_timer_register(SHAPE_APPEAR_DELAY_MS * 3, prv_add_shape_timer_callback, quad_data);
    
    layer_mark_dirty(s_canvas_layer);
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
