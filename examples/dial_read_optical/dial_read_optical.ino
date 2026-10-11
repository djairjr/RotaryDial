/**
 * dial_read_optical.ino — example for an optical encoder (Djair's disc).
 *
 * Reads digits from a telephone dial adapted with an optical sensor.
 *
 * ── HARDWARE ────────────────────────────────────────────
 *   Physical disc: 10 uniform slits + resting area (~1/4 of the disc).
 *   Optical sensor (TCRT5000 or similar) on the interrupt pin.
 *   INPUT_PULLUP enabled internally.
 *
 * ── Default pin per platform ────────────────────────────
 *   ESP32  → GPIO 0      RP2040 → D0 (GPIO 0)      AVR → 2 (INT0)
 *   To use another pin: RotaryDialUnified::beginOptical(MY_PIN);
 *
 * ── DISC TIMING ─────────────────────────────────────────
 *   This disc does NOT follow the traditional telephone pattern.
 *   It was created to rescue damaged mechanical dials.
 *
 *   Pulse pattern (measured with pulse_timing.ino):
 *     Digit 1 → 148
 *     Digit 2 → 121 58
 *     Digit 3 → 140 59 52
 *     ...
 *
 *     The FIRST highLen of each line is the GAP (resting area).
 *     The rest are SLITS (slots in the disc).
 *
 *   Gap  (resting area):      ~100-176 ms
 *   Slit (slot in the disc):  ~52-62 ms
 *
 *   Gap and slit have DIFFERENT but OVERLAPPING durations
 *   in the 50-200ms range. They cannot be told apart by
 *   duration — the library uses the time distance instead:
 *   if > DIGITEND since the last pulse → GAP (ignore),
 *   otherwise → SLIT (count).
 *
 * ── TIMING PARAMETERS (defined in the library) ──────────
 *   MS_DEBOUNCE = 50 ms  — floor: ignores pulses < 50ms
 *                            (filters noise and return pulses)
 *   MS_MAXGAP   = 0      — ceiling: off
 *                            (gap and slit share the same range)
 *   MS_DIGITEND = 800 ms — timeout: closes the digit after
 *                            800ms without a new pulse
 *
 *   These values can be adjusted at runtime:
 *     RotaryDialUnified::setDebounce(ms);
 *     RotaryDialUnified::setMaxGap(ms);
 *     RotaryDialUnified::setDigitEnd(ms);
 *
 * ── OUTPUT ──────────────────────────────────────────────
 *   Serial 115200 baud. Prints digits until the timeout.
 */

#include <RotaryDialUnified.h>

// ── Sketch parameters ────────────────────────────────────
// Time without a new digit before printing a line break
#define ROTARY_DIAL_TIMEOUT  3000   // ms

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("Optical RotaryDialUnified (Djair's disc) — ready."));
    Serial.println(F("  Gap ~100-176ms | Slit ~52-62ms | Debounce 50ms | DigitEnd 800ms"));
    Serial.println(F("Dial the digits..."));
    RotaryDialUnified::beginOptical();   // default pin for the platform
}

unsigned long lastDigitTime = 0;
bool newline = true;

void loop() {
    if (RotaryDialUnified::available()) {
        int digit = RotaryDialUnified::read();
        Serial.print(digit);
        lastDigitTime = millis();
        newline = false;
    }

    if (!newline && millis() - lastDigitTime > ROTARY_DIAL_TIMEOUT) {
        Serial.println(F(" OK"));
        newline = true;
    }
}
