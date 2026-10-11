/**
 * RotaryDialUnified.cpp — unified implementation (mechanical + optical).
 *
 * DEFAULT: MECHANICAL (MS_DEBOUNCE=15ms, no ceiling, MS_DIGITEND=600ms).
 *
 * MECHANICAL:
 *   - Counts every pulse with highLen > MS_DEBOUNCE_MECH (15ms)
 *   - Closes the digit after MS_DIGITEND_MECH (600ms) without pulses
 *
 * OPTICAL:
 *   - Counts pulses with highLen > MS_DEBOUNCE_OPT (50ms)
 *   - Uses _firstPulse to skip the disc's return pulse (triggered after
 *     read()+reset(), where lastHigh=0 produces a spurious measurement)
 *   - Closes the digit after MS_DIGITEND_OPT (800ms) without pulses
 *
 * Platforms:
 *   - ESP32:   the ISR is marked IRAM_ATTR (ISR_ATTR macro in the header).
 *              digitalRead/millis are already IRAM-safe in the Arduino core.
 *   - RP2040:  any GPIO takes an interrupt, no special attribute.
 *   - AVR:     only pins 2/3 for INT0/INT1. Pass pin 2 explicitly.
 *
 * Default pin: D0 (RP2040/ESP32).
 *
 * Algorithm and API names follow the original RotaryDial by Tomas Mudrunka
 * (MIT, https://github.com/Harvie/RotaryDial); the optical mode, the platform
 * attributes and the runtime timing adjustments are additions.
 */

#include "Arduino.h"
#include "RotaryDialUnified.h"

// ── Default values per mode ─────────────────────────────

#define MS_DEBOUNCE_MECH   15UL
#define MS_DIGITEND_MECH  600UL

#define MS_DEBOUNCE_OPT    50UL
#define MS_MAXGAP_OPT       0UL    // No ceiling — on the optical disc the gap and the slot share one range
#define MS_DIGITEND_OPT   800UL

// ── Static members (default = mechanical) ───────────────

RotaryDialUnifiedMode RotaryDialUnified::_mode = ROTARYDIAL_UNIFIED_MECHANICAL;
int RotaryDialUnified::_interruptPin = ROTARYDIAL_UNIFIED_DEFAULT_PIN;

volatile int RotaryDialUnified::_pulseCount = 0;
volatile unsigned long RotaryDialUnified::_lastHigh = 0;
volatile int RotaryDialUnified::_lastState = HIGH;

unsigned long RotaryDialUnified::_debounceMs = MS_DEBOUNCE_MECH;
unsigned long RotaryDialUnified::_maxGapMs   = 0;                 // mechanical: no ceiling
unsigned long RotaryDialUnified::_digitEndMs = MS_DIGITEND_MECH;
volatile bool RotaryDialUnified::_firstPulse = false;

// highLen buffer for debug/calibration
#define HL_BUF_SIZE 20
static volatile unsigned long _hlBuf[HL_BUF_SIZE];
static volatile int _hlCount = 0;

// ── Initialization ──────────────────────────────────────

void RotaryDialUnified::begin(int pulsePin, RotaryDialUnifiedMode mode) {
    _interruptPin = pulsePin;
    _mode = mode;
    reset();

    if (mode == ROTARYDIAL_UNIFIED_MECHANICAL) {
        _debounceMs = MS_DEBOUNCE_MECH;
        _maxGapMs   = 0;               // no ceiling in mechanical mode
        _digitEndMs = MS_DIGITEND_MECH;
    } else {
        _debounceMs = MS_DEBOUNCE_OPT;
        _maxGapMs   = MS_MAXGAP_OPT;
        _digitEndMs = MS_DIGITEND_OPT;
        _firstPulse = true;          // the next HIGH is the return pulse — skip it
    }

    pinMode(_interruptPin, INPUT_PULLUP);
    attachInterrupt(
        digitalPinToInterrupt(_interruptPin),
        RotaryDialUnified::isr,
        CHANGE
    );
}

void RotaryDialUnified::beginMechanical(int pulsePin) {
    begin(pulsePin, ROTARYDIAL_UNIFIED_MECHANICAL);
}

void RotaryDialUnified::beginOptical(int pulsePin) {
    begin(pulsePin, ROTARYDIAL_UNIFIED_OPTICAL);
}

// ── ISR (IRAM_ATTR on ESP32, no effect on AVR/RP2040) ──

void ISR_ATTR RotaryDialUnified::isr() {
    int state = digitalRead(_interruptPin);

    // Debounce: ignore if the state did not change
    if (state == _lastState) return;
    _lastState = state;

    // Do not accumulate pulses while a digit is waiting to be read
    if (available()) return;

    if (state == HIGH) {
        _lastHigh = millis();
    } else {
        // Falling edge — measure how long HIGH lasted
        unsigned long highLen = millis() - _lastHigh;

        if (highLen > _debounceMs) {
            // Record it in the debug buffer
            if (_hlCount < HL_BUF_SIZE) {
                _hlBuf[_hlCount++] = highLen;
            }

            // Optical mode: the first HIGH after read()+reset() is the disc's
            // return pulse (lastHigh=0 produces a spurious measurement). Skip it.
            if (_mode == ROTARYDIAL_UNIFIED_OPTICAL && _firstPulse) {
                _firstPulse = false;
            } else {
                _pulseCount++;
            }
        }
    }
}

// ── Debug / calibration ────────────────────────────────

int RotaryDialUnified::getHighLenCount() {
    return _hlCount;
}

unsigned long RotaryDialUnified::getHighLen(int i) {
    if (i >= 0 && i < _hlCount) return _hlBuf[i];
    return 0;
}

void RotaryDialUnified::clearHighLens() {
    _hlCount = 0;
}

// ── Reading ─────────────────────────────────────────────

bool RotaryDialUnified::available() {
    // The digit is ready when there are pulses and the silence timeout has passed
    return (_pulseCount > 0 &&
            (millis() - _lastHigh) > _digitEndMs);
}

int RotaryDialUnified::decodeDigit(int pulses) {
    if (pulses == 10) return 0;
    if (pulses >= 1 && pulses <= 9) return pulses;
    return -1;
}

int RotaryDialUnified::read() {
    if (!available()) return -1;
    int pulses = _pulseCount;
    int digit = decodeDigit(pulses);
    reset();
    return digit;
}

int RotaryDialUnified::readPulses() {
    if (!available()) return -1;
    int pulses = _pulseCount;
    reset();
    return pulses;
}

void RotaryDialUnified::reset() {
    _pulseCount = 0;
    _lastHigh = 0;
    _lastState = HIGH;
    _firstPulse = (_mode == ROTARYDIAL_UNIFIED_OPTICAL);
}

// ── Runtime timing adjustments ─────────────────────────

void RotaryDialUnified::setDebounce(unsigned long ms) {
    _debounceMs = ms;
}

void RotaryDialUnified::setMaxGap(unsigned long ms) {
    _maxGapMs = ms;
}

void RotaryDialUnified::setDigitEnd(unsigned long ms) {
    _digitEndMs = ms;
}
