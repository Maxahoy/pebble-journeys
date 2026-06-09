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

// === LAYOUT ===
#define HORIZON_Y  97
#define BAR_X      10
#define BAR_W      180
#define BAR_H      10

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

static void draw_sky(GContext *ctx, int w) {
    // 7 bands from deep navy (top) → hot pink (horizon)
    static const struct { uint8_t y; uint8_t argb; } bands[] = {
        {14, 0b11000001}, // oxford blue
        {28, 0b11010001}, // imperial purple
        {44, 0b11100010}, // medium purple
        {60, 0b11100011}, // vivid violet
        {76, 0b11110011}, // magenta
        {88, 0b11110111}, // shocking pink
        {97, 0b11110111}, // horizon level
    };
    int prev = 0;
    for (int i = 0; i < 7; i++) {
        graphics_context_set_fill_color(ctx, ((GColor){.argb = bands[i].argb}));
        graphics_fill_rect(ctx, GRect(0, prev, w, bands[i].y - prev), 0, GCornerNone);
        prev = bands[i].y;
    }
}

static void draw_sun(GContext *ctx) {
    int cx = 100, cy = 47, r = 28;
    // Yellow circle
    graphics_context_set_fill_color(ctx, C_SUN_FILL);
    graphics_fill_circle(ctx, GPoint(cx, cy), r);
    // Dark horizontal stripes in lower half (retrowave look)
    // Half-widths for dy = 0,3,6,...,27
    static const uint8_t hw[] = {28,27,27,26,25,23,21,18,14,7};
    for (int i = 0; i < 10; i++) {
        int sy = cy + i * 3;
        if (sy < cy) continue;
        if (sy > cy + r) break;
        if (i % 2 == 0) {
            graphics_context_set_fill_color(ctx, C_SUN_LINE);
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

static void draw_grid(GContext *ctx, int w, int h, int offset) {
    int vx = w / 2, vy = HORIZON_Y;
    int road_h = h - vy;  // 228 - 97 = 131

    // Converging perspective lines
    graphics_context_set_stroke_color(ctx, C_GRID_V);
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
        graphics_context_set_stroke_color(ctx, (i % 2 == 0) ? C_GRID_H1 : C_GRID_H2);
        graphics_draw_line(ctx, GPoint(vx - spread, y), GPoint(vx + spread, y));
    }
}

static void draw_palm(GContext *ctx, int bx, int by, int trunk_h, bool flip) {
    graphics_context_set_fill_color(ctx, C_PALM_T);
    for (int i = 0; i < trunk_h; i++) {
        int lean = flip ? -(i / 6) : (i / 6);
        graphics_fill_rect(ctx, GRect(bx + lean - 1, by - i, 3, 1), 0, GCornerNone);
    }
    int tx = bx + (flip ? -(trunk_h / 6) : (trunk_h / 6));
    int ty = by - trunk_h;
    graphics_context_set_fill_color(ctx, C_PALM_L);
    graphics_fill_circle(ctx, GPoint(tx,     ty - 5), 6);
    graphics_fill_circle(ctx, GPoint(tx - 8, ty - 1), 5);
    graphics_fill_circle(ctx, GPoint(tx + 8, ty - 1), 5);
    graphics_fill_circle(ctx, GPoint(tx - 5, ty + 3), 4);
    graphics_fill_circle(ctx, GPoint(tx + 5, ty + 3), 4);
}

static void draw_neon_bar(GContext *ctx, int x, int y, int w, int h,
                           int current, int total, GColor fill, GColor glow) {
    graphics_context_set_fill_color(ctx, glow);
    graphics_fill_rect(ctx, GRect(x-1, y-1, w+2, h+2), 2, GCornersAll);
    graphics_context_set_fill_color(ctx, C_BAR_BG);
    graphics_fill_rect(ctx, GRect(x, y, w, h), 2, GCornersAll);
    if (total > 0 && current > 0) {
        int fw = (current * w) / total;
        if (fw > w) fw = w;
        graphics_context_set_fill_color(ctx, fill);
        graphics_fill_rect(ctx, GRect(x, y, fw, h), 2, GCornersAll);
    }
}

// Solid info bar at a given Y, height 16px
static void draw_info_bar(GContext *ctx, int y, int w, GColor bg, GColor border) {
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, GRect(0, y, w, 16), 0, GCornerNone);
    graphics_context_set_stroke_color(ctx, border);
    graphics_draw_line(ctx, GPoint(0, y), GPoint(w, y));
}

// === CANVAS UPDATE PROC ===
static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    GFont f14  = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    GFont f14b = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

    // 1. Sky gradient
    draw_sky(ctx, w);

    // 2. Retrowave sun (sits behind time display)
    draw_sun(ctx);

    // 3. Battery bar (top 3px)
    draw_battery(ctx, w);

    // 4. Road surface fill
    graphics_context_set_fill_color(ctx, C_ROAD);
    graphics_fill_rect(ctx, GRect(0, HORIZON_Y, w, h - HORIZON_Y), 0, GCornerNone);

    // 5. Perspective grid
    draw_grid(ctx, w, h, s_drive_offset);

    // 6. Horizon line (2px bright pink)
    graphics_context_set_stroke_color(ctx, C_HORIZON);
    graphics_draw_line(ctx, GPoint(0, HORIZON_Y),   GPoint(w, HORIZON_Y));
    graphics_draw_line(ctx, GPoint(0, HORIZON_Y+1), GPoint(w, HORIZON_Y+1));

    // 7. Palm trees
    draw_palm(ctx, 16,  HORIZON_Y, 32, false);
    draw_palm(ctx, 5,   HORIZON_Y, 22, false);
    draw_palm(ctx, 184, HORIZON_Y, 32, true);
    draw_palm(ctx, 195, HORIZON_Y, 22, true);

    // 8. Overall Trip bar  (Y=99–125)
    graphics_context_set_text_color(ctx, C_TXT_OVR);
    graphics_draw_text(ctx, "Overall Trip", f14,
        GRect(BAR_X, 99, 120, 13),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, s_ovr_pct_buf, f14b,
        GRect(w-42, 99, 40, 13),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    draw_neon_bar(ctx, BAR_X, 115, BAR_W, BAR_H,
                  (int)s_sa.current_distance, (int)s_sa.total_distance,
                  C_BAR_OVR, C_BAR_OVRG);

    // 9. Day bar  (Y=128–154)
    int day_idx = (int)s_sa.trip_day - 1;
    if (day_idx < 0) day_idx = 0;
    if (day_idx > 4) day_idx = 4;
    int day_target = (int)s_sa.day_distances[day_idx];
    int completed  = 0;
    for (int i = 0; i < day_idx; i++) completed += s_sa.day_distances[i];
    int day_current = (int)s_sa.current_distance - completed;
    if (day_current < 0) day_current = 0;

    graphics_context_set_text_color(ctx, C_TXT_DAY);
    graphics_draw_text(ctx, s_day_label_buf, f14,
        GRect(BAR_X, 128, 80, 13),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, s_day_pct_buf, f14b,
        GRect(w-42, 128, 40, 13),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    draw_neon_bar(ctx, BAR_X, 144, BAR_W, BAR_H,
                  day_current, day_target,
                  C_BAR_DAY, C_BAR_DAYG);

    // Y=155–189: open road visible (35px)

    // 10. Weather bar  (Y=190–205, solid background)
    if (s_sa.show_weather) {
        draw_info_bar(ctx, 190, w, C_BAR_BG_SOLID, GColorCyan);
        // Center divider
        graphics_context_set_stroke_color(ctx, GColorCyan);
        graphics_draw_line(ctx, GPoint(w/2, 190), GPoint(w/2, 206));
        // Left: current location
        graphics_context_set_text_color(ctx, C_TXT_OVR);
        graphics_draw_text(ctx, s_weather_cur_buf, f14,
            GRect(2, 191, w/2 - 4, 14),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
        // Right: destination
        graphics_context_set_text_color(ctx, C_TXT_DAY);
        graphics_draw_text(ctx, s_weather_dst_buf, f14,
            GRect(w/2 + 2, 191, w/2 - 4, 14),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }

    // 11. Route bar  (Y=207–222, solid background)
    draw_info_bar(ctx, 207, w, C_BAR_BG_SOLID2, GColorMagenta);
    graphics_context_set_text_color(ctx, C_TXT_WHT);
    graphics_draw_text(ctx, s_route_buf, f14,
        GRect(2, 208, w-4, 14),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    // 12. Bottom neon grid decoration (Y=223–228)
    graphics_context_set_stroke_color(ctx, GColorCyan);
    graphics_draw_line(ctx, GPoint(0, 223), GPoint(w, 223));
    for (int gx = 0; gx < w; gx += 12)
        graphics_draw_line(ctx, GPoint(gx, 223), GPoint(gx, h));
    graphics_context_set_stroke_color(ctx, GColorMagenta);
    for (int gx = 4; gx < w; gx += 12)
        graphics_draw_line(ctx, GPoint(gx, 225), GPoint(gx+6, 225));
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

    // Leg label — Y=3, H=14, cyan
    s_leg_layer = text_layer_create(GRect(0, 3, w, 14));
    text_layer_set_background_color(s_leg_layer, GColorClear);
    text_layer_set_text_color(s_leg_layer, GColorCyan);
    text_layer_set_font(s_leg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_leg_layer, GTextAlignmentCenter);
    layer_add_child(wl, text_layer_get_layer(s_leg_layer));

    // Time — Y=55, bottom of sky just above horizon, LECO_36
    s_time_layer = text_layer_create(GRect(0, 55, w, 40));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(wl, text_layer_get_layer(s_time_layer));

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
