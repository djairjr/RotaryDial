/**
 * DialCalibrator.ino — Guided calibrator for a Rotary Dial (mechanical or optical).
 *
 * ISR identical to pulse_timing.ino (proven).
 * The user controls the advance with Enter — no automatic skips.
 * Each captured pulse appears on screen in real time.
 *
 * ── USAGE ─────────────────────────────────────────────────
 *   1. Connect the sensor/contact to the pin configured below.
 *   2. Open the Serial Monitor (115200 baud).
 *   3. Press Enter to start.
 *   4. Dial a digit, watch the pulses, press Enter.
 *   5. Repeat for all digits (1 to 0).
 *   6. At the end, see the analysis and the suggested values.
 *
 * ── OUTPUT ───────────────────────────────────────────────
 *   Serial 115200 baud.
 */

// ═══════════════════════════════════════════════════════════
// CONFIGURATION — TUNE HERE
// ═══════════════════════════════════════════════════════════

// Pin connected to the dial sensor/contact
const int PIN = 20;

// FLOOR (ms): ignores pulses shorter than this value.
// Filters noise, bounce and return pulses (<50ms).
// Start with 50. Raise it if you capture noise, lower it if you lose pulses.
#define MS_DEBOUNCE  50

// TIMEOUT (ms): maximum time waiting for the user's Enter.
// If nothing is typed during this time, it assumes you gave up.
#define MS_USER_TIMEOUT  120000   // 2 minutes

// ── Internal constants ──────────────────────────────────
#define MAX_PULSES    40
#define TOTAL_DIGITS  10

// ── ISR variables (identical to pulse_timing.ino) ─────
volatile unsigned long lastHighMs = 0;
volatile int  lastState = HIGH;
volatile int  pulseCount = 0;
volatile int  bufCount = 0;
volatile unsigned long buf[MAX_PULSES];

// ── Per-digit storage ─────────────────────────────
int   digitPulseCount[TOTAL_DIGITS];
unsigned long digitHighs[TOTAL_DIGITS][MAX_PULSES];

// ── ISR (IDENTICAL to pulse_timing.ino) ──────────────────

void onPulse() {
    int state = digitalRead(PIN);

    if (state == lastState) return;
    lastState = state;

    if (state == HIGH) {
        lastHighMs = millis();
    } else {
        unsigned long highLen = millis() - lastHighMs;
        if (highLen > MS_DEBOUNCE) {
            if (bufCount < MAX_PULSES) {
                buf[bufCount++] = highLen;
            }
            pulseCount++;
        }
    }
}

// ── Waits for the user to press Enter ────────────────────
// Returns true if Enter was pressed, false on timeout.

bool waitForEnter() {
    unsigned long start = millis();
    while (millis() - start < MS_USER_TIMEOUT) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                // Consumes the rest of the line
                while (Serial.available()) Serial.read();
                return true;
            }
        }
        delay(10);
    }
    return false;
}

// ── Consumes any pending character in the buffer ────────

void flushSerial() {
    while (Serial.available()) Serial.read();
}

// ── Prints the pulses accumulated so far ───────────────

void printPulsesSoFar() {
    noInterrupts();
    int n = bufCount;
    unsigned long localBuf[MAX_PULSES];
    for (int i = 0; i < n; i++) localBuf[i] = buf[i];
    interrupts();

    Serial.print(F("  Pulses: "));
    if (n == 0) {
        Serial.println(F("(none yet)"));
    } else {
        for (int i = 0; i < n; i++) {
            Serial.print(localBuf[i]);
            Serial.print(' ');
        }
        Serial.print(F(" ("));
        Serial.print(n);
        Serial.println(F(" total)"));
    }
}

// ── Setup ─────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    // Waits for the serial to stabilize (important for some boards)
    delay(1500);

    Serial.println(F("\n\n=========================================="));
    Serial.println(F("    ROTARY DIAL CALIBRATOR v4.0"));
    Serial.println(F("=========================================="));
    Serial.println(F("Based on pulse_timing.ino (proven)."));
    Serial.print(F("Pin: "));
    Serial.print(PIN);
    Serial.print(F(" | MS_DEBOUNCE="));
    Serial.print(MS_DEBOUNCE);
    Serial.println(F("ms"));
    Serial.println(F("\nThis script does NOT advance on its own."));
    Serial.println(F("You control the pace — press ENTER"));
    Serial.println(F("to confirm that you finished dialing.\n"));

    // Configures the pin and the interrupt
    pinMode(PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN), onPulse, CHANGE);

    // Waits for the user to start
    Serial.println(F("Press ENTER to start..."));
    flushSerial();
    if (!waitForEnter()) {
        Serial.println(F("Timeout. Restart the sketch."));
        while (1);
    }

    // ═══════════════════════════════════════════════════════
    // COLLECTION — DIGIT BY DIGIT
    // ═══════════════════════════════════════════════════════

    for (int i = 0; i < TOTAL_DIGITS; i++) {
        int expected = (i == 9) ? 0 : i + 1;

        // Resets the buffers for the new digit
        noInterrupts();
        pulseCount = 0;
        bufCount   = 0;
        interrupts();

        // Prompt
        Serial.println(F("\n------------------------------------------"));
        Serial.print(F(">>> DIAL THE NUMBER "));
        Serial.print(expected);
        Serial.println(F(" <<<"));
        Serial.println(F("Watch the pulses appearing below."));
        Serial.println(F("When you finish, press ENTER.\n"));

        // Wait loop — shows pulses in real time
        unsigned long lastPrint = 0;
        unsigned long start = millis();
        bool done = false;

        while (!done && millis() - start < MS_USER_TIMEOUT) {
            // Shows the pulses every ~500ms if there is something new
            if (millis() - lastPrint > 500) {
                printPulsesSoFar();
                lastPrint = millis();
            }

            // Checks whether the user pressed Enter
            if (Serial.available()) {
                char c = Serial.read();
                if (c == '\n' || c == '\r') {
                    while (Serial.available()) Serial.read();
                    done = true;
                }
            }

            delay(50);
        }

        if (!done) {
            Serial.println(F("\nTIMEOUT — Restart the sketch."));
            while (1);
        }

        // Shows the final result of this digit
        Serial.println();
        Serial.print(F("  Final: "));
        if (bufCount == 0) {
            Serial.println(F("(no pulse detected)"));
        } else {
            for (int j = 0; j < bufCount; j++) {
                Serial.print(buf[j]);
                Serial.print(' ');
            }
            Serial.print(F(" ("));
            Serial.print(bufCount);
            Serial.println(F(" pulses)"));
        }

        // Copies to the permanent storage
        noInterrupts();
        digitPulseCount[i] = bufCount;
        for (int j = 0; j < bufCount && j < MAX_PULSES; j++) {
            digitHighs[i][j] = buf[j];
        }
        interrupts();

        delay(300);  // pause between digits
    }

    // ═══════════════════════════════════════════════════════
    // ANALYSIS
    // ═══════════════════════════════════════════════════════

    Serial.println(F("\n\n=========================================="));
    Serial.println(F("         CALIBRATION RESULT"));
    Serial.println(F("==========================================\n"));

    // Raw data table
    Serial.println(F("Raw data (highLen in ms):"));
    Serial.println(F(" Dial   | pulses | highLens"));
    Serial.println(F(" -------+--------+--------------------------"));
    for (int i = 0; i < TOTAL_DIGITS; i++) {
        int expected = (i == 9) ? 0 : i + 1;
        Serial.print(F("    "));
        Serial.print(expected);
        Serial.print(F("    |   "));
        Serial.print(digitPulseCount[i]);
        Serial.print(F("    | "));
        for (int j = 0; j < digitPulseCount[i]; j++) {
            Serial.print(digitHighs[i][j]);
            Serial.print(' ');
        }
        Serial.println();
    }

    // ── Identifies gap vs slit ─────────────────────────
    // The FIRST highLen of each digit is the GAP (resting area).
    // The OTHERS are the SLITS of the disc.
    //
    // If your disc has another pattern, interpret it manually.

    unsigned long minGap = 999999, maxGap = 0;
    unsigned long minSlot = 999999, maxSlot = 0;
    int totalGaps = 0, totalSlots = 0;

    Serial.println(F("\n--- Gap vs slit analysis ---"));
    Serial.println(F("  (FIRST value = gap, OTHERS = slits)\n"));

    for (int i = 0; i < TOTAL_DIGITS; i++) {
        if (digitPulseCount[i] > 0) {
            unsigned long g = digitHighs[i][0];
            if (g < minGap) minGap = g;
            if (g > maxGap) maxGap = g;
            totalGaps++;

            for (int j = 1; j < digitPulseCount[i]; j++) {
                unsigned long s = digitHighs[i][j];
                if (s < minSlot) minSlot = s;
                if (s > maxSlot) maxSlot = s;
                totalSlots++;
            }
        }
    }

    if (totalGaps > 0) {
        Serial.print(F("  Gap  (resting area): "));
        Serial.print(minGap);
        Serial.print(F(" - "));
        Serial.print(maxGap);
        Serial.print(F(" ms  ("));
        Serial.print(totalGaps);
        Serial.println(F(" samples)"));
    } else {
        Serial.println(F("  Gap: none detected."));
    }

    if (totalSlots > 0) {
        Serial.print(F("  Slit (slots):        "));
        Serial.print(minSlot);
        Serial.print(F(" - "));
        Serial.print(maxSlot);
        Serial.print(F(" ms  ("));
        Serial.print(totalSlots);
        Serial.println(F(" samples)"));
    } else {
        Serial.println(F("  Slits: none detected."));
    }

    // ── Suggested values ────────────────────────────────
    // If there is not enough data, it suggests nothing

    if (totalSlots > 0) {
        unsigned long debounce = minSlot > 5 ? minSlot - 5 : minSlot;
        unsigned long maxgap   = 0;   // no ceiling
        unsigned long digitend;

        if (totalGaps > 0) {
            digitend = maxGap + 200;
        } else {
            digitend = (maxSlot > 0) ? maxSlot * 4 : 800;
        }

        Serial.println(F("\n=========================================="));
        Serial.println(F("  SUGGESTED VALUES — Copy to your sketch"));
        Serial.println(F("=========================================="));
        Serial.print(F("  RotaryDialUnified::setDebounce("));
        Serial.print(debounce);
        Serial.println(F(");   // floor: filters noise and return pulses"));

        Serial.print(F("  RotaryDialUnified::setMaxGap("));
        Serial.print(maxgap);
        Serial.println(F(");       // ceiling: 0 = no ceiling (recommended)"));
        Serial.println(F("                                 // gap and slit can be in the same range"));

        Serial.print(F("  RotaryDialUnified::setDigitEnd("));
        Serial.print(digitend);
        Serial.println(F(");  // timeout: closes the digit after silence"));
    } else {
        Serial.println(F("\n=========================================="));
        Serial.println(F("  Not enough data to suggest values."));
        Serial.println(F("  Check the connection and try again."));
        Serial.println(F("=========================================="));
    }

    Serial.println(F("\n=========================================="));
    Serial.println(F("  Calibration finished."));
    Serial.println(F("  Restart the sketch to repeat."));
    Serial.println(F("=========================================="));

    while (1);
}

void loop() {}
