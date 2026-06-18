var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// ---- HTTP helper ----
function xhrGet(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() { callback(this.responseText); };
    xhr.onerror = function() { callback(null); };
    xhr.open('GET', url);
    try { xhr.setRequestHeader('User-Agent', 'PebbleRoadTripWatchface/1.0'); } catch(e) {}
    xhr.send();
}

// ---- Sync Clay settings to our localStorage keys at startup ----
// Clay flattens all settings (extracting .value from each) and stores the result
// in localStorage under 'clay-settings' (with a hyphen). Values are plain strings/numbers.
function syncSettingsFromClay() {
    try {
        var raw = localStorage.getItem('clay-settings');
        if (!raw) {
            console.log('syncSettingsFromClay: clay-settings not found');
            return;
        }
        var s = JSON.parse(raw);
        if (s['StartName']) localStorage.setItem('startName', '' + s['StartName']);
        if (s['DestName'])  localStorage.setItem('destName',  '' + s['DestName']);
        for (var i = 1; i <= 5; i++) {
            var k = 'Waypoint' + i;
            if (s[k]) localStorage.setItem('waypoint' + i, '' + s[k]);
        }
        if (s['ProgressUnit'] !== undefined) localStorage.setItem('progressUnit', '' + s['ProgressUnit']);
        if (s['TempUnit']     !== undefined) localStorage.setItem('tempUnit',     '' + s['TempUnit']);
        if (s['ShowWeather']  !== undefined) localStorage.setItem('showWeather',  '' + s['ShowWeather']);
        if (s['Theme']        !== undefined) localStorage.setItem('theme',        '' + s['Theme']);
        if (s['AutoTheme']    !== undefined) localStorage.setItem('autoTheme',    '' + s['AutoTheme']);
        console.log('syncSettingsFromClay: start=' + s['StartName'] + ' dest=' + s['DestName']);
    } catch(e) {
        console.log('syncSettingsFromClay error: ' + e);
    }
}

// ---- Geocoding (Photon by Komoot — OSM data, no API key, no User-Agent gate) ----
// Nominatim was replaced because it rejects requests without a valid User-Agent
// header, which Pebble JS cannot set. Photon has the same OSM data with no such
// restriction. GeoJSON coordinates are [longitude, latitude] order.
function geocodeCity(name, callback) {
    if (!name || name.trim() === '') { callback(null, null); return; }
    var url = 'https://photon.komoot.io/api/?q=' + encodeURIComponent(name.trim()) + '&limit=1';
    xhrGet(url, function(response) {
        if (!response) { callback(null, null); return; }
        try {
            var data = JSON.parse(response);
            if (data.features && data.features.length > 0) {
                var coords = data.features[0].geometry.coordinates; // [lon, lat]
                callback(parseFloat(coords[1]), parseFloat(coords[0]));
            } else {
                console.log('No geocode result for: ' + name);
                callback(null, null);
            }
        } catch(e) {
            console.log('Geocode parse error: ' + e);
            callback(null, null);
        }
    });
}

// ---- Routing (OSRM public demo) ----
function getDrivingMeters(lat1, lon1, lat2, lon2, callback) {
    var url = 'https://router.project-osrm.org/route/v1/driving/' +
              lon1 + ',' + lat1 + ';' + lon2 + ',' + lat2 + '?overview=false';
    xhrGet(url, function(response) {
        if (!response) { callback(null); return; }
        try {
            var data = JSON.parse(response);
            if (data.code === 'Ok' && data.routes && data.routes.length > 0) {
                callback(data.routes[0].distance);
            } else {
                console.log('OSRM error: ' + (data.code || 'unknown'));
                callback(null);
            }
        } catch(e) {
            console.log('OSRM parse error: ' + e);
            callback(null);
        }
    });
}

// ---- Trip Distance Calculation ----
// Reads from our localStorage keys with Clay-key fallback.
function getStoredStr(key1, key2) {
    return localStorage.getItem(key1) || localStorage.getItem(key2) || '';
}

function calculateTripDistances() {
    var start = getStoredStr('startName', 'StartName');
    var dest  = getStoredStr('destName',  'DestName');
    if (!start || !dest) {
        console.log('calculateTripDistances: start/dest not set');
        return;
    }

    var waypoints = [];
    for (var i = 1; i <= 5; i++) {
        var wp = getStoredStr('waypoint' + i, 'Waypoint' + i);
        if (wp.trim() !== '') waypoints.push(wp.trim());
    }

    var stops = [start].concat(waypoints).concat([dest]);
    var n = stops.length;
    var coords = new Array(n);

    console.log('Geocoding ' + n + ' locations...');

    function geocodeNext(idx) {
        if (idx >= n) {
            // Cache coords for position detection
            localStorage.setItem('stopCoords', JSON.stringify(coords));
            computeLegs(coords);
            return;
        }
        geocodeCity(stops[idx], function(lat, lon) {
            coords[idx] = (lat !== null) ? { lat: lat, lon: lon } : null;
            console.log('Geocoded[' + idx + '] ' + stops[idx] + ' -> ' +
                        (lat !== null ? lat + ',' + lon : 'FAILED'));
            setTimeout(function() { geocodeNext(idx + 1); }, 350);
        });
    }
    geocodeNext(0);
}

function computeLegs(coords) {
    var numLegs = coords.length - 1;
    if (numLegs <= 0) return;

    var legMeters = [0, 0, 0, 0, 0];

    function computeNext(legIdx) {
        if (legIdx >= numLegs) {
            // Build and cache cumulative distances for GPS position detection
            var cumMeters = [0];
            for (var i = 0; i < numLegs; i++) {
                cumMeters.push(cumMeters[i] + (legMeters[i] || 0));
            }
            localStorage.setItem('stopCumMeters', JSON.stringify(cumMeters));
            sendDistances(legMeters);
            return;
        }
        var c1 = coords[legIdx], c2 = coords[legIdx + 1];
        if (c1 && c2) {
            getDrivingMeters(c1.lat, c1.lon, c2.lat, c2.lon, function(meters) {
                if (meters !== null) {
                    legMeters[legIdx] = meters;
                    console.log('Leg ' + legIdx + ': ' +
                                Math.round(meters / 1609.344) + ' mi (' +
                                Math.round(meters / 1000) + ' km)');
                }
                computeNext(legIdx + 1);
            });
        } else {
            console.log('Leg ' + legIdx + ': skipped (missing coords)');
            computeNext(legIdx + 1);
        }
    }
    computeNext(0);
}

function sendDistances(legMeters) {
    var unit = parseInt(localStorage.getItem('progressUnit') || '0');

    function toUnit(meters) {
        return (unit === 1) ? Math.round(meters / 1000) : Math.round(meters / 1609.344);
    }

    var totalMeters = legMeters.reduce(function(sum, m) { return sum + m; }, 0);

    // Sending CalcTotalDist=0 (from failed geocoding) would permanently corrupt
    // total_distance on the watch, making all progress calculations show 0%.
    if (totalMeters === 0) {
        console.log('sendDistances: totalMeters=0, geocoding likely failed — skipping');
        return;
    }

    var dict = {
        'CalcTotalDist': toUnit(totalMeters),
        'CalcDayDist1':  toUnit(legMeters[0] || 0),
        'CalcDayDist2':  toUnit(legMeters[1] || 0),
        'CalcDayDist3':  toUnit(legMeters[2] || 0),
        'CalcDayDist4':  toUnit(legMeters[3] || 0),
        'CalcDayDist5':  toUnit(legMeters[4] || 0),
        'CalcDayDist6':  toUnit(legMeters[5] || 0)
    };

    console.log('Sending distances: total=' + dict['CalcTotalDist']);

    Pebble.sendAppMessage(dict,
        function() {
            console.log('Distances sent OK');
            // Route coords are now cached — get GPS and send position.
            // Delay 200ms so the outbox is clear before the next message.
            setTimeout(detectAndSendPositionOnly, 200);
        },
        function(e) { console.log('Distances send failed: ' + JSON.stringify(e)); }
    );
}

// Gets current GPS and sends TripDay + CurrentDistance independently of weather.
// Called after routing completes so stopCoords/stopCumMeters are guaranteed cached.
function detectAndSendPositionOnly() {
    navigator.geolocation.getCurrentPosition(
        function(pos) {
            var detection = detectTripPosition(pos.coords.latitude, pos.coords.longitude);
            if (!detection) {
                console.log('detectAndSend: no route data cached');
                return;
            }
            console.log('Auto-position: day=' + detection.day + ' dist=' + detection.distance);
            Pebble.sendAppMessage(
                { 'TripDay': detection.day, 'CurrentDistance': detection.distance },
                function() { console.log('Position sent OK'); },
                function(e) { console.log('Position send failed: ' + JSON.stringify(e)); }
            );
        },
        function(err) { console.log('GPS unavailable for position detect: ' + err.message); },
        { timeout: 15000 }
    );
}

// ---- GPS Trip Position Detection ----
function haversineMeters(lat1, lon1, lat2, lon2) {
    var R = 6371000;
    var p1 = lat1 * Math.PI / 180, p2 = lat2 * Math.PI / 180;
    var dp = (lat2 - lat1) * Math.PI / 180;
    var dl = (lon2 - lon1) * Math.PI / 180;
    var a = Math.sin(dp/2)*Math.sin(dp/2) +
            Math.cos(p1)*Math.cos(p2)*Math.sin(dl/2)*Math.sin(dl/2);
    return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
}

// Returns {day (1-based), distance (in user's unit)} or null if route not cached.
function detectTripPosition(currentLat, currentLon) {
    try {
        var coordsJson  = localStorage.getItem('stopCoords');
        var cumJson     = localStorage.getItem('stopCumMeters');
        if (!coordsJson || !cumJson) return null;

        var allCoords = JSON.parse(coordsJson);
        var allCum    = JSON.parse(cumJson);

        // Build list of stops that successfully geocoded
        var stops = [];
        for (var i = 0; i < allCoords.length; i++) {
            if (allCoords[i] && allCum[i] !== undefined) {
                stops.push({
                    lat:  allCoords[i].lat,
                    lon:  allCoords[i].lon,
                    cumM: allCum[i]
                });
            }
        }
        if (stops.length < 2) return null;

        // Distance from current position to each stop
        var dists = stops.map(function(s) {
            return haversineMeters(currentLat, currentLon, s.lat, s.lon);
        });

        // Find nearest stop
        var nearIdx = 0;
        for (var j = 1; j < stops.length; j++) {
            if (dists[j] < dists[nearIdx]) nearIdx = j;
        }

        // Determine which leg (0-based index) we're on
        var legIdx;
        if (nearIdx === stops.length - 1) {
            // Near final destination — last leg
            legIdx = stops.length - 2;
        } else if (nearIdx === 0) {
            // Near start — first leg
            legIdx = 0;
        } else {
            // Check if we're heading toward next stop or the previous one
            legIdx = (dists[nearIdx + 1] < dists[nearIdx - 1]) ? nearIdx : nearIdx - 1;
        }
        if (legIdx < 0) legIdx = 0;
        if (legIdx >= stops.length - 1) legIdx = stops.length - 2;

        var day = legIdx + 1;  // 1-based day number

        // Estimate how far along this leg we are using linear interpolation
        var legStart = stops[legIdx];
        var legEnd   = stops[legIdx + 1];
        var dFromStart = haversineMeters(currentLat, currentLon, legStart.lat, legStart.lon);
        var dToEnd     = haversineMeters(currentLat, currentLon, legEnd.lat,   legEnd.lon);
        var legLen     = legEnd.cumM - legStart.cumM;

        var frac = (dFromStart + dToEnd > 1) ? dFromStart / (dFromStart + dToEnd) : 0;
        if (frac > 1) frac = 1;
        var totalM = legStart.cumM + frac * legLen;

        // Snap to exact leg endpoint when GPS/haversine puts us within 10km of a stop.
        // Without this, rounding at the destination yields 99% instead of 100%.
        var SNAP_M = 10000;
        if (dToEnd < SNAP_M && dToEnd < dFromStart) {
            totalM = legEnd.cumM;
        } else if (dFromStart < SNAP_M && dFromStart < dToEnd) {
            totalM = legStart.cumM;
        }

        // Convert to user's unit (miles or km; percent handled by watch)
        var unit = parseInt(localStorage.getItem('progressUnit') || '0');
        var dist = (unit === 1) ? Math.round(totalM / 1000) : Math.round(totalM / 1609.344);

        console.log('Position: day=' + day + ' dist=' + dist +
                    ' (leg ' + legIdx + ', frac=' + frac.toFixed(2) + ')');
        return { day: day, distance: dist };

    } catch(ex) {
        console.log('detectTripPosition error: ' + ex);
        return null;
    }
}

// ---- Weather ----
function weatherCodeToInt(code) {
    if (code === 0)                       return 1;  // Clear
    if (code <= 3)                        return 2;  // Cloudy
    if (code >= 45 && code <= 48)         return 2;  // Foggy -> Cloudy
    if (code >= 51 && code <= 67)         return 3;  // Drizzle/Rain
    if (code >= 71 && code <= 77)         return 4;  // Snow
    if (code >= 80 && code <= 82)         return 3;  // Showers -> Rain
    if (code >= 95)                       return 5;  // Thunder
    return 2;
}

function fetchWeatherAt(lat, lon, callback) {
    var url = 'https://api.open-meteo.com/v1/forecast?' +
              'latitude=' + lat + '&longitude=' + lon +
              '&current=temperature_2m,weather_code';
    xhrGet(url, function(response) {
        if (!response) { callback(null, 0, -1); return; }
        try {
            var json = JSON.parse(response);
            var temp    = Math.round(json.current.temperature_2m);
            var rawCode = json.current.weather_code;
            var cond    = weatherCodeToInt(rawCode);
            callback(temp, cond, rawCode);
        } catch(e) {
            console.log('Weather parse error: ' + e);
            callback(null, 0, -1);
        }
    });
}

// Maps GPS lat/lon + temperature + raw WMO weather code to a theme index (0-6).
// City (4) is not detectable from weather data, so it never auto-selects.
function computeBiomeTheme(lat, lon, tempC, wmoCode) {
    var absLat  = Math.abs(lat);
    var isSnow  = (wmoCode >= 70 && wmoCode <= 79) || (wmoCode >= 85 && wmoCode <= 86);
    var isClear = (wmoCode >= 0 && wmoCode <= 3);

    // Snow precipitation or freezing temps → Winter
    if (isSnow || tempC <= -2) return 3;

    // Cold + high latitude → still Winter (polar tundra)
    if (tempC <= 5 && absLat >= 60) return 3;

    // Hot + clear + subtropical dry belt → Desert
    if (tempC >= 28 && isClear && absLat >= 8 && absLat <= 40) return 6;

    // Cold + clear + mid-to-high latitude → Mountain
    if (tempC <= 12 && isClear && absLat >= 38) return 2;

    // Warm + tropical or coastal latitudes → Beach
    if (tempC >= 24 && absLat < 40) return 1;

    // Warm + clear + temperate → Plains
    if (tempC >= 16 && isClear) return 5;

    // Warm regardless of cloud cover → Beach
    if (tempC >= 22) return 1;

    // Cool + clear + lower-mid latitude → Mountain
    if (isClear && absLat >= 30) return 2;

    // Default fallback
    return 0;  // Vaporwave
}

var s_curTemp = null, s_curCond = 0;
var s_dstTemp = null, s_dstCond = 0;
var s_curDone = false, s_dstDone = false;
var s_detectedDay = null, s_detectedDist = null;
var s_biomeTheme = null;  // non-null when auto-theme computed a result

// Returns a theme index to send only after 2 consecutive identical results,
// preventing flicker when temperature hovers near a biome boundary.
function stableAutoTheme(newTheme) {
    var last  = parseInt(localStorage.getItem('autoThemeLast')  || '-1');
    var count = parseInt(localStorage.getItem('autoThemeCount') || '0');
    if (newTheme === last) {
        count = Math.min(count + 1, 3);
    } else {
        count = 1;
    }
    localStorage.setItem('autoThemeLast',  '' + newTheme);
    localStorage.setItem('autoThemeCount', '' + count);
    return (count >= 2) ? newTheme : null;
}

function trySendWeather() {
    if (!s_curDone || !s_dstDone) return;

    var dict = {
        'TEMP_CURRENT': (s_curTemp !== null) ? s_curTemp : -99,
        'COND_CURRENT': s_curCond,
        'TEMP_DEST':    (s_dstTemp !== null) ? s_dstTemp : -99,
        'COND_DEST':    s_dstCond
    };

    // Piggyback auto-detected trip position and biome theme into the same message
    if (s_detectedDay  !== null) dict['TripDay']         = s_detectedDay;
    if (s_detectedDist !== null) dict['CurrentDistance'] = s_detectedDist;
    if (s_biomeTheme   !== null) dict['Theme']           = s_biomeTheme;

    console.log('Sending weather: cur=' + dict['TEMP_CURRENT'] +
                ' dst=' + dict['TEMP_DEST'] +
                (s_detectedDay !== null ? ' day=' + s_detectedDay : ''));

    Pebble.sendAppMessage(dict,
        function() { console.log('Weather+position sent OK'); },
        function(e) { console.log('Weather send failed: ' + JSON.stringify(e)); }
    );
}

// Returns the city name for tonight's overnight stop (waypoint for the current day),
// or the final destination if no waypoint is configured for that leg.
function getTonightDestName(day) {
    if (day !== null && day >= 1) {
        var wp = getStoredStr('waypoint' + day, 'Waypoint' + day);
        if (wp && wp.trim() !== '') return wp.trim();
    }
    return getStoredStr('destName', 'DestName');
}

// Geocodes destName and fetches weather there; sets s_dstDone when done.
function fetchDestWeather(destName) {
    if (!destName) {
        console.log('No destination name for weather');
        s_dstTemp = null; s_dstCond = 0;
        s_dstDone = true;
        trySendWeather();
        return;
    }
    geocodeCity(destName, function(lat, lon) {
        if (lat !== null) {
            fetchWeatherAt(lat, lon, function(temp, cond) {
                s_dstTemp = temp; s_dstCond = cond;
                s_dstDone = true;
                trySendWeather();
            });
        } else {
            console.log('Geocode failed for tonight dest: ' + destName);
            s_dstTemp = null; s_dstCond = 0;
            s_dstDone = true;
            trySendWeather();
        }
    });
}

function fetchBothWeather() {
    s_curDone = false; s_dstDone = false;
    s_detectedDay = null; s_detectedDist = null;
    s_biomeTheme = null;

    navigator.geolocation.getCurrentPosition(
        function(pos) {
            var lat = pos.coords.latitude, lon = pos.coords.longitude;

            // Auto-detect trip position to know which day we're on
            var detection = detectTripPosition(lat, lon);
            if (detection) {
                s_detectedDay  = detection.day;
                s_detectedDist = detection.distance;
                localStorage.setItem('lastTripDay', '' + detection.day);
            }

            // Current location weather (concurrent with dest fetch below)
            fetchWeatherAt(lat, lon, function(temp, cond, rawCode) {
                s_curTemp = temp; s_curCond = cond;
                var autoTheme = localStorage.getItem('autoTheme');
                if ((autoTheme === '1' || autoTheme === 'true') && temp !== null && rawCode >= 0) {
                    var candidate = computeBiomeTheme(lat, lon, temp, rawCode);
                    s_biomeTheme = stableAutoTheme(candidate);
                    console.log('Auto-theme: tempC=' + temp + ' wmo=' + rawCode +
                                ' → candidate=' + candidate +
                                (s_biomeTheme !== null ? ' (sending)' : ' (waiting for stability)'));
                }
                s_curDone = true;
                trySendWeather();
            });

            // Tonight's overnight stop weather (Day 1 end = waypoint1, Day 2 end = waypoint2, etc.)
            var tonightDest = getTonightDestName(s_detectedDay);
            console.log('Tonight dest: ' + tonightDest + ' (day=' + s_detectedDay + ')');
            fetchDestWeather(tonightDest);
        },
        function(err) {
            console.log('GPS unavailable: ' + err.message);
            s_curTemp = null; s_curCond = 0;
            s_curDone = true;

            // Use last known day (from previous successful GPS fix) or fall back to final dest
            var lastDay = parseInt(localStorage.getItem('lastTripDay') || '0') || null;
            var tonightDest = getTonightDestName(lastDay);
            console.log('GPS failed — tonight dest fallback: ' + tonightDest);
            fetchDestWeather(tonightDest);
        },
        { timeout: 15000 }
    );
}

// ---- Pebble Events ----

Pebble.addEventListener('ready', function() {
    console.log('PebbleKit JS ready!');
    // Sync Clay settings to our localStorage before fetching
    syncSettingsFromClay();
    fetchBothWeather();
    calculateTripDistances();
});

// Intercept Clay's webviewclosed to extract settings for pkjs use.
// Clay's own handler runs first (registered in the Clay constructor above), calls
// getSettings() which extracts .value from each raw setting object and stores the
// result as plain strings/numbers in localStorage['clay-settings'].
// We read from clay-settings so we get those plain values, not the raw objects.
Pebble.addEventListener('webviewclosed', function(e) {
    if (!e.response) return;
    try {
        var raw = localStorage.getItem('clay-settings');
        if (!raw) return;
        var settings = JSON.parse(raw);
        if (settings['StartName'] !== undefined) localStorage.setItem('startName', '' + settings['StartName']);
        if (settings['DestName']  !== undefined) localStorage.setItem('destName',  '' + settings['DestName']);
        for (var i = 1; i <= 5; i++) {
            var key = 'Waypoint' + i;
            if (settings[key] !== undefined) localStorage.setItem('waypoint' + i, '' + settings[key]);
        }
        if (settings['ProgressUnit'] !== undefined) localStorage.setItem('progressUnit', '' + settings['ProgressUnit']);
        if (settings['TempUnit']     !== undefined) localStorage.setItem('tempUnit',     '' + settings['TempUnit']);
        if (settings['ShowWeather']  !== undefined) localStorage.setItem('showWeather',  '' + settings['ShowWeather']);
        if (settings['Theme']        !== undefined) localStorage.setItem('theme',        '' + settings['Theme']);
        if (settings['AutoTheme']    !== undefined) localStorage.setItem('autoTheme',    '' + settings['AutoTheme']);

        console.log('Clay settings synced: start=' + settings['StartName'] + ' dest=' + settings['DestName']);
        calculateTripDistances();
        fetchBothWeather();
    } catch(ex) {
        console.log('webviewclosed sync error: ' + ex);
    }
});

Pebble.addEventListener('appmessage', function(e) {
    var payload = e.payload;

    // Watch→phone: REQUEST_WEATHER triggers a full refresh including GPS position
    if (payload['REQUEST_WEATHER']) {
        fetchBothWeather();
    }
});
