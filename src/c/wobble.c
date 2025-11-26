#include <pebble.h>
#include <time.h>
#include "physics/physics.h"
#include "physics/numerals.h"
#include "physics/objects.h"

#define MAX_SOFT_BODIES 10
#define PHYSICS_TIMER_MS 33  // ~30 FPS
#define SHAPE_APPEAR_DELAY_MS 50  // Delay between shapes appearing (milliseconds)

static Window *s_window;
static Layer *s_background_layer;
static Layer *s_body_layers[MAX_SOFT_BODIES];
static SoftBody s_soft_bodies[MAX_SOFT_BODIES];
static int s_soft_body_count = 0;
static AppTimer *s_physics_timer = NULL;
static bool s_physics_running = false;
static GRect s_bounds;
static int s_display_hour = 0;
static int s_display_minute = 0;

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

// All object shapes from objects.h
static const ShapeDef s_object_shapes[] = {
    {shape_zero_points, SHAPE_ZERO_POINT_COUNT},
    {shape_one_points, SHAPE_ONE_POINT_COUNT},
    {shape_two_points, SHAPE_TWO_POINT_COUNT},
    {shape_three_points, SHAPE_THREE_POINT_COUNT},
    {shape_four_points, SHAPE_FOUR_POINT_COUNT},
    {shape_five_points, SHAPE_FIVE_POINT_COUNT},
    {shape_six_points, SHAPE_SIX_POINT_COUNT},
    {shape_seven_points, SHAPE_SEVEN_POINT_COUNT},
    {shape_eight_points, SHAPE_EIGHT_POINT_COUNT},
    {shape_nine_points, SHAPE_NINE_POINT_COUNT}
};
#define OBJECT_SHAPE_COUNT (sizeof(s_object_shapes) / sizeof(s_object_shapes[0]))

// Shared scaling options for all shapes
#define SHAPE_START_SCALE 0.4f
#define SHAPE_TARGET_SCALE 0.8f
#define SHAPE_SCALE_SPEED 0.8f

// Structure to pass quadrant information to timer callback
typedef struct {
    int shape_idx;
    GPoint position;
} QuadrantData;

// Per-body layer update proc - draws a single body
static void prv_body_layer_update_proc(Layer *layer, GContext *ctx) {
    // Find body index by searching through body layers array
    int body_idx = -1;
    for (int i = 0; i < s_soft_body_count; i++) {
        if (s_body_layers[i] == layer) {
            body_idx = i;
            break;
        }
    }
    
    if (body_idx < 0 || body_idx >= s_soft_body_count) {
        return;
    }
    
    SoftBody *body = &s_soft_bodies[body_idx];
    
    // Draw the body (layer is transparent by default)
    soft_body_draw(ctx, body);
#if DEBUG_DRAW_ELEMENTS
    // Draw frame outline on top
    soft_body_draw_frame(ctx, body);
#endif
}

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
    int body_idx = s_soft_body_count;
    SoftBody *body = &s_soft_bodies[body_idx];
    soft_body_init(body, positions, shape->count, 1, FRAME_SPRING_DAMPING_DEFAULT, 
                   SHAPE_START_SCALE, SHAPE_TARGET_SCALE, SHAPE_SCALE_SPEED);
    
    // Create a layer for this body
    Layer *window_layer = window_get_root_layer(s_window);
    Layer *body_layer = layer_create(s_bounds);
    layer_set_update_proc(body_layer, prv_body_layer_update_proc);
    layer_add_child(window_layer, body_layer);
    s_body_layers[body_idx] = body_layer;
    
    s_soft_body_count++;
    
    free(positions);
    
    // Mark layer dirty to show the new shape
    layer_mark_dirty(body_layer);
}

static void prv_physics_timer_callback(void *data) {
    // Update physics for all soft bodies

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Physics timer callback");
    for (int i = 0; i < s_soft_body_count; i++) {
        SoftBody *body = &s_soft_bodies[i];
        bool was_sleeping = body->is_sleeping;

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
        
        // Mark layer dirty if body is not sleeping, or if it just went to sleep
        // (to ensure final position is drawn when transitioning to sleep)
        bool just_went_to_sleep = was_sleeping == false && body->is_sleeping == true;
        if (!body->is_sleeping || just_went_to_sleep) {
            if (s_body_layers[i]) {
                layer_mark_dirty(s_body_layers[i]);
            }
        }
    }

    // Schedule next physics update (repeating timer pattern)
    if (s_physics_running) {
        s_physics_timer = app_timer_register(PHYSICS_TIMER_MS, prv_physics_timer_callback, NULL);
    }
}

// Background layer update proc - just draws the background
static void prv_background_update_proc(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, s_bounds, 0, GCornerNone);
}

// Timer callback to add a shape at a specific quadrant
static void prv_add_shape_timer_callback(void *data) {
    QuadrantData *quad_data = (QuadrantData *)data;
    
    // Add the shape at the specified position
    prv_add_shape_at_position(&s_shapes[quad_data->shape_idx], quad_data->position);
    
    // Free the allocated memory
    free(quad_data);
}

// Update the 4 numerals to display the given time (HH:MM format)
static void prv_update_time_display(int hour, int minute) {
    // Clear all existing bodies and their layers
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_destroy(&s_soft_bodies[i]);
        if (s_body_layers[i]) {
            layer_remove_from_parent(s_body_layers[i]);
            layer_destroy(s_body_layers[i]);
            s_body_layers[i] = NULL;
        }
    }
    s_soft_body_count = 0;
    
    // Extract digits: HH:MM -> hour_tens, hour_units, minute_tens, minute_units
    int hour_tens = hour / 10;
    int hour_units = hour % 10;
    int minute_tens = minute / 10;
    int minute_units = minute % 10;
    
    // Calculate quadrant positions for 4 numerals
    int quad_center_x = s_bounds.size.w / 4;
    int quad_center_y = s_bounds.size.h / 4;
    int quad_width = s_bounds.size.w / 2;
    int quad_height = s_bounds.size.h / 2;
    
    // Add 4 numerals: hour_tens (top-left), hour_units (top-right),
    //                 minute_tens (bottom-left), minute_units (bottom-right)
    QuadrantData *quad_data;
    
    // Hour tens (top-left)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = hour_tens;
    quad_data->position = GPoint(quad_center_x, quad_center_y);
    app_timer_register(0, prv_add_shape_timer_callback, quad_data);
    
    // Hour units (top-right)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = hour_units;
    quad_data->position = GPoint(quad_center_x + quad_width, quad_center_y);
    app_timer_register(SHAPE_APPEAR_DELAY_MS, prv_add_shape_timer_callback, quad_data);
    
    // Minute tens (bottom-left)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = minute_tens;
    quad_data->position = GPoint(quad_center_x, quad_center_y + quad_height);
    app_timer_register(SHAPE_APPEAR_DELAY_MS * 2, prv_add_shape_timer_callback, quad_data);
    
    // Minute units (bottom-right)
    quad_data = (QuadrantData *)malloc(sizeof(QuadrantData));
    quad_data->shape_idx = minute_units;
    quad_data->position = GPoint(quad_center_x + quad_width, quad_center_y + quad_height);
    app_timer_register(SHAPE_APPEAR_DELAY_MS * 3, prv_add_shape_timer_callback, quad_data);
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Get current time
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    // Update displayed time
    s_display_hour = tick_time->tm_hour;
    s_display_minute = tick_time->tm_min;
    
    // Update the 4 numerals to show current time
    prv_update_time_display(s_display_hour, s_display_minute);
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Clear all bodies and their layers
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_destroy(&s_soft_bodies[i]);
        if (s_body_layers[i]) {
            layer_remove_from_parent(s_body_layers[i]);
            layer_destroy(s_body_layers[i]);
            s_body_layers[i] = NULL;
        }
    }
    s_soft_body_count = 0;
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Increase minutes by 1
    s_display_minute++;
    
    // Handle minute overflow (wrap to next hour)
    if (s_display_minute >= 60) {
        s_display_minute = 0;
        s_display_hour++;
    }
    
    // Handle hour overflow (wrap to 0)
    if (s_display_hour >= 24) {
        s_display_hour = 0;
    }
    
    // Update the display with new time
    prv_update_time_display(s_display_hour, s_display_minute);
}

static void prv_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    s_bounds = layer_get_bounds(window_layer);
    
    // Initialize body layers array
    for (int i = 0; i < MAX_SOFT_BODIES; i++) {
        s_body_layers[i] = NULL;
    }
    
    // Create background layer
    s_background_layer = layer_create(s_bounds);
    layer_set_update_proc(s_background_layer, prv_background_update_proc);
    layer_add_child(window_layer, s_background_layer);
    
    // Initialize random seed (use a simple seed)
    //srand(42);
    
    // Mark background layer dirty for initial draw
    layer_mark_dirty(s_background_layer);
    
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
    
    // Clean up all soft bodies and their layers
    for (int i = 0; i < s_soft_body_count; i++) {
        soft_body_destroy(&s_soft_bodies[i]);
        if (s_body_layers[i]) {
            layer_remove_from_parent(s_body_layers[i]);
            layer_destroy(s_body_layers[i]);
            s_body_layers[i] = NULL;
        }
    }
    
    // Clean up background layer
    if (s_background_layer) {
        layer_destroy(s_background_layer);
        s_background_layer = NULL;
    }
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
