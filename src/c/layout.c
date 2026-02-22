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
    
    // On round: ROUND_NUMERAL_INSET is the sole spacing rule applied symmetrically on all
    // four sides. widget_bar_height, NUMERAL_X_PADDING, and NUMERAL_BOTTOM_PADDING are all
    // ignored — the battery and date widgets sit at the absolute screen edges and ROUND_NUMERAL_INSET
    // is large enough to clear them.
    // On rect: widget_bar_height reserves space at the top, NUMERAL_X_PADDING pads horizontally,
    // and NUMERAL_BOTTOM_PADDING adds a small bottom margin.
#ifdef PBL_ROUND
    int numeral_area_x = ROUND_NUMERAL_INSET_X;
    int numeral_area_w = unobstructed_bounds.size.w - ROUND_NUMERAL_INSET_X * 2;
    int numeral_area_y = ROUND_NUMERAL_INSET_Y;
    int numeral_area_h = unobstructed_bounds.size.h - ROUND_NUMERAL_INSET_Y * 2;
#else
    int numeral_area_x = NUMERAL_X_PADDING;
    int numeral_area_w = unobstructed_bounds.size.w - NUMERAL_X_PADDING * 2;
    int numeral_area_y = layout->widget_bar_height;
    int numeral_area_h = unobstructed_bounds.size.h - layout->widget_bar_height - NUMERAL_BOTTOM_PADDING;
#endif

    // Calculate quadrant layout properties within numeral area,
    // but reserve widget_bar_height at the top for widgets and NUMERAL_BOTTOM_PADDING at the bottom.
    layout->quad_center_x = numeral_area_x + numeral_area_w / 4;
    layout->quad_center_y = numeral_area_y + numeral_area_h / 4;
    layout->quad_width = numeral_area_w / 2;
    layout->quad_height = numeral_area_h / 2;

    // When obstructed, stagger rows horizontally: top row shifts left, bottom row shifts right
    int top_row_shift = is_obstructed ? -OBSTRUCTION_ROW_SHIFT : 0;
    int bot_row_shift = is_obstructed ?  OBSTRUCTION_ROW_SHIFT : 0;

    // Calculate digit positions (4 quadrants), shifted down by widget_bar_height, symmetrically padded by NUMERAL_X_PADDING
    // DIGIT_POS_HOUR_TENS = 0
    layout->digit_positions[0] = GPoint(layout->quad_center_x + top_row_shift, layout->quad_center_y);
    // DIGIT_POS_HOUR_UNITS = 1
    layout->digit_positions[1] = GPoint(layout->quad_center_x + layout->quad_width + top_row_shift, layout->quad_center_y);
    // DIGIT_POS_MINUTE_TENS = 2
    layout->digit_positions[2] = GPoint(layout->quad_center_x + bot_row_shift, layout->quad_center_y + layout->quad_height);
    // DIGIT_POS_MINUTE_UNITS = 3
    layout->digit_positions[3] = GPoint(layout->quad_center_x + layout->quad_width + bot_row_shift, layout->quad_center_y + layout->quad_height);
    
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
    
    // Battery bar: on round, center horizontally at y=0; on rect, left-aligned with EDGE_PADDING
    #ifdef PBL_ROUND
    int battery_bar_w = bounds.size.w / 3 - 10;
    layout->battery_bar_bounds = GRect((bounds.size.w - battery_bar_w) / 2, ROUND_WIDGET_PADDING, battery_bar_w, layout->widget_bar_height);
    #else
    int battery_bar_w = bounds.size.w / 3;
    layout->battery_bar_bounds = GRect(EDGE_PADDING, EDGE_PADDING, battery_bar_w, layout->widget_bar_height);
#endif

    // Date: on round, centered at the very bottom; on rect, top-right with EDGE_PADDING offsets
#ifdef PBL_ROUND
    layout->date_position = GPoint(bounds.size.w / 2, bounds.size.h - layout->widget_bar_height - ROUND_WIDGET_PADDING - 6);
#else
    layout->date_position = GPoint(bounds.size.w - EDGE_PADDING - DOTW_CIRCLE_RADIUS*4, EDGE_PADDING - 4);
#endif

    // Day of the week: top-right corner (not drawn on round, so only rect constants here)
    layout->dotw_position = GPoint(bounds.size.w - DOTW_CIRCLE_RADIUS - EDGE_PADDING - WIDGET_OUTLINE_WIDTH, EDGE_PADDING + DOTW_CIRCLE_RADIUS + WIDGET_OUTLINE_WIDTH);
}

const Layout* get_layout(void) {
    return &s_layout_instance;
}

