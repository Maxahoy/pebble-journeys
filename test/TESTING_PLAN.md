# Journeys — Road Trip Tracker: Testing Plan

## Quick start

```bash
# C tests (no Pebble SDK needed)
gcc test/c/test_progress.c -o /tmp/test_progress && /tmp/test_progress

# JS tests (Node.js)
node test/js/test_logic.js

# Full build
pebble build

# Emulator smoke test
pebble install --emulator emery
pebble screenshot --no-open --emulator emery screenshot_emery.png
```

---

## 1. Unit test coverage

### C: `test/c/test_progress.c` (51 tests)

| Suite | What it covers |
|---|---|
| overall progress | `calc_progress_pct` — 0%, 50%, 100%, capped at 100% |
| zero-total bug | The root CalcTotalDist=0 corruption: bug behavior documented, both fixes verified |
| day progress | `calc_day_progress_pct` — all days, partial progress, completion |
| unit conversion | Meters→miles and meters→km formulas |
| temperature conversion | Celsius→Fahrenheit at freezing, boiling, crossover (-40) |
| progress clamping | Over-distance caps at 100%, zero returns 0% |
| day index bounds | Day 0/4 (min/max), unused legs (0 target), no div-by-zero |
| multi-day cumulative | Full Portsmouth→KC→Denver trip: start, halfway, overnight stop, destination |
| persist recovery | `recover_total_distance` — negative, zero, valid, different default |

### JS: `test/js/test_logic.js` (41 tests)

| Suite | What it covers |
|---|---|
| haversineMeters | City pairs, same point, precision (equatorial/meridional degrees), symmetry |
| weatherCodeToInt | All five condition buckets (clear, cloudy, rain, snow, thunder) |
| getStoredStr fallback | Primary key, Clay fallback key, both absent, empty-string value |
| sendDistances | CalcTotalDist=0 guard (the original bug), valid route, km unit, partial failure |
| detectTripPosition (4-stop) | Chillicothe→Elwood→SLC→Portland: null when no cache, Day 1/2 detection, destination |
| detectTripPosition (PKD) | Portsmouth→KC→Denver: leg haversine ranges, Cincinnati/St.Louis/Goodland positions |
| Photon geocoder | GeoJSON [lon,lat] extraction, empty features, coordinate sign sanity |
| detectTripPosition edges | 1-day trip (all positions = Day 1), no cache, 1-stop invalid route |
| haversine precision | Symmetry, equatorial/meridional degree sizes |

---

## 2. Integration / hardware testing steps

### After each code change

1. `pebble build` — must succeed with no errors
2. `pebble install --emulator emery` — watchface must load, not crash
3. `pebble screenshot --no-open --emulator emery screenshot_emery.png` — visually inspect layout

### On physical hardware

Connect the Pebble app on phone and run `pebble logs --phone <IP>` before each step.

**Initial install:**
1. Install `build/pebble-journeys.pbw` via Pebble app sideload
2. Confirm watch shows "Journeys" watchface
3. Logs show `Progress: cur=0 tot=1200 pct=0%` (default settings)

**Clay configuration:**
4. Open Clay config in Pebble app → Journeys
5. Set Start City, Destination City, overnight stops
6. Set Distance Units (Miles recommended)
7. Save — logs must show:
   ```
   Clay settings synced: start=<city> dest=<city>
   Geocoding N locations...
   Geocoded[0] <city> -> <lat>,<lon>    ← Photon working
   Sending distances: total=<N>          ← OSRM working
   Rx: total=<N> cur=0 day=1 unit=0
   Progress: cur=0 tot=<N> pct=0%
   ```

**If geocoding still fails (XML error):**
- Check network connectivity on the phone
- Try shorter city names (e.g. "Portland OR" not "Portland, Oregon")
- Use Debug / Overrides → Distance Override as a fallback

**Manual override test:**
8. Open Clay → Debug / Overrides → set Distance Override to a non-zero value
9. Save — logs must show `Rx: total=1200 cur=<N>` and `pct=<N>%` on watch

**Full Reset:**
10. Open Clay → Danger Zone → toggle "Full Reset" ON
11. Save — logs must show:
    ```
    RESET: wiping all persisted trip data
    RESET done: total=1200 cur=0
    ```
12. Toggle Full Reset back OFF before next normal save

**Wrist shake:**
13. Shake wrist — grid animates, vibration fires, weather updates
14. Logs show `Sending weather: cur=<temp>` and `Weather+position sent OK`

**Minute tick:**
15. Wait for minute change — time updates, no crash
16. At minute divisible by 5 — weather request fires automatically

**GPS auto-detection:**
17. (Requires a real trip or spoofed GPS) Shake wrist after geocoding completes
18. Logs show `Position sent: day=<N> dist=<M>` and watch updates to correct day/distance

---

## 3. Known limitations and edge cases

| Limitation | Behavior | Workaround |
|---|---|---|
| Geocoding requires internet on phone | All geocodes fail; zero-guard prevents bad persist | Use Distance Override in Debug section |
| Photon may return wrong result for ambiguous city names | Wrong coords → wrong route distances | Use state/country suffix: "Portland, OR" not "Portland" |
| OSRM public demo server can be slow or down | Route distances not sent; progress stays at default 1200mi | Use Distance Override |
| GPS auto-detection requires route to be geocoded first | `detectTripPosition` returns null if `stopCoords` not cached | Open Clay and save first to trigger geocoding |
| Trip distance is straight-line during GPS detection | Position interpolation uses haversine, not actual roads | Actual driving distance will differ slightly |
| Persist survives reinstall (same UUID) | Stale CalcTotalDist=0 recovered to 1200 on boot | Use Full Reset in Clay if old data is causing issues |
| Weather destination shows `---` until geocoding completes | Geocoding happens after Clay save | Wait 15–30 seconds after saving Clay settings |
| Progress bars show 0% at very start of trip | Correct — 0 miles driven | Set Distance Override > 0 if you want to test the display |

---

## 4. Test maintenance

- When adding a new Clay field: add a `getStoredStr` test for the new key
- When changing the progress formula: update `test_overall_progress` and `test_multiday_progress`
- When changing geocoder API: update the Photon response format tests and add a new response fixture
- When changing layout constants: visually verify via QEMU screenshot; no automated pixel tests exist
