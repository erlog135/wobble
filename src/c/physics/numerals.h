#pragma once

#include <pebble.h>

static const GPoint zero_points[] = {{25, 78}, {55, 78}, {71, 62}, {71, 17}, {56, 2}, {44, 2}, {44, 17}, {48, 17}, {55, 24}, {55, 55}, {48, 62}, {32, 62}, {25, 55}, {25, 24}, {32, 17}, {36, 17}, {36, 2}, {24, 2}, {9, 17}, {9, 62}};
#define ZERO_POINT_COUNT (sizeof(zero_points) / sizeof(zero_points[0]))
static const GPoint one_points[] = {{19, 78}, {65, 78}, {65, 62}, {50, 62}, {50, 2}, {38, 2}, {19, 17}, {19, 32}, {34, 21}, {34, 62}, {19, 62}};
#define ONE_POINT_COUNT (sizeof(one_points) / sizeof(one_points[0]))
static const GPoint two_points[] = {{10, 78}, {71, 78}, {71, 62}, {34, 62}, {71, 32}, {71, 14}, {59, 2}, {21, 2}, {10, 13}, {10, 22}, {25, 22}, {29, 18}, {48, 18}, {55, 25}, {10, 62}};
#define TWO_POINT_COUNT (sizeof(two_points) / sizeof(two_points[0]))
static const GPoint three_points[] = {{10, 58}, {26, 58}, {30, 62}, {48, 62}, {55, 55}, {48, 48}, {33, 48}, {33, 32}, {48, 32}, {55, 25}, {48, 18}, {29, 18}, {25, 22}, {10, 22}, {10, 13}, {21, 2}, {59, 2}, {71, 14}, {71, 33}, {64, 40}, {71, 47}, {71, 66}, {59, 78}, {22, 78}, {10, 67}};
#define THREE_POINT_COUNT (sizeof(three_points) / sizeof(three_points[0]))
static const GPoint four_points[] = {{47, 78}, {62, 78}, {62, 55}, {70, 55}, {70, 40}, {62, 40}, {62, 2}, {47, 2}, {47, 40}, {24, 40}, {39, 17}, {39, 2}, {31, 2}, {9, 40}, {17, 55}, {47, 55}};
#define FOUR_POINT_COUNT (sizeof(four_points) / sizeof(four_points[0]))
static const GPoint five_points[] = {{11, 2}, {71, 2}, {71, 17}, {26, 17}, {26, 32}, {30, 28}, {60, 28}, {71, 39}, {71, 67}, {60, 78}, {22, 78}, {11, 67}, {11, 58}, {26, 58}, {30, 62}, {52, 62}, {56, 58}, {56, 48}, {52, 44}, {30, 44}, {26, 48}, {11, 48}};
#define FIVE_POINT_COUNT (sizeof(five_points) / sizeof(five_points[0]))
static const GPoint six_points[] = {{10, 62}, {26, 78}, {37, 78}, {37, 62}, {32, 62}, {25, 55}, {32, 48}, {48, 48}, {55, 55}, {48, 62}, {43, 62}, {43, 78}, {55, 78}, {71, 62}, {71, 47}, {56, 32}, {25, 32}, {25, 25}, {32, 18}, {49, 18}, {56, 25}, {71, 25}, {71, 17}, {56, 2}, {25, 2}, {10, 17}};
#define SIX_POINT_COUNT (sizeof(six_points) / sizeof(six_points[0]))
static const GPoint seven_points[] = {{70, 2}, {10, 2}, {10, 18}, {51, 18}, {30, 50}, {30, 78}, {49, 78}, {49, 50}, {70, 18}};
#define SEVEN_POINT_COUNT (sizeof(seven_points) / sizeof(seven_points[0]))
static const GPoint eight_points[] = {{10, 62}, {26, 78}, {37, 78}, {37, 62}, {32, 62}, {25, 55}, {32, 48}, {49, 48}, {56, 55}, {49, 62}, {44, 62}, {44, 78}, {55, 78}, {71, 62}, {71, 47}, {64, 40}, {71, 33}, {71, 17}, {56, 2}, {44, 2}, {44, 18}, {49, 18}, {56, 25}, {49, 32}, {32, 32}, {25, 25}, {32, 18}, {37, 18}, {37, 2}, {25, 2}, {10, 17}, {10, 33}, {17, 40}, {10, 47}};
#define EIGHT_POINT_COUNT (sizeof(eight_points) / sizeof(eight_points[0]))
static const GPoint nine_points[] = {{71, 18}, {55, 2}, {44, 2}, {44, 18}, {49, 18}, {56, 25}, {49, 32}, {33, 32}, {26, 25}, {33, 18}, {38, 18}, {38, 2}, {26, 2}, {10, 18}, {10, 33}, {25, 48}, {56, 48}, {56, 55}, {49, 62}, {32, 62}, {25, 55}, {10, 55}, {10, 63}, {25, 78}, {56, 78}, {71, 63}};
#define NINE_POINT_COUNT (sizeof(nine_points) / sizeof(nine_points[0]))