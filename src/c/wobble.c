#include <pebble.h>
#include <time.h>
#include "physics/physics.h"
#include "physics/numerals.h"
#include "physics/objects.h"
#include "layout.h"
#include "widgets.h"

#define MAX_SOFT_BODIES 10
#define PHYSICS_TIMER_MS 33  // ~30 FPS
#define SHAPE_APPEAR_DELAY_MS 50  // Delay between shapes appearing (milliseconds)
#define INITIAL_NUMERAL_DELAY_MS 500  // Delay before showing numerals on first launch (milliseconds)
#define SHAPE_BASE_SIZE 80  // Base size of shapes (80x80px) - shapes are centered at (40, 40)

static Window *s_window;
static Layer *s_background_layer;
static Layer *s_body_layers[MAX_SOFT_BODIES];
static SoftBody s_soft_bodies[MAX_SOFT_BODIES];
static int s_soft_body_count = 0;
static AppTimer *s_physics_timer = NULL;
static AppTimer *s_initial_delay_timer = NULL;
static bool s_physics_running = false;
static GRect s_bounds;
static int s_display_hour = 0;
static int s_display_minute = 0;

// Track which digits are currently displayed and their body indices
typedef enum {
    DIGIT_POS_HOUR_TENS = 0,
    DIGIT_POS_HOUR_UNITS = 1,
    DIGIT_POS_MINUTE_TENS = 2,
    DIGIT_POS_MINUTE_UNITS = 3
} DigitPosition;

static struct {
    int digit_value[4];  // Current digit value for each position (-1 if not set)
    int body_idx[4];      // Body index for each position (-1 if not set)
} s_display_digits = {
    .digit_value = {-1, -1, -1, -1},
    .body_idx = {-1, -1, -1, -1}
};

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

static GColor get_numeral_color(int digit) {
    switch (digit) {
        case 0:  return GColorElectricBlue;
        case 1:  return GColorYellow;
        case 2:  return GColorOrange;
        case 3:  return GColorPictonBlue;
        case 4:  return GColorGreen;
        case 5:  return GColorRichBrilliantLavender;
        case 6:  return GColorPastelYellow;
        case 7:  return GColorFolly;
        case 8:  return GColorMediumAquamarine;
        case 9:  return GColorLavenderIndigo;
        default: return GColorWhite;
    }
}

// Structure to pass quadrant information to timer callback
typedef struct {
    int shape_idx;
    GPoint position;
} QuadrantData;

// Forward declarations
static void prv_physics_timer_callback(void *data);
static void prv_restart_physics_timer_if_needed(void);

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
    
    // Optimization: shapes are always 80x80px centered at (40, 40)
    // No need to calculate bounding box - use fixed offset
    const int shape_center_x = SHAPE_BASE_SIZE / 2;  // 40
    const int shape_center_y = SHAPE_BASE_SIZE / 2;  // 40
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
    
    // Initialize soft body with layout scaling options
    int body_idx = s_soft_body_count;
    SoftBody *body = &s_soft_bodies[body_idx];
    const Layout *layout = get_layout();
    soft_body_init(body, positions, shape->count, 1, FRAME_SPRING_DAMPING_DEFAULT, 
                   layout->start_scale, layout->target_scale, layout->scale_speed);
    
    // Set digit value and fill color based on shape index
    // Find which digit this shape represents by searching s_shapes array
    int digit_value = -1;
    for (size_t i = 0; i < SHAPE_COUNT; i++) {
        if (&s_shapes[i] == shape) {
            digit_value = (int)i;
            break;
        }
    }
    body->digit_value = digit_value;
    body->fill_color = get_numeral_color(digit_value);
    
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
    
    // Restart physics timer if needed (new bodies start awake)
    prv_restart_physics_timer_if_needed();
}

// Replace an existing body with a new shape (for updating digits)
static void prv_replace_body_shape(int body_idx, const ShapeDef *shape, GPoint center_pos) {
    if (body_idx < 0 || body_idx >= s_soft_body_count) {
        return;
    }
    
    SoftBody *body = &s_soft_bodies[body_idx];
    
    // Optimization: shapes are always 80x80px centered at (40, 40)
    // No need to calculate bounding box - use fixed offset
    const int shape_center_x = SHAPE_BASE_SIZE / 2;  // 40
    const int shape_center_y = SHAPE_BASE_SIZE / 2;  // 40
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
    
    // Destroy old body
    soft_body_destroy(body);
    
    // Initialize new soft body with layout scaling options
    const Layout *layout = get_layout();
    soft_body_init(body, positions, shape->count, 1, FRAME_SPRING_DAMPING_DEFAULT, 
                   layout->start_scale, layout->target_scale, layout->scale_speed);
    
    // Set digit value and fill color based on shape index
    // Find which digit this shape represents by searching s_shapes array
    int digit_value = -1;
    for (size_t i = 0; i < SHAPE_COUNT; i++) {
        if (&s_shapes[i] == shape) {
            digit_value = (int)i;
            break;
        }
    }
    body->digit_value = digit_value;
    body->fill_color = get_numeral_color(digit_value);
    
    free(positions);
    
    // Mark layer dirty to show the updated shape
    if (s_body_layers[body_idx]) {
        layer_mark_dirty(s_body_layers[body_idx]);
    }
    
    // Restart physics timer if needed (replaced bodies start awake)
    prv_restart_physics_timer_if_needed();
}

// Update a single digit at a specific position
static void prv_update_digit(DigitPosition pos, int new_digit_value) {
    // Check if digit has changed
    if (s_display_digits.digit_value[pos] == new_digit_value) {
        return; // No change needed
    }
    
    // Get position from layout
    const Layout *layout = get_layout();
    GPoint position = layout->digit_positions[pos];
    
    // If body already exists, replace it; otherwise create new one
    if (s_display_digits.body_idx[pos] >= 0 && s_display_digits.body_idx[pos] < s_soft_body_count) {
        // Replace existing body
        prv_replace_body_shape(s_display_digits.body_idx[pos], &s_shapes[new_digit_value], position);
    } else {
        // Create new body directly (no timer delay for updates)
        prv_add_shape_at_position(&s_shapes[new_digit_value], position);
        s_display_digits.body_idx[pos] = s_soft_body_count - 1;
    }
    
    // Update tracked digit value
    s_display_digits.digit_value[pos] = new_digit_value;
}

// Helper function to restart physics timer if needed
static void prv_restart_physics_timer_if_needed(void) {
    // Only restart if not already running and we have bodies
    if (!s_physics_running && s_soft_body_count > 0) {
        s_physics_running = true;
        s_physics_timer = app_timer_register(PHYSICS_TIMER_MS, prv_physics_timer_callback, NULL);
    }
}

static void prv_physics_timer_callback(void *data) {
    // Update physics for all soft bodies

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Physics timer callback");
    bool has_active_bodies = false;
    bool has_active_scaling = false;
    
    for (int i = 0; i < s_soft_body_count; i++) {
        SoftBody *body = &s_soft_bodies[i];
        bool was_sleeping = body->is_sleeping;

        // Animate frame scale from start_scale to target_scale (separate x and y)
        bool is_scaling = body->frame && 
            (body->frame->current_scale.x != body->target_scale.x || 
             body->frame->current_scale.y != body->target_scale.y);
        
        if (is_scaling) {
            has_active_scaling = true;  // Keep timer running for scaling animations
            // Wake the softbody if it's sleeping (scaling will disturb it)
            if (body->is_sleeping) {
                soft_body_wake(body);
            }

            // Move towards target scale using configurable speed (separate for x and y)
            float scale_diff_x = body->target_scale.x - body->frame->current_scale.x;
            float scale_diff_y = body->target_scale.y - body->frame->current_scale.y;
            float scale_step_x = (scale_diff_x > 0 ? body->scale_speed.x : -body->scale_speed.x);
            float scale_step_y = (scale_diff_y > 0 ? body->scale_speed.y : -body->scale_speed.y);
            float new_scale_x = body->frame->current_scale.x + scale_step_x;
            float new_scale_y = body->frame->current_scale.y + scale_step_y;

            // Clamp to target scale to prevent overshooting (separate for x and y)
            if ((scale_diff_x > 0 && new_scale_x > body->target_scale.x) ||
                (scale_diff_x < 0 && new_scale_x < body->target_scale.x)) {
                new_scale_x = body->target_scale.x;
            }
            if ((scale_diff_y > 0 && new_scale_y > body->target_scale.y) ||
                (scale_diff_y < 0 && new_scale_y < body->target_scale.y)) {
                new_scale_y = body->target_scale.y;
            }

            Scale2D new_scale = {.x = new_scale_x, .y = new_scale_y};
            shape_frame_set_scale(body->frame, new_scale);
        }

        // Update physics only for non-sleeping softbodies
        if (!body->is_sleeping) {
            soft_body_update(body, 0.016f); // ~16ms timestep
            has_active_bodies = true;  // At least one body is active
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

    // Optimization: Only schedule next physics update if there are active bodies or scaling
    // If all bodies are sleeping and no scaling is happening, stop the timer to save CPU
    if (has_active_bodies || has_active_scaling) {
        s_physics_timer = app_timer_register(PHYSICS_TIMER_MS, prv_physics_timer_callback, NULL);
    } else {
        // All bodies are sleeping and no scaling, stop the timer
        s_physics_running = false;
        s_physics_timer = NULL;
    }
}

// Draw grid lines on the background
static void prv_draw_grid(GContext *ctx, GRect bounds) {
    const Layout *layout = get_layout();
    if (!layout->grid_enabled) {
        return;
    }
    
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorDarkGray));
    graphics_context_set_stroke_width(ctx, 1);
    
    // Draw vertical lines (centered)
    for (int x = layout->grid_offset_x; x <= bounds.size.w; x += layout->grid_spacing_x) {
        //graphics_draw_line(ctx, GPoint(x, 0), GPoint(x, bounds.size.h));
        //for now, fill a 1px wide rectangle
        graphics_fill_rect(ctx, GRect(x, 0, 1, bounds.size.h), 0, GCornerNone);
    }
    
    // Draw horizontal lines (centered)
    for (int y = layout->grid_offset_y; y <= bounds.size.h; y += layout->grid_spacing_y) {
        //graphics_draw_line(ctx, GPoint(0, y), GPoint(bounds.size.w, y));
        //for now, fill a 1px tall rectangle
        graphics_fill_rect(ctx, GRect(0, y, bounds.size.w, 1), 0, GCornerNone);
    }
}

// Background layer update proc - draws the background and grid
static void prv_background_update_proc(Layer *layer, GContext *ctx) {
    // Draw white background
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, s_bounds, 0, GCornerNone);
    
    // Draw grid lines on top
    prv_draw_grid(ctx, s_bounds);
    
    // Draw widgets (battery bar, date, day of week)
    widgets_draw(ctx);
}

// Timer callback to add a shape at a specific quadrant
// Note: Currently unused, kept for potential future use with delayed animations
__attribute__((unused)) static void prv_add_shape_timer_callback(void *data) {
    QuadrantData *quad_data = (QuadrantData *)data;
    
    // Add the shape at the specified position
    prv_add_shape_at_position(&s_shapes[quad_data->shape_idx], quad_data->position);
    
    // Free the allocated memory
    free(quad_data);
}

// Convert 24-hour format to 12-hour format
static int prv_convert_to_12h(int hour_24h) {
    if (hour_24h == 0) {
        return 12;  // Midnight -> 12
    } else if (hour_24h > 12) {
        return hour_24h - 12;  // 13-23 -> 1-11
    } else {
        return hour_24h;  // 1-12 -> 1-12
    }
}

// Update the 4 numerals to display the given time (HH:MM format)
// hour should be in 24-hour format; it will be converted based on user preference
static void prv_update_time_display(int hour_24h, int minute) {
    // Convert hour based on user's 12/24 hour preference
    int display_hour;
    if (clock_is_24h_style()) {
        display_hour = hour_24h;  // Use 24-hour format (0-23)
    } else {
        display_hour = prv_convert_to_12h(hour_24h);  // Convert to 12-hour format (1-12)
    }
    
    // Extract digits: HH:MM -> hour_tens, hour_units, minute_tens, minute_units
    int hour_tens = display_hour / 10;
    int hour_units = display_hour % 10;
    int minute_tens = minute / 10;
    int minute_units = minute % 10;
    
    // Update only changed digits
    prv_update_digit(DIGIT_POS_HOUR_TENS, hour_tens);
    prv_update_digit(DIGIT_POS_HOUR_UNITS, hour_units);
    prv_update_digit(DIGIT_POS_MINUTE_TENS, minute_tens);
    prv_update_digit(DIGIT_POS_MINUTE_UNITS, minute_units);
    
    // Update stored time values
    s_display_hour = hour_24h;
    s_display_minute = minute;
}

// Initial delay timer callback - shows numerals after delay on first launch
static void prv_initial_delay_callback(void *data) {
    s_initial_delay_timer = NULL;
    
    // Get current time and display it
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    prv_update_time_display(tick_time->tm_hour, tick_time->tm_min);
}

// Tick timer handler - called every minute
static void prv_tick_handler(struct tm *tick_time, TimeUnits changed) {
    // Update displayed time
    prv_update_time_display(tick_time->tm_hour, tick_time->tm_min);
}

static void prv_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    s_bounds = layer_get_bounds(window_layer);
    
    // Initialize layout with screen bounds (singleton)
    set_layout(s_bounds);
    
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
    
    // Show time on launch after configurable delay
    s_initial_delay_timer = app_timer_register(INITIAL_NUMERAL_DELAY_MS, prv_initial_delay_callback, NULL);
    
    // Physics timer will start automatically when first body is added
}

static void prv_window_unload(Window *window) {
    // Cancel initial delay timer if still pending
    if (s_initial_delay_timer) {
        app_timer_cancel(s_initial_delay_timer);
        s_initial_delay_timer = NULL;
    }
    
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
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload,
    });
    const bool animated = true;
    window_stack_push(s_window, animated);
    
    // Subscribe to minute tick timer to update every minute
    tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
}

static void prv_deinit(void) {
    // Unsubscribe from tick timer
    tick_timer_service_unsubscribe();
    
    window_destroy(s_window);
}

int main(void) {
    prv_init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

    app_event_loop();
    prv_deinit();
}
