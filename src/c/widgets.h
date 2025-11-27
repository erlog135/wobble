#pragma once

#include <pebble.h>

// Draw all widgets using the singleton layout
void widgets_draw(GContext *ctx);

// Draw battery bar widget
void widgets_draw_battery_bar(GContext *ctx);

// Draw date text widget (mm.dd format)
void widgets_draw_date(GContext *ctx);

// Draw day of week widget (2-letter text in circle)
void widgets_draw_day_of_week(GContext *ctx);

