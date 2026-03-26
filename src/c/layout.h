#pragma once

#include <pebble.h>
#include "physics/physics.h"

// Default layout configuration constants


#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    #define DEFAULT_START_SCALE_X 0.3f
    #define DEFAULT_START_SCALE_Y 0.4f
    #define DEFAULT_TARGET_SCALE_X 1.15f
    #define DEFAULT_TARGET_SCALE_Y 1.15f

    #define DEFAULT_WIDGET_BAR_HEIGHT 18

    #define OBSTRUCTION_ROW_SHIFT 14
    #define OBSTRUCTION_NUMERAL_PADDING 8

    #define DEFAULT_GRID_SPACING_X 32
    #define DEFAULT_GRID_SPACING_Y 32
#else
    #define DEFAULT_START_SCALE_X 0.3f
    #define DEFAULT_START_SCALE_Y 0.4f
    #define DEFAULT_TARGET_SCALE_X 0.8f
    #define DEFAULT_TARGET_SCALE_Y 0.8f

    #define DEFAULT_WIDGET_BAR_HEIGHT PBL_IF_ROUND_ELSE(12, 16)

    #define OBSTRUCTION_ROW_SHIFT 8
    #define OBSTRUCTION_NUMERAL_PADDING 4

    #define DEFAULT_GRID_SPACING_X 25
    #define DEFAULT_GRID_SPACING_Y 25
#endif

// Half-height of a numeral shape (shapes are always 80x80px, centered at 40,40)
#define NUMERAL_HALF_SIZE 40

#define DEFAULT_SCALE_SPEED_X 0.5f
#define DEFAULT_SCALE_SPEED_Y 0.5f
#define DEFAULT_GRID_ENABLED true

#define NUMERAL_X_PADDING 10
#define NUMERAL_BOTTOM_PADDING 2


// Round-only spacing constants
#if defined(PBL_PLATFORM_GABBRO)
    #define ROUND_NUMERAL_INSET_X 48  // horizontal inset for the numeral quad
    #define ROUND_NUMERAL_INSET_Y 30  // vertical inset for the numeral quad
    #define ROUND_WIDGET_PADDING  12   // distance from screen edge to battery bar (top) and date (bottom)
#else
    #define ROUND_NUMERAL_INSET_X 30
    #define ROUND_NUMERAL_INSET_Y 20  
    #define ROUND_WIDGET_PADDING  8   
#endif
// Widget layout constants
#define WIDGET_OUTLINE_WIDTH 2
#define EDGE_PADDING 0
#define BATTERY_BAR_PADDING 3

#define DOTW_CIRCLE_RADIUS 10
#define DOTW_TEXT_FONT FONT_KEY_GOTHIC_18_BOLD
#define DATE_TEXT_FONT FONT_KEY_ROBOTO_CONDENSED_21

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
    bool grid_enabled;

    //battery bar properties
    GRect battery_bar_bounds;

    //date position;
    GPoint date_position;

    //day of the week position;
    GPoint dotw_position;

    //widget bar height
    int widget_bar_height;

} Layout;

// Set layout properties based on screen bounds
// This function calculates all layout positions and properties
// Uses singleton pattern - initializes the global layout instance
// bounds: full screen bounds
// unobstructed_bounds: unobstructed area (typically excludes bottom obstruction)
void set_layout(GRect bounds, GRect unobstructed_bounds);

// Get the singleton layout instance
const Layout* get_layout(void);

