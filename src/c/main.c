#include <pebble.h>

// === VAPORWAVE COLORS ===
#define C_SKY0     GColorOxfordBlue
#define C_SKY1     GColorImperialPurple
#define C_SKY2     ((GColor){.argb=0b11100010})   // medium purple
#define C_SKY3     ((GColor){.argb=0b11100011})   // vivid violet
#define C_SKY4     GColorMagenta
#define C_SKY5     GColorShockingPink
#define C_SUN_FILL GColorChromeYellow
#define C_SUN_LINE GColorOxfordBlue
#define C_HORIZON  GColorShockingPink
#define C_ROAD     GColorOxfordBlue
#define C_GRID_V   GColorCyan
#define C_GRID_H1  GColorCyan
#define C_GRID_H2  GColorMagenta
#define C_PALM_T   GColorDarkCandyAppleRed
#define C_PALM_L   GColorImperialPurple
#define C_BAR_BG   GColorOxfordBlue
#define C_BAR_OVR  GColorCyan
#define C_BAR_OVRG GColorTiffanyBlue
#define C_BAR_DAY  GColorMagenta
#define C_BAR_DAYG ((GColor){.argb=0b11100010})
#define C_TXT_OVR  GColorCyan
#define C_TXT_DAY  GColorShockingPink
#define C_TXT_WHT  GColorWhite
#define C_BATT_G   GColorIslamicGreen
#define C_BATT_M   GColorChromeYellow
#define C_BATT_L   GColorRed
#define C_BAR_BG_SOLID  GColorOxfordBlue
#define C_BAR_BG_SOLID2 GColorImperialPurple

// GColor from raw argb byte (used for theme color tables)
#define GCA(v) ((GColor){.argb = (v)})

typedef struct {
    uint8_t sky[7];
    uint8_t road;
    uint8_t grid_v;
    uint8_t grid_h1;
    uint8_t grid_h2;
    uint8_t horizon;
    uint8_t sun_fill;
    uint8_t sun_line;
    uint8_t landmark1;        // primary landmark color (trunk / rock / building / stem)
    uint8_t landmark2;        // secondary landmark color (foliage / snow / windows / flower)
    uint8_t bar_ovr;
    uint8_t bar_ovr_glow;
    uint8_t bar_day;
    uint8_t bar_day_glow;
    uint8_t bar_bg;
    uint8_t txt_ovr;          // bar label + weather left text
    uint8_t txt_day;          // bar label + weather right text
    uint8_t info1_bg;
    uint8_t info2_bg;
    uint8_t info1_border;
    uint8_t info2_border;
    uint8_t txt_time;         // clock TextLayer color
    uint8_t txt_leg;          // leg-label TextLayer color
    uint8_t txt_time_shadow;  // outline/shadow for clock (0 = none)
} ThemeColors;

// argb bit format: 11_RR_GG_BB (2 bits each, fully opaque)
// 7 themes: Vaporwave, Beach, Mountain, Winter, City, Plains, Desert
static const ThemeColors s_themes[7] = {
  { /* 0: Vaporwave — dark navy sky, neon cyan/magenta */
    .sky={0b11000001,0b11010001,0b11100010,0b11100011,0b11110011,0b11110111,0b11110111},
    .road=0b11000001,.grid_v=0b11001111,.grid_h1=0b11001111,.grid_h2=0b11110011,
    .horizon=0b11110111,.sun_fill=0b11111000,.sun_line=0b11000001,
    .landmark1=0b11100000,.landmark2=0b11010001,           // palms: red trunk, purple leaves
    .bar_ovr=0b11001111,.bar_ovr_glow=0b11001110,
    .bar_day=0b11110011,.bar_day_glow=0b11100010,
    .bar_bg=0b11000001,.txt_ovr=0b11001111,.txt_day=0b11110111,
    .info1_bg=0b11000001,.info2_bg=0b11010001,
    .info1_border=0b11001111,.info2_border=0b11110011,
    .txt_time=0b11111111,.txt_leg=0b11001111,.txt_time_shadow=0
  },
  { /* 1: Beach — ocean blue sky, warm sandy road, coral accents */
    .sky={0b11000011,0b11001011,0b11001111,0b11001110,0b11111001,0b11111000,0b11111000},
    .road=0b11111001,.grid_v=0b11001111,.grid_h1=0b11111111,.grid_h2=0b11110101,
    .horizon=0b11111000,.sun_fill=0b11111100,.sun_line=0b11111000,
    .landmark1=0b11100100,.landmark2=0b11001100,           // palms: tan trunk, green leaves
    .bar_ovr=0b11001111,.bar_ovr_glow=0b11001011,
    .bar_day=0b11110101,.bar_day_glow=0b11111000,
    .bar_bg=0b11000101,.txt_ovr=0b11111111,.txt_day=0b11001111, // white+cyan on dark bar_bg
    .info1_bg=0b11000101,.info2_bg=0b11000111,
    .info1_border=0b11001111,.info2_border=0b11110101,
    .txt_time=0b11000000,.txt_leg=0b11111111,.txt_time_shadow=0  // black time on bright sky
  },
  { /* 2: Mountain — dark purple/indigo sky, gray road, gold sunrise */
    .sky={0b11000001,0b11010010,0b11100111,0b11110111,0b11111001,0b11111000,0b11111000},
    .road=0b11010101,.grid_v=0b11111111,.grid_h1=0b11111111,.grid_h2=0b11101010,
    .horizon=0b11111000,.sun_fill=0b11111000,.sun_line=0b11100000,
    .landmark1=0b11101010,.landmark2=0b11111111,           // peaks: gray rock, white snowcap
    .bar_ovr=0b11111000,.bar_ovr_glow=0b11111001,
    .bar_day=0b11001100,.bar_day_glow=0b11001010,
    .bar_bg=0b11010101,.txt_ovr=0b11111000,.txt_day=0b11001110,
    .info1_bg=0b11010101,.info2_bg=0b11000100,
    .info1_border=0b11111000,.info2_border=0b11001100,
    .txt_time=0b11111111,.txt_leg=0b11111111,.txt_time_shadow=0  // white on dark purple sky
  },
  { /* 3: Winter — icy blue-to-white sky, gray road, evergreen trees */
    .sky={0b11000001,0b11000010,0b11000111,0b11010111,0b11101010,0b11111110,0b11111111},
    .road=0b11010101,.grid_v=0b11111111,.grid_h1=0b11111111,.grid_h2=0b11001111,
    .horizon=0b11111110,.sun_fill=0b11111110,.sun_line=0b11101010,
    .landmark1=0b11000100,.landmark2=0b11111111,           // evergreens: dark green, white snow
    .bar_ovr=0b11001111,.bar_ovr_glow=0b11001011,
    .bar_day=0b11000011,.bar_day_glow=0b11001111,
    .bar_bg=0b11010101,.txt_ovr=0b11001111,.txt_day=0b11111110,
    .info1_bg=0b11000001,.info2_bg=0b11010101,
    .info1_border=0b11001111,.info2_border=0b11010111,
    .txt_time=0b11111111,.txt_leg=0b11001111,.txt_time_shadow=0b11000000  // white+black outline
  },
  { /* 4: City — very dark sky with amber glow, skyscrapers */
    .sky={0b11000000,0b11010000,0b11100001,0b11100100,0b11110100,0b11111000,0b11111001},
    .road=0b11010101,.grid_v=0b11111000,.grid_h1=0b11111000,.grid_h2=0b11110100,
    .horizon=0b11111001,.sun_fill=0b11111001,.sun_line=0b11100000,
    .landmark1=0b11010101,.landmark2=0b11111000,           // buildings: gray silhouette, yellow lit windows
    .bar_ovr=0b11111000,.bar_ovr_glow=0b11111001,
    .bar_day=0b11110100,.bar_day_glow=0b11111000,
    .bar_bg=0b11000000,.txt_ovr=0b11111000,.txt_day=0b11110100,
    .info1_bg=0b11000000,.info2_bg=0b11010101,
    .info1_border=0b11111000,.info2_border=0b11110100,
    .txt_time=0b11111111,.txt_leg=0b11111000,.txt_time_shadow=0  // white on very dark sky
  },
  { /* 5: Plains — big blue sky, brass road, wildflowers */
    .sky={0b11000011,0b11001011,0b11001111,0b11011011,0b11111111,0b11111101,0b11111000},
    .road=0b11101001,.grid_v=0b11111100,.grid_h1=0b11111100,.grid_h2=0b11101000,
    .horizon=0b11111000,.sun_fill=0b11111100,.sun_line=0b11111000,
    .landmark1=0b11001000,.landmark2=0b11111100,           // flowers: green stems, yellow blooms
    .bar_ovr=0b11111111,.bar_ovr_glow=0b11111000,          // white bar on dark navy bar_bg
    .bar_day=0b11001111,.bar_day_glow=0b11101000,
    .bar_bg=0b11000001,.txt_ovr=0b11111111,.txt_day=0b11001111, // white+cyan on dark navy bg
    .info1_bg=0b11000001,.info2_bg=0b11000001,
    .info1_border=0b11111100,.info2_border=0b11001100,
    .txt_time=0b11000001,.txt_leg=0b11111111,.txt_time_shadow=0  // oxford blue on bright sky
  },
  { /* 6: Desert — hot cerulean/pastel sky, sandy road, cacti */
    .sky={0b11001011,0b11001110,0b11111110,0b11111110,0b11111001,0b11111000,0b11110100},
    .road=0b11111001,.grid_v=0b11110100,.grid_h1=0b11111000,.grid_h2=0b11100100,
    .horizon=0b11110100,.sun_fill=0b11111111,.sun_line=0b11111000,
    .landmark1=0b11100100,.landmark2=0b11001000,           // cacti: tan body, army green accents
    .bar_ovr=0b11110100,.bar_ovr_glow=0b11111000,
    .bar_day=0b11101000,.bar_day_glow=0b11001100,
    .bar_bg=0b11010000,.txt_ovr=0b11111111,.txt_day=0b11001111, // white+cyan on dark red bar_bg
    .info1_bg=0b11010000,.info2_bg=0b11010000,
    .info1_border=0b11110100,.info2_border=0b11111000,
    .txt_time=0b11000001,.txt_leg=0b11111111,.txt_time_shadow=0  // oxford blue on bright sandy sky
  }
};

// === LAYOUT ===
#define HORIZON_Y  97
#define BAR_X      10
#define BAR_W      180
#define BAR_H      12

// === PERSIST ===
#define PERSIST_KEY_A  1
#define PERSIST_KEY_B  2

// === SETTINGS STRUCTS ===
// sizeof(TripSettingsA) ≈ 116 bytes — safe under 256
typedef struct {
    char    trip_name[24];
    char    start_name[20];
    char    dest_name[20];
    int32_t total_distance;
    int32_t day_distances[5];
    int32_t current_distance;
    int8_t  progress_unit;   // 0=mi, 1=km, 2=%
    int8_t  trip_day;        // 1–6
    bool    show_weather;
    bool    temp_unit_f;
    int8_t  theme;        // 0=Vaporwave 1=Beach 2=Mountain 3=Winter 4=City 5=Plains 6=Desert
} TripSettingsA;

// sizeof(TripSettingsB) ≈ 104 bytes
typedef struct {
    char   waypoints[5][20];
    int8_t cached_temp_cur;   // current loc temp, °C (−99 = no data)
    int8_t cached_cond_cur;   // 0=unknown, 1=clear, 2=cloudy, 3=rain, 4=snow, 5=thunder
    int8_t cached_temp_dest;  // destination temp, °C
    int8_t cached_cond_dest;
} TripSettingsB;

// === GLOBALS ===
static Window    *s_main_window;
static Layer     *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_leg_layer;

static AppTimer *s_anim_timer = NULL;
static int s_drive_frame  = 0;
static int s_drive_offset = 0;

static TripSettingsA s_sa;
static TripSettingsB s_sb;
static int s_battery_level = 100;

static const ThemeColors *prv_theme(void) {
    int t = (int)s_sa.theme;
    if (t < 0 || t >= 7) t = 0;
    return &s_themes[t];
}

static void update_text_colors(void) {
    if (!s_time_layer || !s_leg_layer) return;
    const ThemeColors *tc = prv_theme();
    text_layer_set_text_color(s_time_layer, GCA(tc->txt_time));
    text_layer_set_text_color(s_leg_layer,  GCA(tc->txt_leg));
}

static char s_time_buf[8];
static char s_leg_buf[24];
static char s_ovr_pct_buf[8];
static char s_day_pct_buf[8];
static char s_day_label_buf[10];
static char s_weather_cur_buf[22];  // "22°C  Clear"
static char s_weather_dst_buf[22];  // "18°C  Rain"
static char s_route_buf[44];

// === PERSIST ===
static void prv_default_settings(void) {
    memset(&s_sa, 0, sizeof(s_sa));
    memset(&s_sb, 0, sizeof(s_sb));
    strncpy(s_sa.trip_name,  "Road Trip",    sizeof(s_sa.trip_name)-1);
    strncpy(s_sa.start_name, "Start",        sizeof(s_sa.start_name)-1);
    strncpy(s_sa.dest_name,  "Destination",  sizeof(s_sa.dest_name)-1);
    s_sa.total_distance   = 1200;
    s_sa.current_distance = 0;
    s_sa.day_distances[0] = 280;
    s_sa.day_distances[1] = 240;
    s_sa.day_distances[2] = 240;
    s_sa.day_distances[3] = 240;
    s_sa.day_distances[4] = 200;
    s_sa.progress_unit    = 0;
    s_sa.trip_day         = 1;
    s_sa.show_weather     = true;
    s_sa.temp_unit_f      = false;
    s_sa.theme            = 0;
    s_sb.cached_temp_cur  = -99;
    s_sb.cached_temp_dest = -99;
    strncpy(s_sb.waypoints[0], "Stop 1", sizeof(s_sb.waypoints[0])-1);
    strncpy(s_sb.waypoints[1], "Stop 2", sizeof(s_sb.waypoints[1])-1);
}

static void prv_load_settings(void) {
    prv_default_settings();
    if (persist_exists(PERSIST_KEY_A)) persist_read_data(PERSIST_KEY_A, &s_sa, sizeof(s_sa));
    if (persist_exists(PERSIST_KEY_B)) persist_read_data(PERSIST_KEY_B, &s_sb, sizeof(s_sb));
    // Recover from corrupted persist: total_distance=0 causes permanent 0% progress
    if (s_sa.total_distance <= 0) s_sa.total_distance = 1200;
}

static void prv_save_settings(void) {
    persist_write_data(PERSIST_KEY_A, &s_sa, sizeof(s_sa));
    persist_write_data(PERSIST_KEY_B, &s_sb, sizeof(s_sb));
}

// === DRAW HELPERS ===

static void draw_sky(GContext *ctx, int w, const ThemeColors *tc) {
    static const uint8_t band_y[] = {14, 28, 44, 60, 76, 88, 97};
    int prev = 0;
    for (int i = 0; i < 7; i++) {
        graphics_context_set_fill_color(ctx, GCA(tc->sky[i]));
        graphics_fill_rect(ctx, GRect(0, prev, w, band_y[i] - prev), 0, GCornerNone);
        prev = band_y[i];
    }
}

static void draw_sun(GContext *ctx, const ThemeColors *tc) {
    int cx = 100, cy = 47, r = 28;
    graphics_context_set_fill_color(ctx, GCA(tc->sun_fill));
    graphics_fill_circle(ctx, GPoint(cx, cy), r);
    static const uint8_t hw[] = {28,27,27,26,25,23,21,18,14,7};
    for (int i = 0; i < 10; i++) {
        int sy = cy + i * 3;
        if (sy < cy) continue;
        if (sy > cy + r) break;
        if (i % 2 == 0) {
            graphics_context_set_fill_color(ctx, GCA(tc->sun_line));
            graphics_fill_rect(ctx, GRect(cx - hw[i], sy, hw[i] * 2, 2), 0, GCornerNone);
        }
    }
}

static void draw_battery(GContext *ctx, int w) {
    GColor bc = (s_battery_level <= 20) ? C_BATT_L
              : (s_battery_level <= 40) ? C_BATT_M : C_BATT_G;
    graphics_context_set_fill_color(ctx, bc);
    graphics_fill_rect(ctx, GRect(0, 0, (s_battery_level * w) / 100, 3), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    int bw = (s_battery_level * w) / 100;
    graphics_fill_rect(ctx, GRect(bw, 0, w - bw, 3), 0, GCornerNone);
}

static void draw_grid(GContext *ctx, int w, int h, int offset, const ThemeColors *tc) {
    int vx = w / 2, vy = HORIZON_Y;
    int road_h = h - vy;  // 228 - 97 = 131

    // Converging perspective lines
    graphics_context_set_stroke_color(ctx, GCA(tc->grid_v));
    for (int i = -5; i <= 5; i++) {
        graphics_draw_line(ctx, GPoint(vx, vy), GPoint(vx + i * 22, h));
    }

    // Scrolling horizontal lines
    static const uint8_t base_dy[] = {8, 16, 26, 38, 53, 71, 92, 116};
    for (int i = 0; i < 8; i++) {
        int scroll = (offset * (i + 2)) % road_h;
        int dy = (base_dy[i] + scroll) % road_h;
        if (dy == 0) dy = 1;
        int y = vy + dy;
        if (y >= h) continue;
        int spread = (dy * vx) / road_h;
        graphics_context_set_stroke_color(ctx, (i % 2 == 0) ? GCA(tc->grid_h1) : GCA(tc->grid_h2));
        graphics_draw_line(ctx, GPoint(vx - spread, y), GPoint(vx + spread, y));
    }
}

static void draw_palm(GContext *ctx, int bx, int by, int trunk_h, bool flip,
                      const ThemeColors *tc) {
    graphics_context_set_fill_color(ctx, GCA(tc->landmark1));
    for (int i = 0; i < trunk_h; i++) {
        int lean = flip ? -(i / 6) : (i / 6);
        graphics_fill_rect(ctx, GRect(bx + lean - 1, by - i, 3, 1), 0, GCornerNone);
    }
    int tx = bx + (flip ? -(trunk_h / 6) : (trunk_h / 6));
    int ty = by - trunk_h;
    graphics_context_set_fill_color(ctx, GCA(tc->landmark2));
    graphics_fill_circle(ctx, GPoint(tx,     ty - 5), 6);
    graphics_fill_circle(ctx, GPoint(tx - 8, ty - 1), 5);
    graphics_fill_circle(ctx, GPoint(tx + 8, ty - 1), 5);
    graphics_fill_circle(ctx, GPoint(tx - 5, ty + 3), 4);
    graphics_fill_circle(ctx, GPoint(tx + 5, ty + 3), 4);
}

// Draw a triangular mountain peak; rock drawn first, then snowcap overdrawn
static void draw_mountain_peak(GContext *ctx, int px, int py, int base_half,
                                GColor rock, GColor snow) {
    int h = HORIZON_Y - py;
    int snow_h = h / 3;
    graphics_context_set_stroke_color(ctx, rock);
    for (int dy = snow_h; dy <= h; dy++) {
        int hw = (dy * base_half) / h;
        graphics_draw_line(ctx, GPoint(px - hw, py + dy), GPoint(px + hw, py + dy));
    }
    graphics_context_set_stroke_color(ctx, snow);
    for (int dy = 0; dy < snow_h; dy++) {
        int hw = (dy * base_half) / h;
        graphics_draw_line(ctx, GPoint(px - hw, py + dy), GPoint(px + hw, py + dy));
    }
}

static void draw_mountain_peaks(GContext *ctx, const ThemeColors *tc) {
    GColor rock = GCA(tc->landmark1), snow = GCA(tc->landmark2);
    draw_mountain_peak(ctx,  22, 62, 32, rock, snow);   // left foreground
    draw_mountain_peak(ctx,  -4, 72, 20, rock, snow);   // left background
    draw_mountain_peak(ctx, 178, 62, 32, rock, snow);   // right foreground
    draw_mountain_peak(ctx, 204, 72, 20, rock, snow);   // right background
}

// Pine/evergreen: triangular body with snow on upper third
static void draw_evergreen(GContext *ctx, int bx, int by, int h,
                            const ThemeColors *tc) {
    int max_hw = h / 3;
    graphics_context_set_stroke_color(ctx, GCA(tc->landmark1));
    for (int dy = 0; dy < h; dy++) {
        int hw = ((h - dy) * max_hw) / h;
        graphics_draw_line(ctx, GPoint(bx - hw, by - dy), GPoint(bx + hw, by - dy));
    }
    int snow_start = h * 65 / 100;
    graphics_context_set_stroke_color(ctx, GCA(tc->landmark2));
    for (int dy = snow_start; dy < h; dy++) {
        int hw = ((h - dy) * max_hw) / h;
        if (hw > 0)
            graphics_draw_line(ctx, GPoint(bx - hw, by - dy), GPoint(bx + hw, by - dy));
    }
    graphics_context_set_fill_color(ctx, GCA(tc->landmark1));
    graphics_fill_rect(ctx, GRect(bx - 2, by, 4, 4), 0, GCornerNone);
}

static void draw_evergreens(GContext *ctx, const ThemeColors *tc) {
    draw_evergreen(ctx, 14, HORIZON_Y, 36, tc);
    draw_evergreen(ctx, 30, HORIZON_Y, 26, tc);
    draw_evergreen(ctx, 182, HORIZON_Y, 36, tc);
    draw_evergreen(ctx, 192, HORIZON_Y, 26, tc);
}

// City skyscrapers with lit windows
static void draw_buildings(GContext *ctx, const ThemeColors *tc) {
    static const int8_t bldgs[][3] = {
        {2, 16, 50}, {16, 11, 62}, {26, 14, 44},
        {155, 14, 58}, {167, 16, 68}, {182, 13, 44}
    };
    for (int i = 0; i < 6; i++) {
        int bx = bldgs[i][0], bw = bldgs[i][1], bh = bldgs[i][2];
        int by = HORIZON_Y - bh;
        graphics_context_set_fill_color(ctx, GCA(tc->landmark1));
        graphics_fill_rect(ctx, GRect(bx, by, bw, bh + 1), 0, GCornerNone);
        // Lit windows in rows
        graphics_context_set_fill_color(ctx, GCA(tc->landmark2));
        for (int wy = by + 4; wy < HORIZON_Y - 4; wy += 6) {
            for (int wx = bx + 3; wx < bx + bw - 3; wx += 5) {
                graphics_fill_rect(ctx, GRect(wx, wy, 2, 2), 0, GCornerNone);
            }
        }
    }
}

// Wildflowers: stems + dot blooms along the horizon
static void draw_flowers(GContext *ctx, const ThemeColors *tc) {
    static const int8_t fx[] = {7, 17, 27, 39, 52, 149, 162, 172, 183, 193};
    graphics_context_set_stroke_color(ctx, GCA(tc->landmark1));
    for (int i = 0; i < 10; i++) {
        graphics_draw_line(ctx, GPoint(fx[i], HORIZON_Y),
                                GPoint(fx[i], HORIZON_Y - 11));
    }
    graphics_context_set_fill_color(ctx, GCA(tc->landmark2));
    for (int i = 0; i < 10; i++) {
        graphics_fill_circle(ctx, GPoint(fx[i], HORIZON_Y - 13), 3);
    }
    graphics_context_set_fill_color(ctx, GCA(tc->landmark1));
    for (int i = 0; i < 10; i++) {
        graphics_fill_circle(ctx, GPoint(fx[i], HORIZON_Y - 13), 1);
    }
}

// Dispatch the correct landmark for the active theme
static void draw_landmark(GContext *ctx, const ThemeColors *tc) {
    int t = (int)s_sa.theme;
    if (t < 0 || t >= 7) t = 0;
    switch (t) {
        case 0: case 1: case 6:   // Vaporwave / Beach / Desert — palms
            draw_palm(ctx, 16,  HORIZON_Y, 32, false, tc);
            draw_palm(ctx,  5,  HORIZON_Y, 22, false, tc);
            draw_palm(ctx, 184, HORIZON_Y, 32, true,  tc);
            draw_palm(ctx, 195, HORIZON_Y, 22, true,  tc);
            break;
        case 2: draw_mountain_peaks(ctx, tc); break;
        case 3: draw_evergreens(ctx, tc);     break;
        case 4: draw_buildings(ctx, tc);      break;
        case 5: draw_flowers(ctx, tc);        break;
    }
}

static void draw_neon_bar(GContext *ctx, int x, int y, int w, int h,
                           int current, int total, GColor fill, GColor glow, GColor bg) {
    graphics_context_set_fill_color(ctx, glow);
    graphics_fill_rect(ctx, GRect(x-1, y-1, w+2, h+2), 2, GCornersAll);
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, GRect(x, y, w, h), 2, GCornersAll);
    if (total > 0 && current > 0) {
        int fw = (current * w) / total;
        if (fw > w) fw = w;
        graphics_context_set_fill_color(ctx, fill);
        graphics_fill_rect(ctx, GRect(x, y, fw, h), 2, GCornersAll);
    }
}

// Solid info bar at a given Y with explicit height
static void draw_info_bar(GContext *ctx, int y, int bar_h, int w, GColor bg, GColor border) {
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, GRect(0, y, w, bar_h), 0, GCornerNone);
    graphics_context_set_stroke_color(ctx, border);
    graphics_draw_line(ctx, GPoint(0, y), GPoint(w, y));
}

// === CANVAS UPDATE PROC ===
static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    GFont f18  = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    GFont f18b = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    const ThemeColors *tc = prv_theme();

    // One-day mode: no overnight stops configured
    bool one_day = (s_sa.day_distances[1] == 0 &&
                    s_sa.day_distances[2] == 0 &&
                    s_sa.day_distances[3] == 0 &&
                    s_sa.day_distances[4] == 0);

    // 1. Sky gradient
    draw_sky(ctx, w, tc);

    // 2. Sun
    draw_sun(ctx, tc);

    // 3. Battery bar (top 3px)
    draw_battery(ctx, w);

    // 4. Road surface fill
    graphics_context_set_fill_color(ctx, GCA(tc->road));
    graphics_fill_rect(ctx, GRect(0, HORIZON_Y, w, h - HORIZON_Y), 0, GCornerNone);

    // 5. Perspective grid
    draw_grid(ctx, w, h, s_drive_offset, tc);

    // 6. Horizon line (2px)
    graphics_context_set_stroke_color(ctx, GCA(tc->horizon));
    graphics_draw_line(ctx, GPoint(0, HORIZON_Y),   GPoint(w, HORIZON_Y));
    graphics_draw_line(ctx, GPoint(0, HORIZON_Y+1), GPoint(w, HORIZON_Y+1));

    // 7. Landmarks (theme-specific)
    draw_landmark(ctx, tc);

    // 7b. Clock shadow/outline — drawn in canvas so TextLayer renders on top
    if (tc->txt_time_shadow) {
        GFont f42 = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
        graphics_context_set_text_color(ctx, GCA(tc->txt_time_shadow));
        graphics_draw_text(ctx, s_time_buf, f42, GRect(-1, 48, w+2, 47),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        graphics_draw_text(ctx, s_time_buf, f42, GRect(1, 48, w+2, 47),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        graphics_draw_text(ctx, s_time_buf, f42, GRect(0, 47, w, 47),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        graphics_draw_text(ctx, s_time_buf, f42, GRect(0, 49, w, 47),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }

    // 8. Progress bars
    int day_idx = (int)s_sa.trip_day - 1;
    if (day_idx < 0) day_idx = 0;
    if (day_idx > 4) day_idx = 4;
    int day_target = (int)s_sa.day_distances[day_idx];
    int completed = 0;
    for (int i = 0; i < day_idx; i++) completed += s_sa.day_distances[i];
    int day_current = (int)s_sa.current_distance - completed;
    if (day_current < 0) day_current = 0;

    if (one_day) {
        // Single bar at Day position — more road visible above it
        const char *label = (s_sa.trip_name[0] != '\0') ? s_sa.trip_name : "Your Trip";
        graphics_context_set_text_color(ctx, GCA(tc->txt_ovr));
        graphics_draw_text(ctx, label, f18,
            GRect(BAR_X, 133, 130, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
        graphics_draw_text(ctx, s_ovr_pct_buf, f18b,
            GRect(w-52, 133, 50, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
        draw_neon_bar(ctx, BAR_X, 153, BAR_W, BAR_H,
                      (int)s_sa.current_distance, (int)s_sa.total_distance,
                      GCA(tc->bar_ovr), GCA(tc->bar_ovr_glow), GCA(tc->bar_bg));
    } else {
        // Overall Trip bar  (label Y=99, bar Y=119)
        graphics_context_set_text_color(ctx, GCA(tc->txt_ovr));
        graphics_draw_text(ctx, "Overall Trip", f18,
            GRect(BAR_X, 99, 130, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
        graphics_draw_text(ctx, s_ovr_pct_buf, f18b,
            GRect(w-52, 99, 50, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
        draw_neon_bar(ctx, BAR_X, 119, BAR_W, BAR_H,
                      (int)s_sa.current_distance, (int)s_sa.total_distance,
                      GCA(tc->bar_ovr), GCA(tc->bar_ovr_glow), GCA(tc->bar_bg));

        // Day bar  (label Y=133, bar Y=153)
        graphics_context_set_text_color(ctx, GCA(tc->txt_day));
        graphics_draw_text(ctx, s_day_label_buf, f18,
            GRect(BAR_X, 133, 80, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
        graphics_draw_text(ctx, s_day_pct_buf, f18b,
            GRect(w-52, 133, 50, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
        draw_neon_bar(ctx, BAR_X, 153, BAR_W, BAR_H,
                      day_current, day_target,
                      GCA(tc->bar_day), GCA(tc->bar_day_glow), GCA(tc->bar_bg));
    }

    // Y=165–187: open road visible (22px in two-bar mode, 54px in one-day mode)

    // 9. Weather bar  (Y=188 H=20, to Y=208)
    if (s_sa.show_weather) {
        draw_info_bar(ctx, 188, 20, w, GCA(tc->info1_bg), GCA(tc->info1_border));
        graphics_context_set_stroke_color(ctx, GCA(tc->info1_border));
        graphics_draw_line(ctx, GPoint(w/2, 188), GPoint(w/2, 208));
        graphics_context_set_text_color(ctx, GCA(tc->txt_ovr));
        graphics_draw_text(ctx, s_weather_cur_buf, f18,
            GRect(2, 189, w/2 - 3, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
        graphics_context_set_text_color(ctx, GCA(tc->txt_day));
        graphics_draw_text(ctx, s_weather_dst_buf, f18,
            GRect(w/2 + 2, 189, w/2 - 3, 18),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }

    // 10. Route bar  (Y=208 H=20, fills to Y=228; text at Y=209 gives 3px bottom buffer)
    draw_info_bar(ctx, 208, 20, w, GCA(tc->info2_bg), GCA(tc->info2_border));
    graphics_context_set_text_color(ctx, C_TXT_WHT);
    graphics_draw_text(ctx, s_route_buf, f18,
        GRect(2, 209, w-4, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

// === DISPLAY UPDATES ===

static void update_time(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(s_time_buf, sizeof(s_time_buf),
             clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
    text_layer_set_text(s_time_layer, s_time_buf);
}

// Convert °C to display temp based on user's unit preference
static int to_display_temp(int celsius) {
    if (s_sa.temp_unit_f) return (celsius * 9 / 5) + 32;
    return celsius;
}

static void update_progress(void) {
    int cur   = (int)s_sa.current_distance;
    int tot   = (int)s_sa.total_distance;
    int day_n = (int)s_sa.trip_day;
    int day_i = day_n - 1;
    if (day_i < 0) day_i = 0;
    if (day_i > 4) day_i = 4;

    int d_tot = (int)s_sa.day_distances[day_i];
    int completed = 0;
    for (int i = 0; i < day_i; i++) completed += s_sa.day_distances[i];
    int d_cur = cur - completed;
    if (d_cur < 0) d_cur = 0;

    // Overall %
    int p = (tot > 0) ? (cur * 100) / tot : 0;
    if (p > 100) p = 100;
    APP_LOG(APP_LOG_LEVEL_INFO, "Progress: cur=%d tot=%d pct=%d%% day=%d d_cur=%d d_tot=%d",
            cur, tot, p, day_n, d_cur, d_tot);
    snprintf(s_ovr_pct_buf, sizeof(s_ovr_pct_buf), "%d%%", p);

    // Day %
    int dp = (d_tot > 0) ? (d_cur * 100) / d_tot : 0;
    if (dp > 100) dp = 100;
    snprintf(s_day_pct_buf, sizeof(s_day_pct_buf), "%d%%", dp);

    // Day label
    snprintf(s_day_label_buf, sizeof(s_day_label_buf), "Day %d", day_n);

    // Leg label for TextLayer (always has valid buffer)
    memset(s_leg_buf, 0, sizeof(s_leg_buf));
    if (day_n == 1) {
        const char *wp = (s_sb.waypoints[0][0] != '\0') ? s_sb.waypoints[0] : s_sa.dest_name;
        snprintf(s_leg_buf, sizeof(s_leg_buf), "Day 1 to %s", wp);
    } else if (day_n <= 5 && s_sb.waypoints[day_n-1][0] != '\0') {
        snprintf(s_leg_buf, sizeof(s_leg_buf), "Day %d to %s", day_n, s_sb.waypoints[day_n-1]);
    } else {
        snprintf(s_leg_buf, sizeof(s_leg_buf), "Day %d to %s", day_n, s_sa.dest_name);
    }
    text_layer_set_text(s_leg_layer, s_leg_buf);

    // Route bar text
    snprintf(s_route_buf, sizeof(s_route_buf), "%s  >  %s",
             s_sa.start_name, s_sa.dest_name);

    if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void update_weather(void) {
    static const char *conds[] = {"---","Clear","Cloudy","Rain","Snow","Storm"};

    // Current location
    int ci_c = (int)(uint8_t)s_sb.cached_cond_cur;
    if (ci_c < 0 || ci_c > 5) ci_c = 0;
    if ((int)s_sb.cached_temp_cur == -99 || !s_sa.show_weather) {
        strncpy(s_weather_cur_buf, "Now: ---", sizeof(s_weather_cur_buf)-1);
    } else {
        const char *u = s_sa.temp_unit_f ? "F" : "C";
        snprintf(s_weather_cur_buf, sizeof(s_weather_cur_buf), "%d\xc2\xb0%s %s",
                 to_display_temp((int)s_sb.cached_temp_cur), u, conds[ci_c]);
    }

    // Destination
    int ci_d = (int)(uint8_t)s_sb.cached_cond_dest;
    if (ci_d < 0 || ci_d > 5) ci_d = 0;
    if ((int)s_sb.cached_temp_dest == -99 || !s_sa.show_weather) {
        strncpy(s_weather_dst_buf, "Dest: ---", sizeof(s_weather_dst_buf)-1);
    } else {
        const char *u = s_sa.temp_unit_f ? "F" : "C";
        snprintf(s_weather_dst_buf, sizeof(s_weather_dst_buf), "%d\xc2\xb0%s %s",
                 to_display_temp((int)s_sb.cached_temp_dest), u, conds[ci_d]);
    }

    if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

// === WEATHER REQUEST ===
static void send_weather_request(void) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
        app_message_outbox_send();
    }
}

// === ANIMATION (grid scroll on tap, no car) ===
static void anim_timer_callback(void *data) {
    s_anim_timer = NULL;
    if (s_drive_frame <= 0) return;
    s_drive_frame--;
    s_drive_offset = (s_drive_offset + 7) % 131;
    if (s_drive_frame > 0) {
        s_anim_timer = app_timer_register(100, anim_timer_callback, NULL);
    }
    if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void start_drive_anim(void) {
    s_drive_frame = 20;
    if (s_anim_timer) { app_timer_cancel(s_anim_timer); s_anim_timer = NULL; }
    s_anim_timer = app_timer_register(100, anim_timer_callback, NULL);
}

// === ACCELEROMETER ===
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
    static time_t s_last_tap_time = 0;
    time_t now = time(NULL);
    if (now - s_last_tap_time < 15) return;
    s_last_tap_time = now;
    start_drive_anim();
    if (s_sa.show_weather) send_weather_request();
    update_progress();
    vibes_short_pulse();
}

// === BATTERY ===
static void battery_cb(BatteryChargeState state) {
    s_battery_level = (int)state.charge_percent;
    if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

// === APP MESSAGE ===

// Macro that handles both integer tuples and "integer-as-string" tuples from Clay selects
#define RI32(KEY, field) { Tuple *_t = dict_find(iterator, MESSAGE_KEY_##KEY); \
    if (_t) { \
        field = (_t->type == TUPLE_CSTRING) \
            ? (int32_t)atoi(_t->value->cstring) \
            : (int32_t)_t->value->int32; \
        changed = true; } }

#define RI8(KEY, field) { Tuple *_t = dict_find(iterator, MESSAGE_KEY_##KEY); \
    if (_t) { \
        field = (_t->type == TUPLE_CSTRING) \
            ? (int8_t)atoi(_t->value->cstring) \
            : (int8_t)_t->value->int32; \
        changed = true; } }

#define RB(KEY, field) { Tuple *_t = dict_find(iterator, MESSAGE_KEY_##KEY); \
    if (_t) { field = (_t->value->int32 != 0); changed = true; } }

#define RS(KEY, field, maxlen) { Tuple *_t = dict_find(iterator, MESSAGE_KEY_##KEY); \
    if (_t) { strncpy(field, _t->value->cstring, maxlen-1); \
              field[maxlen-1] = '\0'; changed = true; } }

static void inbox_received(DictionaryIterator *iterator, void *context) {
    bool changed = false;

    // Full reset — checked first so defaults are applied before remaining Clay settings land
    {
        Tuple *_t = dict_find(iterator, MESSAGE_KEY_ResetData);
        if (_t && _t->value->int32 != 0) {
            APP_LOG(APP_LOG_LEVEL_INFO, "RESET: wiping all persisted trip data");
            prv_default_settings();
            prv_save_settings();
            APP_LOG(APP_LOG_LEVEL_INFO, "RESET done: total=%ld cur=%ld",
                    (long)s_sa.total_distance, (long)s_sa.current_distance);
        }
    }

    // Weather data from pkjs (raw °C, condition int)
    {
        Tuple *t = dict_find(iterator, MESSAGE_KEY_TEMP_CURRENT);
        if (t) { s_sb.cached_temp_cur  = (int8_t)t->value->int32; changed = true; }
    }
    {
        Tuple *t = dict_find(iterator, MESSAGE_KEY_COND_CURRENT);
        if (t) { s_sb.cached_cond_cur  = (int8_t)t->value->int32; changed = true; }
    }
    {
        Tuple *t = dict_find(iterator, MESSAGE_KEY_TEMP_DEST);
        if (t) { s_sb.cached_temp_dest = (int8_t)t->value->int32; changed = true; }
    }
    {
        Tuple *t = dict_find(iterator, MESSAGE_KEY_COND_DEST);
        if (t) { s_sb.cached_cond_dest = (int8_t)t->value->int32; changed = true; }
    }

    // Auto-calculated distances from pkjs (geocoding + OSRM routing).
    // CalcTotalDist=0 means geocoding failed — never store it; it would make
    // every progress calculation show 0% forever (persisted across installs).
    {
        Tuple *_t = dict_find(iterator, MESSAGE_KEY_CalcTotalDist);
        if (_t) {
            int32_t v = (_t->type == TUPLE_CSTRING)
                ? (int32_t)atoi(_t->value->cstring) : _t->value->int32;
            if (v > 0) { s_sa.total_distance = v; changed = true; }
        }
    }
    // Day distances: 0 is valid for unused legs (e.g. a 3-day trip has DayDist4=DayDist5=0)
    RI32(CalcDayDist1,   s_sa.day_distances[0])
    RI32(CalcDayDist2,   s_sa.day_distances[1])
    RI32(CalcDayDist3,   s_sa.day_distances[2])
    RI32(CalcDayDist4,   s_sa.day_distances[3])
    RI32(CalcDayDist5,   s_sa.day_distances[4])

    // Clay settings
    RS(TripName,         s_sa.trip_name,    24)
    RS(StartName,        s_sa.start_name,   20)
    RS(DestName,         s_sa.dest_name,    20)
    RS(Waypoint1,        s_sb.waypoints[0], 20)
    RS(Waypoint2,        s_sb.waypoints[1], 20)
    RS(Waypoint3,        s_sb.waypoints[2], 20)
    RS(Waypoint4,        s_sb.waypoints[3], 20)
    RS(Waypoint5,        s_sb.waypoints[4], 20)
    RI32(CurrentDistance, s_sa.current_distance)
    RI8(ProgressUnit,    s_sa.progress_unit)
    RI8(TripDay,         s_sa.trip_day)
    RI8(Theme,           s_sa.theme)
    RB(ShowWeather,      s_sa.show_weather)
    RB(TempUnit,         s_sa.temp_unit_f)

#undef RI32
#undef RI8
#undef RB
#undef RS

    APP_LOG(APP_LOG_LEVEL_INFO, "Rx: total=%ld cur=%ld day=%d unit=%d",
            (long)s_sa.total_distance, (long)s_sa.current_distance,
            (int)s_sa.trip_day, (int)s_sa.progress_unit);

    if (changed) prv_save_settings();
    update_text_colors();
    update_progress();
    update_weather();
}

static void inbox_dropped(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox dropped: %d", (int)reason);
}

// === TICK ===
static void tick_handler(struct tm *t, TimeUnits u) {
    update_time();
    if (t->tm_min % 5 == 0 && s_sa.show_weather) send_weather_request();
}

// === WINDOW ===
static void window_load(Window *window) {
    Layer *wl = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(wl);
    int w = bounds.size.w;

    window_set_background_color(window, GColorBlack);

    // Canvas (beneath TextLayers)
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(wl, s_canvas_layer);

    // Leg label — Y=3, H=20, cyan, GOTHIC_18
    s_leg_layer = text_layer_create(GRect(0, 3, w, 20));
    text_layer_set_background_color(s_leg_layer, GColorClear);
    text_layer_set_text_color(s_leg_layer, GColorCyan);
    text_layer_set_font(s_leg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_leg_layer, GTextAlignmentCenter);
    layer_add_child(wl, text_layer_get_layer(s_leg_layer));

    // Time — Y=48, H=47, bottom at Y=95 (just above horizon at 97), LECO_42 (~10% larger)
    s_time_layer = text_layer_create(GRect(0, 48, w, 47));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(wl, text_layer_get_layer(s_time_layer));

    update_text_colors();
    update_time();
    update_progress();
    update_weather();
}

static void window_unload(Window *window) {
    if (s_anim_timer) { app_timer_cancel(s_anim_timer); s_anim_timer = NULL; }
    layer_destroy(s_canvas_layer);
    text_layer_destroy(s_leg_layer);
    text_layer_destroy(s_time_layer);
}

// === INIT / DEINIT / MAIN ===
static void init(void) {
    prv_load_settings();

    app_message_register_inbox_received(inbox_received);
    app_message_register_inbox_dropped(inbox_dropped);
    app_message_open(512, 128);

    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers){
        .load   = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_cb);
    battery_cb(battery_state_service_peek());
    accel_tap_service_subscribe(accel_tap_handler);

    if (s_sa.show_weather) send_weather_request();
}

static void deinit(void) {
    accel_tap_service_unsubscribe();
    battery_state_service_unsubscribe();
    tick_timer_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}
