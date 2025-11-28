#pragma once

#include <pebble.h>
#include "physics/physics.h"

// Default layout configuration constants
#define DEFAULT_START_SCALE_X 0.2f
#define DEFAULT_START_SCALE_Y 0.4f
#define DEFAULT_TARGET_SCALE_X 0.8f
#define DEFAULT_TARGET_SCALE_Y 0.8f
#define DEFAULT_SCALE_SPEED_X 0.8f
#define DEFAULT_SCALE_SPEED_Y 0.8f

#define DEFAULT_GRID_SPACING_X 25
#define DEFAULT_GRID_SPACING_Y 25
#define DEFAULT_GRID_ENABLED true

#define NUMERAL_X_PADDING 10
#define NUMERAL_BOTTOM_PADDING 2

// Widget layout constants
#define WIDGET_OUTLINE_WIDTH 2
#define EDGE_PADDING 0
#define BATTERY_BAR_PADDING 3
#define WIDGET_BAR_HEIGHT 16
#define DOTW_CIRCLE_RADIUS 10
#define DOTW_TEXT_FONT FONT_KEY_GOTHIC_18_BOLD
#define DATE_TEXT_FONT FONT_KEY_GOTHIC_18_BOLD

// Layout structure containing all layout properties
typedef struct {
    // Screen bounds
    GRect bounds;
    
    // Digit positions (4 positions for hour tens, hour units, minute tens, minute units)
    GPoint digit_positions[4];
    
    // Quadrant layout properties
    int quad_center_x;
    int quad_center_y;
    int quad_width;
    int quad_height;
    
    // Shape scale properties
    Scale2D start_scale;
    Scale2D target_scale;
    Scale2D scale_speed;
    
    // Grid properties
    int grid_spacing_x;
    int grid_spacing_y;
    int grid_offset_x;
    int grid_offset_y;
    bool grid_enabled;

    //battery bar properties
    GRect battery_bar_bounds;

    //date position;
    GPoint date_position;

    //day of the week position;
    GPoint dotw_position;

} Layout;

// Set layout properties based on screen bounds
// This function calculates all layout positions and properties
// Uses singleton pattern - initializes the global layout instance
void set_layout(GRect bounds);

// Get the singleton layout instance
const Layout* get_layout(void);

