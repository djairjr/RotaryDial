#pragma once

/**
 * RotaryDial.h — unified library for telephone rotary dials.
 *
 * Two operating modes in a single library:
 *   ROTARYDIAL_MECHANICAL — metallic contacts (pulse = closure) [DEFAULT]
 *   ROTARYDIAL_OPTICAL    — optical encoder (disc with 10 slots + resting area)
 *
 * Platforms: AVR, RP2040, ESP32.
 *   - ESP32:  the ISR is marked with IRAM_ATTR automatically.
 *   - AVR:    only pins 2 or 3 can take an external interrupt.
 *   - RP2040: any GPIO can take an interrupt.
 *
 * Default pin (ROTARYDIAL_DEFAULT_PIN):
 *   ESP32  → GPIO 0
 *   RP2040 → D0 (GPIO 0)
 *   AVR    → 2 (INT0)
 *
 * Default mode: MECHANICAL.
 *
 * API:
 *   begin(pin, mode)      — start with a pin and a mode (default: MECHANICAL)
 *   beginMechanical(pin)  — mechanical shortcut
 *   beginOptical(pin)     — optical shortcut
 *   available()           — true when a digit is ready to be read
 *   read()                — decoded digit (0-9) or -1
 *   readPulses()          — raw pulse count (1-10) or -1
 *   reset()               — clears the internal state
 *
 * Runtime fine tuning (optional):
 *   setDebounce(ms)       — minimum HIGH length (ms)
 *   setMaxGap(ms)         — maximum HIGH length (ms), 0 = no ceiling
 *   setDigitEnd(ms)       — silence without pulses that closes a digit (ms)
 */

#include "Arduino.h"

// ── Default pin per platform ───────────────────────────
// ESP32 does not define D0; it uses the raw GPIO number.
// RP2040 (Arduino-Pico) defines D0 = GPIO0.
// AVR: pins 0-13 are raw numbers; only 2 and 3 have INTx.
#ifndef ROTARYDIAL_DEFAULT_PIN
  #if defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) || \
      defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || \
      defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    #define ROTARYDIAL_DEFAULT_PIN  0    // ESP32: GPIO0
  #elif defined(ARDUINO_ARCH_RP2040)
    #define ROTARYDIAL_DEFAULT_PIN  D0   // RP2040: D0 = GPIO0
  #elif defined(ARDUINO_ARCH_AVR)
    #define ROTARYDIAL_DEFAULT_PIN  2    // AVR: INT0 (the only safe pin)
  #else
    #define ROTARYDIAL_DEFAULT_PIN  0    // fallback
  #endif
#endif

// ── ISR attribute per platform ─────────────────────────
// ESP32 requires IRAM_ATTR on functions called by attachInterrupt.
// On the Arduino core 3.x, digitalRead/millis are already IRAM_ATTR-safe.
#ifndef ISR_ATTR
  #if defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) || \
      defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || \
      defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    #define ISR_ATTR IRAM_ATTR
  #else
    #define ISR_ATTR
  #endif
#endif

enum RotaryDialMode : int {
    ROTARYDIAL_MECHANICAL,   // traditional electrical contact
    ROTARYDIAL_OPTICAL        // optical encoder (slot + resting area)
};

class RotaryDial {
public:
    // ── Initialization ─────────────────────────────────
    static void begin(int pulsePin = ROTARYDIAL_DEFAULT_PIN,
                      RotaryDialMode mode = ROTARYDIAL_MECHANICAL);
    static void beginMechanical(int pulsePin = ROTARYDIAL_DEFAULT_PIN);
    static void beginOptical(int pulsePin = ROTARYDIAL_DEFAULT_PIN);

    // ── Reading ────────────────────────────────────────
    static bool available();
    static int  read();          // digit 0-9, -1 when nothing is available
    static int  readPulses();    // pulses 1-10, -1 when nothing is available
    static void reset();

    // ── Debug / calibration ────────────────────────────
    static int  getHighLenCount();           // how many highLens are in the buffer
    static unsigned long getHighLen(int i);  // highLen[i] (0-based)
    static void clearHighLens();             // clears the debug buffer

    // ── Timing adjustments (runtime) ───────────────────
    static void setDebounce(unsigned long ms);
    static void setMaxGap(unsigned long ms);
    static void setDigitEnd(unsigned long ms);

private:
    // State
    static RotaryDialMode     _mode;
    static int                _interruptPin;
    static volatile int       _pulseCount;
    static volatile unsigned long _lastHigh;
    static volatile int       _lastState;

    // Timing parameters
    static unsigned long _debounceMs;   // floor: minimum HIGH to be counted
    static unsigned long _maxGapMs;     // ceiling: maximum HIGH (0 = off)
    static unsigned long _digitEndMs;   // timeout that closes a digit

    // Initial gap control (optical disc)
    static volatile bool _firstPulse;   // true = the next highLen is the gap, skip it

    // ISR and decoding
    static void ISR_ATTR isr();
    static int  decodeDigit(int pulses);
};
