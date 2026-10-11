/**
 * RotaryDial.cpp — unified implementation (mechanical + optical).
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
#include "RotaryDial.h"

// ── Default values per mode ─────────────────────────────

#define MS_DEBOUNCE_MECH   15UL
#define MS_DIGITEND_MECH  600UL

#define MS_DEBOUNCE_OPT    50UL
#define MS_MAXGAP_OPT       0UL    // No ceiling — on the optical disc the gap and the slot share one range
#define MS_DIGITEND_OPT   800UL

// ── Static members (default = mechanical) ───────────────

RotaryDialMode RotaryDial::_mode = ROTARYDIAL_MECHANICAL;
int RotaryDial::_interruptPin = ROTARYDIAL_DEFAULT_PIN;

volatile int RotaryDial::_pulseCount = 0;
volatile unsigned long RotaryDial::_lastHigh = 0;
volatile int RotaryDial::_lastState = HIGH;

unsigned long RotaryDial::_debounceMs = MS_DEBOUNCE_MECH;
unsigned long RotaryDial::_maxGapMs   = 0;                 // mechanical: no ceiling
unsigned long RotaryDial::_digitEndMs = MS_DIGITEND_MECH;
volatile bool RotaryDial::_firstPulse = false;

// highLen buffer for debug/calibration
#define HL_BUF_SIZE 20
static volatile unsigned long _hlBuf[HL_BUF_SIZE];
static volatile int _hlCount = 0;

// ── Initialization ──────────────────────────────────────

void RotaryDial::begin(int pulsePin, RotaryDialMode mode) {
    _interruptPin = pulsePin;
    _mode = mode;
    reset();

    if (mode == ROTARYDIAL_MECHANICAL) {
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
        RotaryDial::isr,
        CHANGE
    );
}

void RotaryDial::beginMechanical(int pulsePin) {
    begin(pulsePin, ROTARYDIAL_MECHANICAL);
}

void RotaryDial::beginOptical(int pulsePin) {
    begin(pulsePin, ROTARYDIAL_OPTICAL);
}

// ── ISR (IRAM_ATTR on ESP32, no effect on AVR/RP2040) ──

void ISR_ATTR RotaryDial::isr() {
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
            if (_mode == ROTARYDIAL_OPTICAL && _firstPulse) {
                _firstPulse = false;
            } else {
                _pulseCount++;
            }
        }
    }
}

// ── Debug / calibration ────────────────────────────────

int RotaryDial::getHighLenCount() {
    return _hlCount;
}

unsigned long RotaryDial::getHighLen(int i) {
    if (i >= 0 && i < _hlCount) return _hlBuf[i];
    return 0;
}

void RotaryDial::clearHighLens() {
    _hlCount = 0;
}

// ── Reading ─────────────────────────────────────────────

bool RotaryDial::available() {
    // The digit is ready when there are pulses and the silence timeout has passed
    return (_pulseCount > 0 &&
            (millis() - _lastHigh) > _digitEndMs);
}

int RotaryDial::decodeDigit(int pulses) {
    if (pulses == 10) return 0;
    if (pulses >= 1 && pulses <= 9) return pulses;
    return -1;
}

int RotaryDial::read() {
    if (!available()) return -1;
    int pulses = _pulseCount;
    int digit = decodeDigit(pulses);
    reset();
    return digit;
}

int RotaryDial::readPulses() {
    if (!available()) return -1;
    int pulses = _pulseCount;
    reset();
    return pulses;
}

void RotaryDial::reset() {
    _pulseCount = 0;
    _lastHigh = 0;
    _lastState = HIGH;
    _firstPulse = (_mode == ROTARYDIAL_OPTICAL);
}

// ── Runtime timing adjustments ─────────────────────────

void RotaryDial::setDebounce(unsigned long ms) {
    _debounceMs = ms;
}

void RotaryDial::setMaxGap(unsigned long ms) {
    _maxGapMs = ms;
}

void RotaryDial::setDigitEnd(unsigned long ms) {
    _digitEndMs = ms;
}
