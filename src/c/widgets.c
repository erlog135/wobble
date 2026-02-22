#include "widgets.h"
#include "layout.h"
#include <time.h>

#ifdef DEMO_MODE
// Demo mode flag
static bool s_demo_mode = false;

void widgets_set_demo_mode(bool enabled) {
    s_demo_mode = enabled;
}
#endif

// Day of week abbreviations (2 letters)
static const char* s_day_abbreviations[] = {
    "SU",  // Sunday
    "MO",  // Monday
    "TU",  // Tuesday
    "WE",  // Wednesday
    "TH",  // Thursday
    "FR",  // Friday
    "SA"   // Saturday
};

void widgets_draw_battery_bar(GContext *ctx) {
    const Layout *layout = get_layout();
    GRect bounds = layout->battery_bar_bounds;
    
    // Get battery state
    uint8_t battery_percent;
#ifdef DEMO_MODE
    if (s_demo_mode) {
        battery_percent = 100;  // Demo: 100%
    } else {
        BatteryChargeState battery_state = battery_state_service_peek();
        battery_percent = battery_state.charge_percent;
    }
#else
    BatteryChargeState battery_state = battery_state_service_peek();
    battery_percent = battery_state.charge_percent;
#endif
    
    // Calculate the main bar width - shrinks from full width to 0
    // Account for outline (2px on each side) and padding
    int full_width = bounds.size.w;
    int main_bar_width = (full_width * battery_percent) / 100;
    
    // On round: draw flush to the bounds origin — ROUND_WIDGET_PADDING is the sole position control.
    // On rect: inset by WIDGET_OUTLINE_WIDTH + BATTERY_BAR_PADDING so the stroke fits inside the bar area.
    int base_x = bounds.origin.x + PBL_IF_ROUND_ELSE(0, WIDGET_OUTLINE_WIDTH + BATTERY_BAR_PADDING);
    int base_y = bounds.origin.y + PBL_IF_ROUND_ELSE(0, WIDGET_OUTLINE_WIDTH + BATTERY_BAR_PADDING);
    int bar_height = bounds.size.h - PBL_IF_ROUND_ELSE(0, BATTERY_BAR_PADDING * 2);
    
    // Calculate segment widths - each segment represents 25% of the full width.
    // last_segment_width absorbs any remainder from integer division so the
    // rightmost colored fill always reaches exactly main_bar_width at 100%.
    int segment_width = full_width / 4;
    int last_segment_width = full_width - segment_width * 3;
    int segment_75_100_width = 0;
    int segment_50_75_width = 0;
    int segment_25_50_width = 0;
    int segment_0_25_width = 0;
    
    // Calculate segment widths based on battery percentage within their ranges
    // Segments fill proportionally within their 25% range and are clamped by main bar width
    // Lower segments are always drawn at full width when battery is above their range
    if (main_bar_width > 0) {
        // 75-100% segment: fills proportionally from 75% to 100%.
        // Uses last_segment_width (not segment_width) so it absorbs any
        // integer-division remainder and always reaches the right edge at 100%.
        if (battery_percent > 75) {
            int segment_start_x = segment_width * 3;
            segment_75_100_width = (last_segment_width * (battery_percent - 75)) / 25;
            // Clamp to last_segment_width and ensure it doesn't extend beyond main bar
            if (segment_75_100_width > last_segment_width) {
                segment_75_100_width = last_segment_width;
            }
            if (segment_start_x + segment_75_100_width > main_bar_width) {
                segment_75_100_width = (main_bar_width > segment_start_x) ? (main_bar_width - segment_start_x) : 0;
            }
        }
        
        // 50-75% segment: fills proportionally from 50% to 75%, or full width if battery > 75%
        if (battery_percent > 50) {
            int segment_start_x = segment_width * 2;
            if (battery_percent > 75) {
                // Battery is above this range, draw at full width
                segment_50_75_width = segment_width;
            } else {
                // Battery is in this range, fill proportionally
                segment_50_75_width = (segment_width * (battery_percent - 50)) / 25;
            }
            // Clamp to segment width and ensure it doesn't extend beyond main bar
            if (segment_50_75_width > segment_width) {
                segment_50_75_width = segment_width;
            }
            if (segment_start_x + segment_50_75_width > main_bar_width) {
                segment_50_75_width = (main_bar_width > segment_start_x) ? (main_bar_width - segment_start_x) : 0;
            }
        }
        
        // 25-50% segment: fills proportionally from 25% to 50%, or full width if battery > 50%
        if (battery_percent > 25) {
            int segment_start_x = segment_width;
            if (battery_percent > 50) {
                // Battery is above this range, draw at full width
                segment_25_50_width = segment_width;
            } else {
                // Battery is in this range, fill proportionally
                segment_25_50_width = (segment_width * (battery_percent - 25)) / 25;
            }
            // Clamp to segment width and ensure it doesn't extend beyond main bar
            if (segment_25_50_width > segment_width) {
                segment_25_50_width = segment_width;
            }
            if (segment_start_x + segment_25_50_width > main_bar_width) {
                segment_25_50_width = (main_bar_width > segment_start_x) ? (main_bar_width - segment_start_x) : 0;
            }
        }
        
        // 0-25% segment: fills proportionally from 0% to 25%, or full width if battery > 25%
        if (battery_percent > 0) {
            int segment_start_x = 0;
            if (battery_percent > 25) {
                // Battery is above this range, draw at full width
                segment_0_25_width = segment_width;
            } else {
                // Battery is in this range, fill proportionally
                segment_0_25_width = (segment_width * battery_percent) / 25;
            }
            // Clamp to segment width and ensure it doesn't extend beyond main bar
            if (segment_0_25_width > segment_width) {
                segment_0_25_width = segment_width;
            }
            if (segment_start_x + segment_0_25_width > main_bar_width) {
                segment_0_25_width = (main_bar_width > segment_start_x) ? (main_bar_width - segment_start_x) : 0;
            }
        }
    }
    
    // Draw main outlined filled rectangle (shrinks from full width to 0)
    // Draw this FIRST (bottom layer) so segments appear on top
    if (main_bar_width > 0) {
        // Draw black background rectangle to emulate stroke (bottommost layer)
        // This extends WIDGET_OUTLINE_WIDTH pixels on all sides
        GRect stroke_background = GRect(
            base_x - WIDGET_OUTLINE_WIDTH,
            base_y - WIDGET_OUTLINE_WIDTH,
            main_bar_width + (WIDGET_OUTLINE_WIDTH * 2),
            bar_height + (WIDGET_OUTLINE_WIDTH * 2)
        );
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, stroke_background, 0, GCornerNone);
        
        // Draw segment rectangles (filled, no outline) on top of black background
        // Colors from highest to lowest: Yellow, ChromeYellow, Orange, Red
        // 75-100% segment (rightmost quarter) - Yellow
        if (segment_75_100_width > 0) {
            GRect seg_75_100 = GRect(
                base_x + segment_width * 3,
                base_y,
                segment_75_100_width,
                bar_height
            );
            graphics_context_set_fill_color(ctx, GColorYellow);
            graphics_fill_rect(ctx, seg_75_100, 0, GCornerNone);
        }
        
        // 50-75% segment (third quarter) - ChromeYellow
        if (segment_50_75_width > 0) {
            GRect seg_50_75 = GRect(
                base_x + segment_width * 2,
                base_y,
                segment_50_75_width,
                bar_height
            );
            graphics_context_set_fill_color(ctx, GColorChromeYellow);
            graphics_fill_rect(ctx, seg_50_75, 0, GCornerNone);
        }
        
        // 25-50% segment (second quarter) - Orange
        if (segment_25_50_width > 0) {
            GRect seg_25_50 = GRect(
                base_x + segment_width,
                base_y,
                segment_25_50_width,
                bar_height
            );
            graphics_context_set_fill_color(ctx, GColorOrange);
            graphics_fill_rect(ctx, seg_25_50, 0, GCornerNone);
        }
        
        // 0-25% segment (leftmost quarter) - Red
        if (segment_0_25_width > 0) {
            GRect seg_0_25 = GRect(
                base_x,
                base_y,
                segment_0_25_width,
                bar_height
            );
            graphics_context_set_fill_color(ctx, GColorRed);
            graphics_fill_rect(ctx, seg_0_25, 0, GCornerNone);
        }
    } else {
        // Draw empty outline if battery is at 0% using fill_rect
        GRect empty_stroke = GRect(
            base_x - WIDGET_OUTLINE_WIDTH,
            base_y - WIDGET_OUTLINE_WIDTH,
            WIDGET_OUTLINE_WIDTH * 2,
            bar_height + (WIDGET_OUTLINE_WIDTH * 2)
        );
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, empty_stroke, 0, GCornerNone);
    }
}

void widgets_draw_date(GContext *ctx) {
    const Layout *layout = get_layout();
    
    // Format date as mm.dd
    static char date_buffer[6];  // "mm.dd\0"
#ifdef DEMO_MODE
    if (s_demo_mode) {
        // Demo: 02.12 (February 12th)
        snprintf(date_buffer, sizeof(date_buffer), "02.12");
    } else {
        // Get current time
        time_t temp = time(NULL);
        struct tm *tick_time = localtime(&temp);
        snprintf(date_buffer, sizeof(date_buffer), "%02d.%02d", 
                 tick_time->tm_mon + 1, tick_time->tm_mday);
    }
#else
    // Get current time
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    snprintf(date_buffer, sizeof(date_buffer), "%02d.%02d", 
             tick_time->tm_mon + 1, tick_time->tm_mday);
#endif
    
    // Get font
    GFont font = fonts_get_system_font(DATE_TEXT_FONT);
    
    // On round: center the text horizontally around date_position (bottom-center).
    // Otherwise: right-align at top-right.
    GTextAlignment date_align = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight);
    int date_rect_x = PBL_IF_ROUND_ELSE(layout->date_position.x - 25, layout->date_position.x - 40);
    graphics_context_set_text_color(ctx, GColorDarkGreen);
    graphics_draw_text(ctx, date_buffer, font,
                       GRect(date_rect_x, layout->date_position.y, 50, 20),
                       GTextOverflowModeTrailingEllipsis,
                       date_align,
                       NULL);
}

void widgets_draw_day_of_week(GContext *ctx) {
    const Layout *layout = get_layout();
    
    // Get day of week (0 = Sunday, 6 = Saturday)
    int day_of_week;
#ifdef DEMO_MODE
    if (s_demo_mode) {
        // Demo: Thursday (4)
        day_of_week = 4;
    } else {
        // Get current time
        time_t temp = time(NULL);
        struct tm *tick_time = localtime(&temp);
        day_of_week = tick_time->tm_wday;
    }
#else
    // Get current time
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    day_of_week = tick_time->tm_wday;
#endif
    const char* day_text = s_day_abbreviations[day_of_week];
    
    // Draw black outer circle to emulate stroke (bottommost layer)
    // Stroke width is 2px, so outer radius is DOTW_CIRCLE_RADIUS + 2
    int outer_radius = DOTW_CIRCLE_RADIUS + WIDGET_OUTLINE_WIDTH;
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, layout->dotw_position, outer_radius);
    
    // Draw green filled circle on top (middle layer)
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_circle(ctx, layout->dotw_position, DOTW_CIRCLE_RADIUS);
    
    // Get font for text
    GFont font = fonts_get_system_font(DOTW_TEXT_FONT);
    
    // Calculate text position to center it in the circle
    // Approximate text size for centering (2 characters, small font)
    GSize text_size = graphics_text_layout_get_content_size(day_text, font, 
                                                             GRect(0, 0, 50, 20),
                                                             GTextOverflowModeTrailingEllipsis,
                                                             GTextAlignmentCenter);
    
    GPoint text_pos = GPoint(
        layout->dotw_position.x - text_size.w / 2 - 2, //hehe magic number
        layout->date_position.y + 4
    );
    
    // Draw text centered in circle (top layer)
    graphics_context_set_text_color(ctx, GColorMidnightGreen);
    graphics_draw_text(ctx, day_text, font,
                       GRect(text_pos.x, text_pos.y, text_size.w + 4, text_size.h + 4),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
}

void widgets_draw(GContext *ctx) {
    const Layout *layout = get_layout();
    
    // Don't draw widgets if widget bar height is 0 (obstructed)
    if (layout->widget_bar_height == 0) {
        return;
    }
    
    widgets_draw_battery_bar(ctx);
    widgets_draw_date(ctx);
#ifndef PBL_ROUND
    widgets_draw_day_of_week(ctx);
#endif
}

