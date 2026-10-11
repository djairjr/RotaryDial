/**
 * pulse_timing.ino — Pure debug: prints highLen (ms) on every valid pulse.
 *
 * Based on the DEBUG_PULSE of the original RotaryDial.
 * On every valid FALLING edge, prints the time the signal stayed HIGH.
 * When the sensor stays idle for MS_PAUSE, breaks the line.
 *
 * Useful for calibration and to understand the behavior of your disc.
 *
 * ── HARDWARE ────────────────────────────────────────────
 *   Sensor/contact on the interrupt pin, INPUT_PULLUP.
 *
 * ── Pin per platform ─────────────────────────────────
 *   ESP32  → GPIO 0      RP2040 → D0 (GPIO 0)      AVR → 2 (INT0)
 *
 * ── TIMING: WHAT EACH DEFINE MEANS ─────────────────
 *
 *   MS_DEBOUNCE = 52 ms  — FLOOR.
 *     Ignores any highLen smaller than this value.
 *     Filters noise and bounce.
 *     On Djair's optical disc, it filters return pulses
 *     (fast return, <50ms) and false triggers.
 *     ⚠ Tuning: if you lose pulses, lower it. If you capture noise, raise it.
 *
 *   MS_PAUSE = 800 ms    — LINE TIMEOUT.
 *     If this time passes with no new pulse,
 *     it breaks the line on the Serial (closes the group).
 *     Does not affect the count — it is only cosmetic.
 *     ⚠ To see pulses without automatic line breaks, use 9999.
 *
 * ── INTERPRETING THE OUTPUT ──────────────────────────────
 *   Each line = one digit dialed.
 *   The FIRST number on the line is the GAP (resting area).
 *   The following numbers are the SLITS of the disc.
 *
 *   Example with Djair's disc:
 *     148           ← digit 1: gap=148ms, 0 slits
 *     121 58        ← digit 2: gap=121ms, 1 slit=58ms
 *     140 59 52     ← digit 3: gap=140ms, 2 slits=59+52ms
 *
 * ── OUTPUT ────────────────────────────────────────────────
 *   Serial 115200 baud.
 */

// ── Pin per platform (standalone, does not depend on the library) ──
#if defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) ||     defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) ||     defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
  const int PIN = 0;        // ESP32: GPIO0
#elif defined(ARDUINO_ARCH_RP2040)
  const int PIN = D0;       // RP2040: D0 = GPIO0
#elif defined(ARDUINO_ARCH_AVR)
  const int PIN = 2;        // AVR: INT0
#else
  const int PIN = 0;        // fallback
#endif

// ═══════════════════════════════════════════════════════════
// TIMING PARAMETERS — TUNE FOR YOUR DIAL
// ═══════════════════════════════════════════════════════════

// FLOOR: ignores pulses shorter than this value (ms).
//       Filters noise, bounce, and spurious return pulses.
#define MS_DEBOUNCE  52

// LINE TIMEOUT: time with no pulse to break the line (ms).
//       Only cosmetic — does not affect the count.
#define MS_PAUSE     800

volatile unsigned long lastHighMs = 0;
volatile int  lastState = HIGH;
volatile int  pulseCount = 0;
volatile bool lineStarted = false;

void onPulse() {
    int state = digitalRead(PIN);

    if (state == lastState) return;
    lastState = state;

    if (state == HIGH) {
        lastHighMs = millis();
    } else {
        unsigned long highLen = millis() - lastHighMs;
        if (highLen > MS_DEBOUNCE) {
            if (lineStarted) Serial.print(' ');
            Serial.print(highLen);
            lineStarted = true;
            pulseCount++;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("Pulse Timing Debug — ready."));
    Serial.print(F("  MS_DEBOUNCE="));
    Serial.print(MS_DEBOUNCE);
    Serial.print(F("ms | MS_PAUSE="));
    Serial.print(MS_PAUSE);
    Serial.println(F("ms"));
    Serial.println(F("Dial to see the highLen (ms)..."));
    Serial.println(F("  (first number = gap | the rest = slits)"));

    pinMode(PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN), onPulse, CHANGE);
}

void loop() {
    if (pulseCount > 0 && millis() - lastHighMs > MS_PAUSE) {
        noInterrupts();
        pulseCount = 0;
        interrupts();
        Serial.println();
        lineStarted = false;
    }
}
