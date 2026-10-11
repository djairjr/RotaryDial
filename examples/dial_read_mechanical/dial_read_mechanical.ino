/**
 * dial_read_mechanical.ino — example for a mechanical dial (DEFAULT).
 *
 * Reads digits from a traditional telephone dial (metal contacts).
 *
 * ── HARDWARE ────────────────────────────────────────────
 *   Dial contact on the interrupt pin.
 *   INPUT_PULLUP enabled internally.
 *
 * ── Default pin per platform ────────────────────────────
 *   ESP32  → GPIO 0      RP2040 → D0 (GPIO 0)      AVR → 2 (INT0)
 *   To use another pin: RotaryDial::begin(MY_PIN);
 *
 * ── TIMING PARAMETERS (defined in the library) ──────────
 *   MS_DEBOUNCE = 15 ms  — floor: ignores pulses < 15ms
 *                            (filters out contact bounce)
 *   MS_MAXGAP   = 0      — ceiling: off
 *                            (a mechanical dial does not need it)
 *   MS_DIGITEND = 600 ms — timeout: closes the digit after
 *                            600ms without a new pulse
 *
 *   These values can be adjusted at runtime:
 *     RotaryDial::setDebounce(ms);
 *     RotaryDial::setMaxGap(ms);
 *     RotaryDial::setDigitEnd(ms);
 *
 * ── OUTPUT ──────────────────────────────────────────────
 *   Serial 115200 baud. Prints digits until the timeout.
 */

#include <RotaryDial.h>

// ── Sketch parameters ────────────────────────────────────
// Time without a new digit before printing a line break
#define ROTARY_DIAL_TIMEOUT  3000   // ms

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("Mechanical RotaryDial (default) — ready."));
    Serial.println(F("  Debounce 15ms | MaxGap off | DigitEnd 600ms"));
    Serial.println(F("Dial the digits..."));
    RotaryDial::begin();   // default pin, MECHANICAL mode
}

unsigned long lastDigitTime = 0;
bool newline = true;

void loop() {
    if (RotaryDial::available()) {
        int digit = RotaryDial::read();
        Serial.print(digit);
        lastDigitTime = millis();
        newline = false;
    }

    if (!newline && millis() - lastDigitTime > ROTARY_DIAL_TIMEOUT) {
        Serial.println(F(" OK"));
        newline = true;
    }
}
