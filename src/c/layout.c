#include "layout.h"

// Singleton layout instance
static Layout s_layout_instance;

void set_layout(GRect bounds) {
    Layout *layout = &s_layout_instance;
    
    // Store bounds
    layout->bounds = bounds;
    
    // Calculate quadrant layout properties,
    // but reserve WIDGET_BAR_HEIGHT at the top for widgets.
    layout->quad_center_x = bounds.size.w / 4;
    layout->quad_center_y = (bounds.size.h - WIDGET_BAR_HEIGHT) / 4 + WIDGET_BAR_HEIGHT;
    layout->quad_width = bounds.size.w / 2;
    layout->quad_height = (bounds.size.h - WIDGET_BAR_HEIGHT) / 2;

    // Calculate digit positions (4 quadrants), shifted down by WIDGET_BAR_HEIGHT
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
    layout->target_scale.y = DEFAULT_TARGET_SCALE_Y;
    layout->scale_speed.x = DEFAULT_SCALE_SPEED_X;
    layout->scale_speed.y = DEFAULT_SCALE_SPEED_Y;
    
    // Set grid properties
    layout->grid_spacing_x = DEFAULT_GRID_SPACING_X;
    layout->grid_spacing_y = DEFAULT_GRID_SPACING_Y;
    layout->grid_enabled = DEFAULT_GRID_ENABLED;
    
    // Calculate grid offsets to center the grid
    layout->grid_offset_x = (bounds.size.w % layout->grid_spacing_x) / 2;
    layout->grid_offset_y = (bounds.size.h % layout->grid_spacing_y) / 2;

    //battery bar is at the padded top left, taking up (up to) 1/3 of the screen width
    layout->battery_bar_bounds = GRect(2, 2, bounds.size.w / 3, WIDGET_BAR_HEIGHT);

    //date is at the padded top right
    layout->date_position = GPoint(bounds.size.w - EDGE_PADDING - WIDGET_BAR_HEIGHT - DOTW_CIRCLE_RADIUS*2, EDGE_PADDING);

    //day of the week is at the padded top right
    layout->dotw_position = GPoint(bounds.size.w - DOTW_CIRCLE_RADIUS - EDGE_PADDING, EDGE_PADDING + DOTW_CIRCLE_RADIUS);
}

const Layout* get_layout(void) {
    return &s_layout_instance;
}

