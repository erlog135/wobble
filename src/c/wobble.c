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
static AppTimer *s_tap_return_timer = NULL;
static bool s_physics_running = false;
static bool s_tap_animation_in_progress = false;  // Track if tap animation is currently happening
static GRect s_bounds;
static int s_display_hour = 0;
static int s_display_minute = 0;
static GPoint s_tap_offsets[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};  // Store offsets for each digit position
static bool s_pending_time_update = false;  // Track if a time update is pending
static int s_pending_hour_24h = 0;  // Pending hour (24-hour format)
static int s_pending_minute = 0;  // Pending minute

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
        case 2:  return PBL_IF_COLOR_ELSE(GColorOrange,GColorBlack);
        case 3:  return GColorPictonBlue;
        case 4:  return GColorGreen;
        case 5:  return PBL_IF_COLOR_ELSE(GColorRichBrilliantLavender, GColorDarkGray);
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
static void prv_tap_handler(AccelAxisType axis, int32_t direction);
static void prv_tap_return_callback(void *data);
static void prv_update_time_display(int hour_24h, int minute);

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
        
        // If tap animation was in progress, mark it as complete now that all bodies are sleeping
        if (s_tap_animation_in_progress) {
            s_tap_animation_in_progress = false;
        }
        
        // Apply pending time update if one exists
        if (s_pending_time_update) {
            s_pending_time_update = false;
            int pending_hour = s_pending_hour_24h;
            int pending_minute = s_pending_minute;
            // Clear pending values before calling (in case it needs to defer again)
            s_pending_hour_24h = 0;
            s_pending_minute = 0;
            // Apply the update (physics is now sleeping, so it will apply immediately)
            prv_update_time_display(pending_hour, pending_minute);
        }
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
    
    // Calculate center points
    int center_x = bounds.size.w / 2;
    int center_y = bounds.size.h / 2;
    
    // Calculate half spacing (first line distance from center)
    int half_spacing_x = layout->grid_spacing_x / 2;
    int half_spacing_y = layout->grid_spacing_y / 2;
    
    // Draw vertical lines (centered, going outward in both directions)
    // Start from center ± half_spacing, then add full spacing each time
    for (int offset = half_spacing_x; offset <= center_x; offset += layout->grid_spacing_x) {
        // Draw line to the right of center
        int x_right = center_x + offset;
        if (x_right < bounds.size.w) {
            graphics_fill_rect(ctx, GRect(x_right, 0, 1, bounds.size.h), 0, GCornerNone);
        }
        
        // Draw line to the left of center (if not at center)
        if (offset > 0) {
            int x_left = center_x - offset;
            if (x_left >= 0) {
                graphics_fill_rect(ctx, GRect(x_left, 0, 1, bounds.size.h), 0, GCornerNone);
            }
        }
    }
    
    // Draw horizontal lines (centered, going outward in both directions)
    // Start from center ± half_spacing, then add full spacing each time
    for (int offset = half_spacing_y; offset <= center_y; offset += layout->grid_spacing_y) {
        // Draw line below center
        int y_bottom = center_y + offset;
        if (y_bottom < bounds.size.h) {
            graphics_fill_rect(ctx, GRect(0, y_bottom, bounds.size.w, 1), 0, GCornerNone);
        }
        
        // Draw line above center (if not at center)
        if (offset > 0) {
            int y_top = center_y - offset;
            if (y_top >= 0) {
                graphics_fill_rect(ctx, GRect(0, y_top, bounds.size.w, 1), 0, GCornerNone);
            }
        }
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

// Tap handler - translates each digit in a random direction
static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
    // Ignore new taps if animation is already in progress
    if (s_tap_animation_in_progress) {
        return;
    }
    
    // Mark that tap animation is starting
    s_tap_animation_in_progress = true;
    
    // Cancel any pending return timer (shouldn't happen, but be safe)
    if (s_tap_return_timer) {
        app_timer_cancel(s_tap_return_timer);
        s_tap_return_timer = NULL;
    }
 
    // Generate random offsets for each digit
    for (int pos = 0; pos < 4; pos++) {
        int body_idx = s_display_digits.body_idx[pos];
        if (body_idx >= 0 && body_idx < s_soft_body_count) {
            SoftBody *body = &s_soft_bodies[body_idx];

            // Randomly split 15 between x and y, and randomly assign each positive or negative
            int total = 15;

            // Choose a split of total (at least 1 in each direction)
            int split = (rand() % (total - 1)) + 1; // split in [1,14]
            int mag_x = split;
            int mag_y = total - split;

            // Ensure the offset magnitude is always > 14
            // (since sqrt(mag_x^2 + mag_y^2) >= sqrt(1^2+14^2)=~14, always > 14)

            // Randomize sign for x and y
            int sign_x = (rand() % 2) == 0 ? 1 : -1;
            int sign_y = (rand() % 2) == 0 ? 1 : -1;

            int offset_x = mag_x * sign_x;
            int offset_y = mag_y * sign_y;

            s_tap_offsets[pos] = GPoint(offset_x, offset_y);

            // Translate the body in the random direction
            if (body->frame) {
                soft_body_translate_with_lag(body, s_tap_offsets[pos], 20);
            }
        } else {
            // No body at this position, set offset to zero
            s_tap_offsets[pos] = GPoint(0, 0);
        }
    }

    // Schedule return translation after a short delay (200ms)
    s_tap_return_timer = app_timer_register(200, prv_tap_return_callback, NULL);

    // Restart physics timer to animate the movement
    prv_restart_physics_timer_if_needed();
}

// Timer callback to translate digits back to original position
static void prv_tap_return_callback(void *data) {
    s_tap_return_timer = NULL;
    
    // Translate each digit back to original position (negate the offset)
    for (int pos = 0; pos < 4; pos++) {
        int body_idx = s_display_digits.body_idx[pos];
        if (body_idx >= 0 && body_idx < s_soft_body_count) {
            SoftBody *body = &s_soft_bodies[body_idx];
            
            // Negate the offset to return to original position
            GPoint return_offset = GPoint(-s_tap_offsets[pos].x, -s_tap_offsets[pos].y);
            
            // Translate back
            if (body->frame && (return_offset.x != 0 || return_offset.y != 0)) {
                soft_body_translate_with_lag(body, return_offset, 20);
            }
            
            // Reset stored offset
            s_tap_offsets[pos] = GPoint(0, 0);
        }
    }
    
    // Don't reset s_tap_animation_in_progress here - wait for all bodies to sleep
    // The flag will be reset in the physics timer callback when all bodies are sleeping
    
    // Restart physics timer to animate the return movement
    prv_restart_physics_timer_if_needed();
}

// Unobstructed area change handler - called only after the unobstructed area has changed
static void prv_unobstructed_area_did_change(void *context) {

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Unobstructed area did change");

    Layer *window_layer = window_get_root_layer(s_window);
    s_bounds = layer_get_bounds(window_layer);
    
    // Get updated unobstructed bounds
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(window_layer);
    
    // Store old layout positions before updating
    const Layout *old_layout = get_layout();
    GPoint old_positions[4];
    for (int i = 0; i < 4; i++) {
        old_positions[i] = old_layout->digit_positions[i];
    }
    
    // Update layout with new unobstructed bounds
    set_layout(s_bounds, unobstructed_bounds);
    
    // Get new layout
    const Layout *layout = get_layout();
    bool is_landscape = s_bounds.size.w > s_bounds.size.h;
    float new_target_scale_y = is_landscape ? (DEFAULT_TARGET_SCALE_Y / 2.0f) : DEFAULT_TARGET_SCALE_Y;
    
    // Update existing bodies: reposition and rescale
    for (int pos = 0; pos < 4; pos++) {
        int body_idx = s_display_digits.body_idx[pos];
        if (body_idx >= 0 && body_idx < s_soft_body_count) {
            SoftBody *body = &s_soft_bodies[body_idx];

            
            // Calculate offset to move from old position to new position
            GPoint old_pos = old_positions[pos];
            GPoint new_pos = layout->digit_positions[pos];
            GPoint offset = GPoint(new_pos.x - old_pos.x, new_pos.y - old_pos.y);
            
            // Translate frame and body to new position with lag for smooth animation
            if (body->frame && (offset.x != 0 || offset.y != 0)) {
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Translating body %d with lag: offset=(%d,%d)", body_idx, offset.x, offset.y);
                soft_body_translate_with_lag(body, offset, 20);
            }
            
            // Update target scale if landscape mode changed
            if (body->target_scale.y != new_target_scale_y) {
                body->target_scale.y = new_target_scale_y;
                // Wake body to animate to new scale
                soft_body_wake(body);
            }
            
            // Mark layer dirty
            if (s_body_layers[body_idx]) {
                layer_mark_dirty(s_body_layers[body_idx]);
            }
        }
    }
    
    // Mark background layer dirty to redraw with new layout
    layer_mark_dirty(s_background_layer);
    
    // Restart physics timer if needed to animate position and scale changes
    prv_restart_physics_timer_if_needed();
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
    // If physics is running, defer the update until physics sleeps
    if (s_physics_running) {
        s_pending_time_update = true;
        s_pending_hour_24h = hour_24h;
        s_pending_minute = minute;
        return;
    }
    
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
    
    // Get unobstructed bounds (excludes bottom obstruction area)
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(window_layer);
    
    // Initialize layout with screen bounds and unobstructed bounds (singleton)
    set_layout(s_bounds, unobstructed_bounds);
    
    // Initialize body layers array
    for (int i = 0; i < MAX_SOFT_BODIES; i++) {
        s_body_layers[i] = NULL;
    }
    
    // Create background layer
    s_background_layer = layer_create(s_bounds);
    layer_set_update_proc(s_background_layer, prv_background_update_proc);
    layer_add_child(window_layer, s_background_layer);
    
    // Subscribe to unobstructed area service to handle dynamic obstruction changes
    UnobstructedAreaHandlers handlers = {
        .did_change = prv_unobstructed_area_did_change
    };
    unobstructed_area_service_subscribe(handlers, window);
    
    // Subscribe to tap service for digit animation
    accel_tap_service_subscribe(prv_tap_handler);
    
    // Initialize random seed using current time
    srand(time(NULL));
    
    // Mark background layer dirty for initial draw
    layer_mark_dirty(s_background_layer);
    
    // Show time on launch after configurable delay
    s_initial_delay_timer = app_timer_register(INITIAL_NUMERAL_DELAY_MS, prv_initial_delay_callback, NULL);
    
    // Physics timer will start automatically when first body is added
}

static void prv_window_unload(Window *window) {
    // Unsubscribe from unobstructed area service
    unobstructed_area_service_unsubscribe();
    
    // Unsubscribe from tap service
    accel_tap_service_unsubscribe();
    
    // Cancel tap return timer if pending
    if (s_tap_return_timer) {
        app_timer_cancel(s_tap_return_timer);
        s_tap_return_timer = NULL;
    }
    
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
