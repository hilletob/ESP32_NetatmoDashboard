#ifndef LAYOUT_H
#define LAYOUT_H

// Screen dimensions (800x480 landscape)
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// General layout constants
#define MARGIN 8
#define CARD_SPACING 8
#define COLUMN_GAP 8

// Header section
#define HEADER_HEIGHT 45
#define HEADER_Y 0

// Three-column card grid (3 × 256px)
#define CARD_WIDTH 256
#define CARD_HEIGHT 95  // Default card height

// Individual card heights
#define TEMP_CARD_HEIGHT 115       // Temperature cards (with min/max times)
#define HUMID_CARD_HEIGHT 95       // Humidity cards
#define AIR_QUALITY_CARD_HEIGHT 80 // CO2 card (no quality label)
#define PRESSURE_CARD_HEIGHT 80    // Pressure card (no trend label)
#define SUN_CARD_HEIGHT 85         // Sun widget (sunrise/sunset)
#define MOON_CARD_HEIGHT 82        // Moon widget (moonrise/moonset/phase)

// Column positions
// Math check: 8 + 256 + 8 + 256 + 8 + 256 + 8 = 800 ✓
#define COL1_X MARGIN                           // 8
#define COL2_X (COL1_X + CARD_WIDTH + COLUMN_GAP)  // 272
#define COL3_X (COL2_X + CARD_WIDTH + COLUMN_GAP)  // 536

// Forecast column (tall widget for 3-day forecast)
#define FORECAST_COL_WIDTH CARD_WIDTH
#define FORECAST_COL_HEIGHT 422  // SCREEN_HEIGHT - HEADER_HEIGHT - MARGIN - STATUS_BAR_HEIGHT - MARGIN

// Forecast column internal spacing (uses same gap as columns 1 & 2)
#define FORECAST_SECTION_GAP 8           // Gap between day sections (matches CARD_SPACING)
#define FORECAST_HEUTE_HEIGHT 128        // HEUTE section height (reduced for tighter fit)
#define FORECAST_MORGEN_HEIGHT 127       // MORGEN section height
#define FORECAST_UEBERMORGEN_HEIGHT 127  // UEBERMORGEN section height
// Math: 128 + 8 + 127 + 8 + 127 = 398px (24px spacing at bottom)

// Card vertical positions - calculated per column
// Column 1 & 2 rows (adjusted for different card heights)
#define ROW1_Y (HEADER_HEIGHT + MARGIN)                              // 53
#define ROW2_Y (ROW1_Y + TEMP_CARD_HEIGHT + CARD_SPACING)           // 176 (53+115+8)
#define ROW3_Y (ROW2_Y + HUMID_CARD_HEIGHT + CARD_SPACING)          // 279 (176+95+8)
#define ROW4_Y (ROW3_Y + AIR_QUALITY_CARD_HEIGHT + CARD_SPACING)    // 367 (279+80+8)

// Card internal layout (adjusted for narrower 256px cards)
#define CARD_PADDING 6
#define CARD_LABEL_Y 8           // Label position from card top
#define CARD_VALUE_Y 28          // Large value position
#define CARD_TREND_X_OFFSET 200  // Trend icon X offset (adjusted from 340 for 256px width)
#define CARD_TREND_Y 30          // Trend icon Y position
#define CARD_DETAIL_Y 75         // Detail text position (min/max, etc)

// ============================================================================
// COLUMN 1: INDOOR SENSORS (Netatmo Base Station)
// ============================================================================
#define INDOOR_TEMP_X COL1_X
#define INDOOR_TEMP_Y ROW1_Y

#define INDOOR_HUMID_X COL1_X
#define INDOOR_HUMID_Y ROW2_Y

#define AIR_QUALITY_X COL1_X
#define AIR_QUALITY_Y ROW3_Y

#define PRESSURE_X COL1_X
#define PRESSURE_Y ROW4_Y

// ============================================================================
// COLUMN 2: OUTDOOR SENSORS + SUN/MOON WIDGETS
// ============================================================================
#define OUTDOOR_TEMP_X COL2_X
#define OUTDOOR_TEMP_Y ROW1_Y

#define OUTDOOR_HUMID_X COL2_X
#define OUTDOOR_HUMID_Y ROW2_Y

// Sun/Moon widgets (replaces old Wind/Rain widgets)
#define SUN_X COL2_X
#define SUN_Y ROW3_Y

#define MOON_X COL2_X
#define MOON_Y ROW4_Y

// Legacy positions (deprecated, kept for compatibility)
#define WIND_X COL2_X
#define WIND_Y ROW3_Y

#define RAIN_X COL2_X
#define RAIN_Y ROW4_Y

// ============================================================================
// COLUMN 3: 3-DAY FORECAST (met.no API)
// ============================================================================
#define FORECAST_COL_X COL3_X
#define FORECAST_COL_Y ROW1_Y

// Status bar at bottom (thin strip)
#define STATUS_BAR_Y 465
#define STATUS_BAR_HEIGHT 15

#endif  // LAYOUT_H
