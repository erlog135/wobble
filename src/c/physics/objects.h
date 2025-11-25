#pragma once

#include <pebble.h>

// Example object definitions as lists of GPoints
// These define the initial shape/positions of soft body objects

// Example: Square shape
static const GPoint square_points[] = {
    {50, 50},
    {100, 50},
    {100, 100},
    {50, 100}
};
#define SQUARE_POINT_COUNT (sizeof(square_points) / sizeof(square_points[0]))

// Example: Triangle shape
static const GPoint triangle_points[] = {
    {75, 30},
    {50, 80},
    {100, 80}
};
#define TRIANGLE_POINT_COUNT (sizeof(triangle_points) / sizeof(triangle_points[0]))

// Example: Hexagon shape
static const GPoint hexagon_points[] = {
    {75, 20},   // Top
    {95, 35},   // Top-right
    {95, 65},   // Bottom-right
    {75, 80},   // Bottom
    {55, 65},   // Bottom-left
    {55, 35}    // Top-left
};
#define HEXAGON_POINT_COUNT (sizeof(hexagon_points) / sizeof(hexagon_points[0]))

// Example: Custom shape (blob-like)
// 80px tall number '2' as an outline polygon (clockwise)
// Note: Coordinates chosen to look like a '2' digit, top left at (50,20), height ~80px.
static const GPoint blob_points[] = {
    {60, 20},   // Top left curve
    {90, 20},   // Top right
    {95, 30},   // Curve right
    {90, 45},   // Upper belly
    {65, 55},   // Middle transition
    {60, 70},   // Bottom left knee
    {65, 80},   // Bottom
    {90, 80},   // Bottom right
    {95, 75},   // Bottom right curve
    {93, 70},   // Curl up into base
    {70, 60},   // Inside base curve
    {90, 50},   // Pinch middle
    {95, 40},   // Top tip
    {95, 30},   // Top right
    {60, 20}    // Closing point (matches first for polygon)
};
#define BLOB_POINT_COUNT (sizeof(blob_points) / sizeof(blob_points[0]))

