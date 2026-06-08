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

/* ---- Summary ---- */
console.log('\n' + passed + ' passed, ' + failed + ' failed');
process.exit(failed > 0 ? 1 : 0);
