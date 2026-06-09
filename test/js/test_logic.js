/*
 * test_logic.js — JS tests for pkjs routing and progress logic.
 * Run: node test/js/test_logic.js
 *
 * Tests the pure functions from src/pkjs/index.js and covers the specific
 * bug where CalcTotalDist=0 corrupts total_distance on the watch.
 */

'use strict';
var assert = require('assert');

/* ---- Minimal mocks ---- */

/* localStorage mock backed by a plain object */
function makeLocalStorage(initial) {
    var store = Object.assign({}, initial || {});
    return {
        getItem:  function(k)    { return store.hasOwnProperty(k) ? store[k] : null; },
        setItem:  function(k, v) { store[k] = String(v); },
        removeItem:function(k)   { delete store[k]; },
        clear:    function()     { store = {}; },
        _store:   store
    };
}

/* Pebble mock — records all sendAppMessage calls */
function makePebbleMock() {
    var messages = [];
    return {
        sendAppMessage: function(dict, onSuccess, onFailure) {
            messages.push(JSON.parse(JSON.stringify(dict)));
            if (onSuccess) onSuccess();
        },
        messages: messages,
        reset: function() { messages.length = 0; }
    };
}

/* ---- Functions under test (inlined from src/pkjs/index.js) ---- */

function haversineMeters(lat1, lon1, lat2, lon2) {
    var R = 6371000;
    var p1 = lat1 * Math.PI / 180, p2 = lat2 * Math.PI / 180;
    var dp = (lat2 - lat1) * Math.PI / 180;
    var dl = (lon2 - lon1) * Math.PI / 180;
    var a = Math.sin(dp/2)*Math.sin(dp/2) +
            Math.cos(p1)*Math.cos(p2)*Math.sin(dl/2)*Math.sin(dl/2);
    return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
}

function weatherCodeToInt(code) {
    if (code === 0)                       return 1;
    if (code <= 3)                        return 2;
    if (code >= 45 && code <= 48)         return 2;
    if (code >= 51 && code <= 67)         return 3;
    if (code >= 71 && code <= 77)         return 4;
    if (code >= 80 && code <= 82)         return 3;
    if (code >= 95)                       return 5;
    return 2;
}

function makeGetStoredStr(localStorage) {
    return function getStoredStr(key1, key2) {
        return localStorage.getItem(key1) || localStorage.getItem(key2) || '';
    };
}

function makeSendDistances(localStorage, Pebble) {
    return function sendDistances(legMeters) {
        var unit = parseInt(localStorage.getItem('progressUnit') || '0');
        function toUnit(m) {
            return (unit === 1) ? Math.round(m / 1000) : Math.round(m / 1609.344);
        }
        var totalMeters = legMeters.reduce(function(sum, m) { return sum + m; }, 0);

        /* THE BUG FIX: skip when geocoding produced nothing */
        if (totalMeters === 0) return false;

        Pebble.sendAppMessage({
            'CalcTotalDist': toUnit(totalMeters),
            'CalcDayDist1':  toUnit(legMeters[0]),
            'CalcDayDist2':  toUnit(legMeters[1]),
            'CalcDayDist3':  toUnit(legMeters[2]),
            'CalcDayDist4':  toUnit(legMeters[3]),
            'CalcDayDist5':  toUnit(legMeters[4])
        });
        return true;
    };
}

function makeDetectTripPosition(localStorage) {
    return function detectTripPosition(currentLat, currentLon) {
        try {
            var coordsJson = localStorage.getItem('stopCoords');
            var cumJson    = localStorage.getItem('stopCumMeters');
            if (!coordsJson || !cumJson) return null;
            var allCoords = JSON.parse(coordsJson);
            var allCum    = JSON.parse(cumJson);

            var stops = [];
            for (var i = 0; i < allCoords.length; i++) {
                if (allCoords[i] && allCum[i] !== undefined) {
                    stops.push({ lat: allCoords[i].lat, lon: allCoords[i].lon, cumM: allCum[i] });
                }
            }
            if (stops.length < 2) return null;

            var dists = stops.map(function(s) {
                return haversineMeters(currentLat, currentLon, s.lat, s.lon);
            });
            var nearIdx = 0;
            for (var j = 1; j < stops.length; j++) {
                if (dists[j] < dists[nearIdx]) nearIdx = j;
            }

            var legIdx;
            if (nearIdx === stops.length - 1) {
                legIdx = stops.length - 2;
            } else if (nearIdx === 0) {
                legIdx = 0;
            } else {
                legIdx = (dists[nearIdx + 1] < dists[nearIdx - 1]) ? nearIdx : nearIdx - 1;
            }
            if (legIdx < 0) legIdx = 0;
            if (legIdx >= stops.length - 1) legIdx = stops.length - 2;

            var day = legIdx + 1;
            var legStart = stops[legIdx];
            var legEnd   = stops[legIdx + 1];
            var dFrom = haversineMeters(currentLat, currentLon, legStart.lat, legStart.lon);
            var dTo   = haversineMeters(currentLat, currentLon, legEnd.lat,   legEnd.lon);
            var legLen = legEnd.cumM - legStart.cumM;
            var frac = (dFrom + dTo > 1) ? dFrom / (dFrom + dTo) : 0;
            if (frac > 1) frac = 1;
            var totalM = legStart.cumM + frac * legLen;
            var unit = parseInt(localStorage.getItem('progressUnit') || '0');
            var dist = (unit === 1) ? Math.round(totalM / 1000) : Math.round(totalM / 1609.344);
            return { day: day, distance: dist };
        } catch(e) {
            return null;
        }
    };
}

/* ---- Test helpers ---- */
var passed = 0, failed = 0;

function test(label, fn) {
    try {
        fn();
        console.log('  PASS: ' + label);
        passed++;
    } catch(e) {
        console.log('  FAIL: ' + label);
        console.log('        ' + e.message);
        failed++;
    }
}

/* ---- Test suites ---- */

console.log('\n[haversineMeters]');

test('Cincinnati → Columbus ≈ 165km', function() {
    var d = haversineMeters(39.103, -84.512, 39.961, -82.999);
    assert.ok(d > 160000 && d < 175000, 'expected ~168km, got ' + Math.round(d/1000) + 'km');
});

test('Same point = 0m', function() {
    assert.strictEqual(haversineMeters(39.0, -84.0, 39.0, -84.0), 0);
});

test('Cincinnati → Salt Lake City ≈ 2300km straight-line', function() {
    var d = haversineMeters(39.103, -84.512, 40.76, -111.89);
    assert.ok(d > 2200000 && d < 2500000, 'expected ~2330km straight-line, got ' + Math.round(d/1000) + 'km');
});

/* ---- */
console.log('\n[weatherCodeToInt]');

test('code 0 → clear (1)',       function() { assert.strictEqual(weatherCodeToInt(0),  1); });
test('code 1 → cloudy (2)',      function() { assert.strictEqual(weatherCodeToInt(1),  2); });
test('code 61 → rain (3)',       function() { assert.strictEqual(weatherCodeToInt(61), 3); });
test('code 73 → snow (4)',       function() { assert.strictEqual(weatherCodeToInt(73), 4); });
test('code 95 → thunder (5)',    function() { assert.strictEqual(weatherCodeToInt(95), 5); });

/* ---- */
console.log('\n[getStoredStr fallback]');

test('returns own key when present', function() {
    var ls = makeLocalStorage({ destName: 'Portland, OR', DestName: 'WRONG' });
    var getStoredStr = makeGetStoredStr(ls);
    assert.strictEqual(getStoredStr('destName', 'DestName'), 'Portland, OR');
});

test('falls back to Clay key when own key absent', function() {
    var ls = makeLocalStorage({ DestName: 'Portland, OR' });
    var getStoredStr = makeGetStoredStr(ls);
    assert.strictEqual(getStoredStr('destName', 'DestName'), 'Portland, OR');
});

test('returns empty string when both keys absent', function() {
    var ls = makeLocalStorage({});
    var getStoredStr = makeGetStoredStr(ls);
    assert.strictEqual(getStoredStr('destName', 'DestName'), '');
});

/* ---- */
console.log('\n[sendDistances — CalcTotalDist=0 bug fix]');

test('BUG SCENARIO: all legMeters=0 → should NOT send (was the bug)', function() {
    var ls = makeLocalStorage({ progressUnit: '0' });
    var pebble = makePebbleMock();
    var sendDistances = makeSendDistances(ls, pebble);
    var sent = sendDistances([0, 0, 0, 0, 0]);
    assert.strictEqual(sent, false, 'sendDistances should return false when skipping');
    assert.strictEqual(pebble.messages.length, 0, 'No message should be sent');
});

test('Valid miles route sends CalcTotalDist > 0', function() {
    var ls = makeLocalStorage({ progressUnit: '0' });
    var pebble = makePebbleMock();
    var sendDistances = makeSendDistances(ls, pebble);
    // Cincinnati→Elwood: ~700mi, Elwood→SLC: ~900mi, SLC→Portland: ~750mi
    var legs = [1126540, 1448000, 1207000, 0, 0];  // meters
    sendDistances(legs);
    assert.strictEqual(pebble.messages.length, 1);
    var msg = pebble.messages[0];
    assert.ok(msg['CalcTotalDist'] > 0, 'CalcTotalDist should be > 0');
    assert.ok(msg['CalcTotalDist'] > 2000, 'Total should be > 2000 miles');
    assert.strictEqual(msg['CalcDayDist4'], 0, 'Unused leg 4 = 0');
    assert.strictEqual(msg['CalcDayDist5'], 0, 'Unused leg 5 = 0');
});

test('km unit conversion', function() {
    var ls = makeLocalStorage({ progressUnit: '1' });
    var pebble = makePebbleMock();
    var sendDistances = makeSendDistances(ls, pebble);
    sendDistances([100000, 200000, 0, 0, 0]);  // 100km + 200km legs
    assert.strictEqual(pebble.messages.length, 1);
    assert.strictEqual(pebble.messages[0]['CalcTotalDist'], 300);
    assert.strictEqual(pebble.messages[0]['CalcDayDist1'],  100);
    assert.strictEqual(pebble.messages[0]['CalcDayDist2'],  200);
});

/* ---- */
console.log('\n[detectTripPosition]');

/* Fake route: Chillicothe(0km) → Elwood(700km) → SLC(1600km) → Portland(2400km) */
var FAKE_COORDS = [
    { lat: 39.33,  lon: -82.98 },  // Chillicothe, OH
    { lat: 41.99,  lon: -99.00 },  // Elwood, NE
    { lat: 40.76,  lon: -111.89 }, // Salt Lake City, UT
    { lat: 45.52,  lon: -122.68 }  // Portland, OR
];
var FAKE_CUM = [0, 1126540, 2574540, 3781540]; // meters

test('returns null when no route cached', function() {
    var ls = makeLocalStorage({});
    var detect = makeDetectTripPosition(ls);
    assert.strictEqual(detect(39.33, -82.98), null);
});

test('near start → Day 1, ~0 miles', function() {
    var ls = makeLocalStorage({
        stopCoords:   JSON.stringify(FAKE_COORDS),
        stopCumMeters: JSON.stringify(FAKE_CUM),
        progressUnit: '0'
    });
    var detect = makeDetectTripPosition(ls);
    var result = detect(39.33, -82.98);  // at Chillicothe exactly
    assert.ok(result !== null, 'should return a result');
    assert.strictEqual(result.day, 1, 'should be Day 1');
    assert.ok(result.distance < 20, 'should be < 20 miles from start');
});

test('past first stop → Day 2', function() {
    var ls = makeLocalStorage({
        stopCoords:    JSON.stringify(FAKE_COORDS),
        stopCumMeters: JSON.stringify(FAKE_CUM),
        progressUnit:  '0'
    });
    var detect = makeDetectTripPosition(ls);
    // Position just east of Elwood (past stop 1, heading toward SLC)
    var result = detect(41.99, -99.5);
    assert.ok(result !== null);
    assert.strictEqual(result.day, 2, 'past first stop → Day 2');
});

test('near destination → last leg day', function() {
    var ls = makeLocalStorage({
        stopCoords:    JSON.stringify(FAKE_COORDS),
        stopCumMeters: JSON.stringify(FAKE_CUM),
        progressUnit:  '0'
    });
    var detect = makeDetectTripPosition(ls);
    var result = detect(45.52, -122.68);  // at Portland exactly
    assert.ok(result !== null);
    assert.strictEqual(result.day, FAKE_COORDS.length - 1, 'at destination → last leg day');
});

/* ---- */
console.log('\n[detectTripPosition — Portsmouth OH → Kansas City MO → Denver CO]');

/*
 * Two-leg road trip:
 *   Day 1: Portsmouth, OH → Kansas City, MO   (~623 mi driving; haversine ~623 mi)
 *   Day 2: Kansas City, MO → Denver, CO        (~556 mi driving; haversine ~557 mi)
 *
 * Cumulative distances use haversine (straight-line) so they stay consistent
 * without needing real OSRM data. Detection logic is tested, not OSRM values.
 *
 * Test positions and expected day/distance:
 *   Cincinnati, OH  (39.10, -84.51) — Day 1, ~86 mi from start  (near Portsmouth)
 *   St. Louis, MO   (38.63, -90.20) — Day 1, ~388 mi from start (past halfway)
 *   Goodland, KS    (39.35,-101.71) — Day 2, ~990 mi from start (2/3 into Day 2)
 */

var PKD_COORDS = [
    { lat: 38.72, lon:  -82.99 }, // Portsmouth, OH
    { lat: 39.10, lon:  -94.58 }, // Kansas City, MO
    { lat: 39.74, lon: -104.99 }  // Denver, CO
];

var PKD_CUM = (function() {
    var d01 = haversineMeters(PKD_COORDS[0].lat, PKD_COORDS[0].lon,
                              PKD_COORDS[1].lat, PKD_COORDS[1].lon);
    var d12 = haversineMeters(PKD_COORDS[1].lat, PKD_COORDS[1].lon,
                              PKD_COORDS[2].lat, PKD_COORDS[2].lon);
    return [0, d01, d01 + d12];
}());

console.log('  Portsmouth→KC: ' + Math.round(PKD_CUM[1]/1609.344) + ' mi straight-line');
console.log('  KC→Denver:     ' + Math.round((PKD_CUM[2]-PKD_CUM[1])/1609.344) + ' mi straight-line');
console.log('  Total:         ' + Math.round(PKD_CUM[2]/1609.344) + ' mi straight-line');

function makePkdStore() {
    return makeLocalStorage({
        stopCoords:    JSON.stringify(PKD_COORDS),
        stopCumMeters: JSON.stringify(PKD_CUM),
        progressUnit:  '0'
    });
}

test('PKD: Portsmouth→KC haversine ≈ 600–650 mi', function() {
    var miles = PKD_CUM[1] / 1609.344;
    assert.ok(miles > 600 && miles < 650,
        'Portsmouth→KC expected 600–650 mi straight-line, got ' + Math.round(miles) + ' mi');
});

test('PKD: KC→Denver haversine ≈ 530–580 mi', function() {
    var miles = (PKD_CUM[2] - PKD_CUM[1]) / 1609.344;
    assert.ok(miles > 530 && miles < 580,
        'KC→Denver expected 530–580 mi straight-line, got ' + Math.round(miles) + ' mi');
});

test('PKD: Cincinnati OH → Day 1, 70–115 mi from start', function() {
    // Cincinnati is ~90 mi NW of Portsmouth — early on Day 1
    var detect = makeDetectTripPosition(makePkdStore());
    var result = detect(39.10, -84.51);
    assert.ok(result !== null, 'should detect position');
    assert.strictEqual(result.day, 1, 'Cincinnati is on Day 1 (Portsmouth→KC leg)');
    assert.ok(result.distance > 70 && result.distance < 115,
        'Cincinnati expected 70–115 mi from start, got ' + result.distance + ' mi');
});

test('PKD: St. Louis MO → Day 1, 350–430 mi from start', function() {
    // St. Louis is past the midpoint of the Portsmouth→KC leg
    var detect = makeDetectTripPosition(makePkdStore());
    var result = detect(38.63, -90.20);
    assert.ok(result !== null, 'should detect position');
    assert.strictEqual(result.day, 1, 'St. Louis is on Day 1 (Portsmouth→KC leg)');
    assert.ok(result.distance > 350 && result.distance < 430,
        'St. Louis expected 350–430 mi from start, got ' + result.distance + ' mi');
});

test('PKD: Goodland KS → Day 2, 940–1060 mi from start', function() {
    // Goodland, KS is roughly 2/3 of the way through the KC→Denver leg
    var detect = makeDetectTripPosition(makePkdStore());
    var result = detect(39.35, -101.71);
    assert.ok(result !== null, 'should detect position');
    assert.strictEqual(result.day, 2, 'Goodland is on Day 2 (KC→Denver leg)');
    assert.ok(result.distance > 940 && result.distance < 1060,
        'Goodland expected 940–1060 mi from start, got ' + result.distance + ' mi');
});

test('PKD: at Portsmouth exactly → Day 1, 0 mi', function() {
    var detect = makeDetectTripPosition(makePkdStore());
    var result = detect(38.72, -82.99);
    assert.ok(result !== null);
    assert.strictEqual(result.day, 1, 'at start → Day 1');
    assert.ok(result.distance < 5, 'at Portsmouth → < 5 mi from start');
});

test('PKD: at Denver exactly → Day 2, near total distance', function() {
    var detect = makeDetectTripPosition(makePkdStore());
    var totalMiles = Math.round(PKD_CUM[2] / 1609.344);
    var result = detect(39.74, -104.99);
    assert.ok(result !== null);
    assert.strictEqual(result.day, 2, 'at destination → last day');
    assert.ok(result.distance >= totalMiles - 5,
        'at Denver → near total distance (' + totalMiles + ' mi), got ' + result.distance);
});

/* ---- */
console.log('\n[Photon geocoder response format]');

/*
 * Photon (photon.komoot.io) returns GeoJSON where coordinates are [longitude, latitude].
 * Our geocodeCity function extracts coords[1] as lat and coords[0] as lon.
 * These tests verify the extraction logic with realistic Photon responses.
 */

function parsePhotonCoords(responseText) {
    var data = JSON.parse(responseText);
    if (data.features && data.features.length > 0) {
        var coords = data.features[0].geometry.coordinates; // [lon, lat]
        return { lat: parseFloat(coords[1]), lon: parseFloat(coords[0]) };
    }
    return null;
}

var PHOTON_PORTSMOUTH = JSON.stringify({
    'features': [{
        'geometry': { 'type': 'Point', 'coordinates': [-82.9977, 38.7318] },
        'properties': { 'name': 'Portsmouth', 'state': 'Ohio', 'country': 'United States' }
    }]
});

var PHOTON_EMPTY = JSON.stringify({ 'features': [] });

test('Photon: extracts lat correctly (index 1)', function() {
    var result = parsePhotonCoords(PHOTON_PORTSMOUTH);
    assert.ok(result !== null, 'should return coords');
    assert.ok(Math.abs(result.lat - 38.7318) < 0.001,
        'lat should be ~38.73, got ' + result.lat);
});

test('Photon: extracts lon correctly (index 0)', function() {
    var result = parsePhotonCoords(PHOTON_PORTSMOUTH);
    assert.ok(result !== null, 'should return coords');
    assert.ok(Math.abs(result.lon - (-82.9977)) < 0.001,
        'lon should be ~-82.99, got ' + result.lon);
});

test('Photon: returns null for empty features', function() {
    var result = parsePhotonCoords(PHOTON_EMPTY);
    assert.strictEqual(result, null, 'empty features → null');
});

test('Photon: lat/lon are NOT swapped (lon is negative for US cities)', function() {
    var result = parsePhotonCoords(PHOTON_PORTSMOUTH);
    assert.ok(result.lon < 0,   'US city longitude should be negative');
    assert.ok(result.lat > 0,   'US city latitude should be positive');
    assert.ok(result.lat < 90,  'latitude must be < 90');
    assert.ok(result.lon > -180,'longitude must be > -180');
});

/* ---- */
console.log('\n[sendDistances — partial geocoding failure]');

test('partial failure: 2 of 3 legs succeed → should still send', function() {
    var ls = makeLocalStorage({ progressUnit: '0' });
    var pebble = makePebbleMock();
    var sendDistances = makeSendDistances(ls, pebble);
    // Day 1 and Day 2 geocoded OK, Day 3 not configured (empty leg)
    var legs = [1094400, 896000, 0, 0, 0];  // Portsmouth→KC, KC→Denver, rest 0
    var sent = sendDistances(legs);
    assert.strictEqual(sent, true, 'partial success should still send');
    assert.strictEqual(pebble.messages.length, 1);
    var msg = pebble.messages[0];
    assert.ok(msg['CalcTotalDist'] > 0, 'CalcTotalDist should be > 0');
    assert.ok(msg['CalcDayDist1']  > 0, 'Day 1 leg should be > 0');
    assert.ok(msg['CalcDayDist2']  > 0, 'Day 2 leg should be > 0');
    assert.strictEqual(msg['CalcDayDist3'], 0, 'Day 3 unused → 0');
});

test('all legs 0 → skip send (original bug scenario)', function() {
    var ls = makeLocalStorage({ progressUnit: '0' });
    var pebble = makePebbleMock();
    var sendDistances = makeSendDistances(ls, pebble);
    var sent = sendDistances([0, 0, 0, 0, 0]);
    assert.strictEqual(sent, false, 'all-zero should be skipped');
    assert.strictEqual(pebble.messages.length, 0, 'no message should be sent');
});

/* ---- */
console.log('\n[detectTripPosition — edge cases]');

/* 1-day trip: Portsmouth → Portland (no overnight stops) */
var ONE_DAY_COORDS = [
    { lat: 38.72,  lon:  -82.99 }, // Portsmouth, OH
    { lat: 45.52,  lon: -122.68 }  // Portland, OR
];
var ONE_DAY_CUM = [0, haversineMeters(38.72, -82.99, 45.52, -122.68)];

function makeOneDayStore() {
    return makeLocalStorage({
        stopCoords:    JSON.stringify(ONE_DAY_COORDS),
        stopCumMeters: JSON.stringify(ONE_DAY_CUM),
        progressUnit:  '0'
    });
}

test('1-day trip: at start → Day 1, 0 mi', function() {
    var result = makeDetectTripPosition(makeOneDayStore())(38.72, -82.99);
    assert.ok(result !== null);
    assert.strictEqual(result.day, 1);
    assert.ok(result.distance < 5, 'at start → < 5 mi');
});

test('1-day trip: at destination → Day 1, near total', function() {
    var result = makeDetectTripPosition(makeOneDayStore())(45.52, -122.68);
    var totalMi = Math.round(ONE_DAY_CUM[1] / 1609.344);
    assert.ok(result !== null);
    assert.strictEqual(result.day, 1, '1-day trip always Day 1');
    assert.ok(result.distance >= totalMi - 5, 'at destination → near total mi');
});

test('1-day trip: intermediate position → Day 1', function() {
    // Kansas City is roughly 55% of the way from Portsmouth to Portland
    var result = makeDetectTripPosition(makeOneDayStore())(39.10, -94.58);
    assert.ok(result !== null);
    assert.strictEqual(result.day, 1, 'always Day 1 on a 1-day trip');
    assert.ok(result.distance > 0, 'intermediate position > 0 mi');
});

test('no route cached → returns null', function() {
    var result = makeDetectTripPosition(makeLocalStorage({}))(39.10, -84.51);
    assert.strictEqual(result, null, 'no cached route → null');
});

test('only 1 stop cached (invalid route) → returns null', function() {
    var ls = makeLocalStorage({
        stopCoords:    JSON.stringify([{ lat: 38.72, lon: -82.99 }]),
        stopCumMeters: JSON.stringify([0]),
        progressUnit:  '0'
    });
    var result = makeDetectTripPosition(ls)(38.72, -82.99);
    assert.strictEqual(result, null, 'only 1 stop → cannot determine leg → null');
});

/* ---- */
console.log('\n[getStoredStr — additional edge cases]');

test('returns empty string for key with empty-string value', function() {
    var ls = makeLocalStorage({ startName: '' });
    var getStoredStr = makeGetStoredStr(ls);
    assert.strictEqual(getStoredStr('startName', 'StartName'), '', 'empty value → ""');
});

test('null fallback key also absent → empty string', function() {
    var ls = makeLocalStorage({});
    var getStoredStr = makeGetStoredStr(ls);
    assert.strictEqual(getStoredStr('x', 'y'), '', 'both absent → ""');
});

/* ---- */
console.log('\n[haversine precision]');

test('haversine symmetric: A→B = B→A', function() {
    var ab = haversineMeters(38.72, -82.99, 39.10, -94.58);
    var ba = haversineMeters(39.10, -94.58, 38.72, -82.99);
    assert.strictEqual(ab, ba, 'haversine must be symmetric');
});

test('haversine: equatorial degree ≈ 111.32 km', function() {
    // 1° longitude at equator ≈ 111,320 m
    var d = haversineMeters(0, 0, 0, 1);
    assert.ok(d > 111000 && d < 111700,
        'equatorial 1° expected ~111.32km, got ' + Math.round(d/1000) + 'km');
});

test('haversine: meridional degree ≈ 110.57 km', function() {
    // 1° latitude ≈ 110,570 m (slightly less than equatorial)
    var d = haversineMeters(0, 0, 1, 0);
    assert.ok(d > 110000 && d < 111200,
        'meridional 1° expected ~110.57km, got ' + Math.round(d/1000) + 'km');
});

/* ---- Summary ---- */
console.log('\n' + passed + ' passed, ' + failed + ' failed');
process.exit(failed > 0 ? 1 : 0);
