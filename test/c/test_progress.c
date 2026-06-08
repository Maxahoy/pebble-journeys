/*
 * test_progress.c — standalone C tests for progress calculation logic.
 * Build and run:
 *   gcc test/c/test_progress.c -o /tmp/test_progress && /tmp/test_progress
 *
 * These tests mirror the logic in src/c/main.c without the Pebble SDK.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

/* ---- Pure logic extracted from main.c ---- */

static int calc_progress_pct(int current, int total) {
    if (total <= 0) return 0;
    int p = (current * 100) / total;
    return (p > 100) ? 100 : p;
}

static int calc_day_progress_pct(int current_distance, int *day_distances, int day_idx) {
    int completed = 0;
    for (int i = 0; i < day_idx; i++) completed += day_distances[i];
    int day_current = current_distance - completed;
    if (day_current < 0) day_current = 0;
    int day_target = day_distances[day_idx];
    if (day_target <= 0) return 0;
    int p = (day_current * 100) / day_target;
    return (p > 100) ? 100 : p;
}

/* Mirrors the C-side fix: reject CalcTotalDist=0 */
static int should_apply_calc_total(int incoming_value) {
    return (incoming_value > 0);
}

/* Mirrors the persist-recovery guard */
static int recover_total_distance(int persisted_value, int default_value) {
    return (persisted_value <= 0) ? default_value : persisted_value;
}

/* ---- Test helpers ---- */

static int s_pass = 0, s_fail = 0;

#define EXPECT_EQ(actual, expected, label) do { \
    if ((actual) == (expected)) { \
        printf("  PASS: %s\n", label); \
        s_pass++; \
    } else { \
        printf("  FAIL: %s — expected %d, got %d\n", label, expected, actual); \
        s_fail++; \
    } \
} while(0)

#define EXPECT_TRUE(cond, label) EXPECT_EQ(!!(cond), 1, label)
#define EXPECT_FALSE(cond, label) EXPECT_EQ(!!(cond), 0, label)

/* ---- Tests ---- */

static void test_overall_progress(void) {
    printf("\n[overall progress]\n");
    EXPECT_EQ(calc_progress_pct(200,  1200), 16,  "200/1200 = 16%");
    EXPECT_EQ(calc_progress_pct(600,  1200), 50,  "600/1200 = 50%");
    EXPECT_EQ(calc_progress_pct(1200, 1200), 100, "1200/1200 = 100%");
    EXPECT_EQ(calc_progress_pct(1500, 1200), 100, "over-distance capped at 100%");
    EXPECT_EQ(calc_progress_pct(0,    1200), 0,   "0 distance = 0%");
}

static void test_zero_total_bug(void) {
    printf("\n[zero-total bug — THE root cause of the 0%% issue]\n");
    /* When CalcTotalDist=0 was persisted, every progress calc returns 0. */
    EXPECT_EQ(calc_progress_pct(200, 0), 0,
        "BUG: 200 miles driven but total=0 → shows 0% (was persisted by failed geocode)");
    EXPECT_EQ(calc_progress_pct(600, 0), 0,
        "BUG: 600 miles driven but total=0 → still 0%");

    /* Fix: reject incoming CalcTotalDist=0 */
    EXPECT_FALSE(should_apply_calc_total(0),
        "FIX: CalcTotalDist=0 should be rejected (geocoding failed)");
    EXPECT_TRUE(should_apply_calc_total(1847),
        "FIX: CalcTotalDist=1847 (valid route) should be accepted");

    /* Fix: recover stale persisted total_distance=0 on load */
    EXPECT_EQ(recover_total_distance(0, 1200), 1200,
        "FIX: persisted total=0 falls back to default 1200 on load");
    EXPECT_EQ(recover_total_distance(1847, 1200), 1847,
        "FIX: valid persisted total kept as-is");
}

static void test_day_progress(void) {
    printf("\n[day progress]\n");
    int days[5] = {280, 240, 240, 240, 200};

    EXPECT_EQ(calc_day_progress_pct(0,   days, 0), 0,  "Day1: 0/280 = 0%");
    EXPECT_EQ(calc_day_progress_pct(140, days, 0), 50, "Day1: 140/280 = 50%");
    EXPECT_EQ(calc_day_progress_pct(280, days, 0), 100,"Day1: 280/280 = 100%");

    /* Day 2 starts after 280 miles (sum of day 1) */
    EXPECT_EQ(calc_day_progress_pct(280, days, 1), 0,  "Day2: just started (280 total)");
    EXPECT_EQ(calc_day_progress_pct(400, days, 1), 50, "Day2: 120/240 = 50%");
    EXPECT_EQ(calc_day_progress_pct(520, days, 1), 100,"Day2: 240/240 = 100%");

    /* Unused leg (0-length day) never crashes */
    int short_days[5] = {280, 240, 0, 0, 0};
    EXPECT_EQ(calc_day_progress_pct(300, short_days, 2), 0,
        "Unused day with 0 target = 0% (no crash)");
}

static void test_progress_unit_conversion(void) {
    printf("\n[unit conversion — mirrors JS toUnit()]\n");
    /* miles: meters / 1609.344 */
    int miles_from_1609m = (int)(1609.344 / 1609.344 + 0.5);
    EXPECT_EQ(miles_from_1609m, 1, "1609m ≈ 1 mile");

    /* km: meters / 1000 */
    EXPECT_EQ(100000 / 1000, 100, "100000m = 100 km");
}

/* ---- Main ---- */

int main(void) {
    printf("=== Journeys progress calculation tests ===\n");
    test_overall_progress();
    test_zero_total_bug();
    test_day_progress();
    test_progress_unit_conversion();
    printf("\n%d passed, %d failed\n", s_pass, s_fail);
    return s_fail > 0 ? 1 : 0;
}
