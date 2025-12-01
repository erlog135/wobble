#pragma once

#include <pebble.h>

// Demo mode: Define DEMO_MODE to enable demo mode with fixed values
// Demo values: Time 12:35, Battery 100%, Date 02.12, Day Thursday
// Comment out the line below to disable demo mode
// #define DEMO_MODE

#ifdef DEMO_MODE
// Enable/disable demo mode
void widgets_set_demo_mode(bool enabled);
#endif

// Draw all widgets using the singleton layout
void widgets_draw(GContext *ctx);

// Draw battery bar widget
void widgets_draw_battery_bar(GContext *ctx);

// Draw date text widget (mm.dd format)
void widgets_draw_date(GContext *ctx);

// Draw day of week widget (2-letter text in circle)
void widgets_draw_day_of_week(GContext *ctx);

