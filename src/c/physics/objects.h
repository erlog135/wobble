#pragma once

#include <pebble.h>

// 80px shapes corresponding to numerals 0-9
// Shapes are centered around (40, 40) for an 80px bounding box

// 0 - Star (5-pointed star)
static const GPoint shape_zero_points[] = {
    {40, 5},    // Top point
    {48, 28},   // Top-right inner
    {72, 28},   // Right point
    {54, 42},   // Bottom-right inner
    {60, 65},   // Bottom right point
    {40, 52},   // Bottom inner
    {20, 65},   // Bottom left point
    {26, 42},   // Bottom-left inner
    {8, 28},    // Left point
    {32, 28}    // Top-left inner
};
#define SHAPE_ZERO_POINT_COUNT (sizeof(shape_zero_points) / sizeof(shape_zero_points[0]))

// 1 - Vertical rectangle (tall thin rectangle)
static const GPoint shape_one_points[] = {
    {30, 0},    // Top-left
    {50, 0},    // Top-right
    {50, 80},   // Bottom-right
    {30, 80}    // Bottom-left
};
#define SHAPE_ONE_POINT_COUNT (sizeof(shape_one_points) / sizeof(shape_one_points[0]))

// 2 - Rhombus (diamond shape)
static const GPoint shape_two_points[] = {
    {40, 0},    // Top
    {80, 40},   // Right
    {40, 80},   // Bottom
    {0, 40}     // Left
};
#define SHAPE_TWO_POINT_COUNT (sizeof(shape_two_points) / sizeof(shape_two_points[0]))

// 3 - Triangle (pointing up)
static const GPoint shape_three_points[] = {
    {40, 0},    // Top point
    {0, 80},    // Bottom-left
    {80, 80}    // Bottom-right
};
#define SHAPE_THREE_POINT_COUNT (sizeof(shape_three_points) / sizeof(shape_three_points[0]))

// 4 - Square
static const GPoint shape_four_points[] = {
    {0, 0},     // Top-left
    {80, 0},    // Top-right
    {80, 80},   // Bottom-right
    {0, 80}     // Bottom-left
};
#define SHAPE_FOUR_POINT_COUNT (sizeof(shape_four_points) / sizeof(shape_four_points[0]))

// 5 - Pentagon (regular pentagon)
static const GPoint shape_five_points[] = {
    {40, 0},    // Top
    {75, 20},   // Top-right
    {65, 60},   // Bottom-right
    {15, 60},   // Bottom-left
    {5, 20}     // Top-left
};
#define SHAPE_FIVE_POINT_COUNT (sizeof(shape_five_points) / sizeof(shape_five_points[0]))

// 6 - Hexagon (regular hexagon)
static const GPoint shape_six_points[] = {
    {40, 0},    // Top
    {70, 15},   // Top-right
    {70, 65},   // Bottom-right
    {40, 80},   // Bottom
    {10, 65},   // Bottom-left
    {10, 15}    // Top-left
};
#define SHAPE_SIX_POINT_COUNT (sizeof(shape_six_points) / sizeof(shape_six_points[0]))

// 7 - Arrow pointing up
static const GPoint shape_seven_points[] = {
    {40, 0},    // Arrow tip (top)
    {25, 25},   // Left of tip
    {30, 25},   // Left shaft top
    {30, 65},   // Left shaft bottom
    {50, 65},   // Right shaft bottom
    {50, 25},   // Right shaft top
    {55, 25}    // Right of tip
};
#define SHAPE_SEVEN_POINT_COUNT (sizeof(shape_seven_points) / sizeof(shape_seven_points[0]))

// 8 - Octagon (regular octagon)
static const GPoint shape_eight_points[] = {
    {40, 0},    // Top
    {60, 10},   // Top-right
    {70, 30},   // Right-top
    {70, 50},   // Right-bottom
    {60, 70},   // Bottom-right
    {40, 80},   // Bottom
    {20, 70},   // Bottom-left
    {10, 50},   // Left-bottom
    {10, 30},   // Left-top
    {20, 10}    // Top-left
};
#define SHAPE_EIGHT_POINT_COUNT (sizeof(shape_eight_points) / sizeof(shape_eight_points[0]))

// 9 - Dart (kite/dart shape)
static const GPoint shape_nine_points[] = {
    {40, 0},    // Front point (top)
    {60, 25},   // Right front
    {70, 50},   // Right back
    {50, 80},   // Back point (bottom)
    {30, 50},   // Left back
    {20, 25}    // Left front
};
#define SHAPE_NINE_POINT_COUNT (sizeof(shape_nine_points) / sizeof(shape_nine_points[0]))
