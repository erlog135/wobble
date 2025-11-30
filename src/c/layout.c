#include "layout.h"

// Singleton layout instance
static Layout s_layout_instance;

void set_layout(GRect bounds, GRect unobstructed_bounds) {
    Layout *layout = &s_layout_instance;
    
    // Store bounds
    layout->bounds = bounds;
    
    // Set widget bar height - if obstructed (unobstructed height < full height), hide widgets
    bool is_obstructed = (unobstructed_bounds.size.h < bounds.size.h) || 
                         (unobstructed_bounds.size.w < bounds.size.w);
    layout->widget_bar_height = is_obstructed ? 0 : DEFAULT_WIDGET_BAR_HEIGHT;
    
    // Use unobstructed bounds for numeral area calculations (obstructions are at the bottom)
    // Calculate available width for numerals after applying horizontal paddings
    int numeral_area_x = NUMERAL_X_PADDING;
    int numeral_area_w = unobstructed_bounds.size.w - NUMERAL_X_PADDING * 2;
    int numeral_area_y = layout->widget_bar_height;
    // Use unobstructed height, accounting for widget bar at top and bottom padding
    int numeral_area_h = unobstructed_bounds.size.h - layout->widget_bar_height - NUMERAL_BOTTOM_PADDING;

    // Calculate quadrant layout properties within numeral area,
    // but reserve widget_bar_height at the top for widgets and NUMERAL_BOTTOM_PADDING at the bottom.
    layout->quad_center_x = numeral_area_x + numeral_area_w / 4;
    layout->quad_center_y = numeral_area_y + numeral_area_h / 4;
    layout->quad_width = numeral_area_w / 2;
    layout->quad_height = numeral_area_h / 2;

    // Calculate digit positions (4 quadrants), shifted down by widget_bar_height, symmetrically padded by NUMERAL_X_PADDING
    // DIGIT_POS_HOUR_TENS = 0
    layout->digit_positions[0] = GPoint(layout->quad_center_x, layout->quad_center_y);
    // DIGIT_POS_HOUR_UNITS = 1
    layout->digit_positions[1] = GPoint(layout->quad_center_x + layout->quad_width, layout->quad_center_y);
    // DIGIT_POS_MINUTE_TENS = 2
    layout->digit_positions[2] = GPoint(layout->quad_center_x, layout->quad_center_y + layout->quad_height);
    // DIGIT_POS_MINUTE_UNITS = 3
    layout->digit_positions[3] = GPoint(layout->quad_center_x + layout->quad_width, layout->quad_center_y + layout->quad_height);
    
    // Set scale properties
    layout->start_scale.x = DEFAULT_START_SCALE_X;
    layout->start_scale.y = DEFAULT_START_SCALE_Y;
    layout->target_scale.x = DEFAULT_TARGET_SCALE_X;
    
    // If screen is wider than tall (landscape), halve the target scale height
    bool is_landscape = bounds.size.w > bounds.size.h;
    layout->target_scale.y = is_landscape ? (DEFAULT_TARGET_SCALE_Y / 2.0f) : DEFAULT_TARGET_SCALE_Y;
    
    layout->scale_speed.x = DEFAULT_SCALE_SPEED_X;
    layout->scale_speed.y = DEFAULT_SCALE_SPEED_Y;
    
    // Set grid properties
    layout->grid_spacing_x = DEFAULT_GRID_SPACING_X;
    layout->grid_spacing_y = DEFAULT_GRID_SPACING_Y;
    layout->grid_enabled = DEFAULT_GRID_ENABLED;
    
    //battery bar is at the padded top left, taking up (up to) 1/3 of the screen width
    layout->battery_bar_bounds = GRect(EDGE_PADDING, EDGE_PADDING, bounds.size.w /3, layout->widget_bar_height);

    //date is at the padded top right
    layout->date_position = GPoint(bounds.size.w - EDGE_PADDING - DOTW_CIRCLE_RADIUS*4, EDGE_PADDING - 4);

    //day of the week is at the padded top right
    layout->dotw_position = GPoint(bounds.size.w - DOTW_CIRCLE_RADIUS - EDGE_PADDING - WIDGET_OUTLINE_WIDTH, EDGE_PADDING + DOTW_CIRCLE_RADIUS + WIDGET_OUTLINE_WIDTH);
}

const Layout* get_layout(void) {
    return &s_layout_instance;
}

