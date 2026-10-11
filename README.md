# RotaryDial

Unified Arduino library for reading telephone rotary dials.

Supports **two modes** of operation in a single library:

| Mode | Sensor | How to start |
|------|--------|-------------|
| `ROTARYDIAL_MECHANICAL` **(default)** | Traditional metallic contacts | `begin(D0)` or `beginMechanical(D0)` |
| `ROTARYDIAL_OPTICAL` | Optical encoder (10-slit disc + rest area) | `beginOptical(D0)` |

**Platforms:** AVR, RP2040, ESP32  
**Default pin:** D0 (RP2040/ESP32) — on AVR use pin 2 or 3  
**Default mode:** MECHANICAL  
**Single include:** `#include <RotaryDial.h>`

---

## Platform compatibility

| Platform | Interrupt pins | ISR | Notes |
|---|---|---|---|
| **ESP32** | All GPIOs | `IRAM_ATTR` (automatic) | `digitalRead`/`millis` are already IRAM-safe |
| **RP2040** | All GPIOs | No special attribute | Arduino-Pico core (Earle Philhower) |
| **AVR** (Nano/Uno) | **Only pins 2 and 3** | No special attribute | `digitalPinToInterrupt()` works only on INT0/INT1 |

> ⚠️ **AVR:** the default pin `D0` does not support interrupts on AVR. Pass `begin(2)` or `begin(3)` explicitly.

---

## Installation

Copy the `RotaryDial` folder into `Documents/Arduino/libraries/`.

```
Documents/Arduino/libraries/
└── RotaryDial/
    ├── library.properties
    ├── README.md
    ├── LICENSE.md
    ├── src/
    │   ├── RotaryDial.h
    │   └── RotaryDial.cpp
    ├── examples/
    │   ├── dial_read_mechanical/
    │   ├── dial_read_optical/
    │   ├── DialCalibrator/
    │   └── pulse_timing/
    └── docs/
```

---

## API

```cpp
#include <RotaryDial.h>

// ── Initialization ─────────────────────────

RotaryDial::begin(D0);                           // Default = MECHANICAL
RotaryDial::begin(D0, ROTARYDIAL_MECHANICAL);    // Explicit
RotaryDial::begin(D0, ROTARYDIAL_OPTICAL);       // Optical
RotaryDial::beginMechanical(D0);                 // Shortcut
RotaryDial::beginOptical(D0);                    // Shortcut

// ── Reading ────────────────────────────────

bool ready = RotaryDial::available();   // Digit ready?
int digit  = RotaryDial::read();        // 0-9, or -1
int pulses = RotaryDial::readPulses();  // 1-10, or -1
RotaryDial::reset();                   // Clears the state

// ── Fine tuning (optional) ─────────────────

RotaryDial::setDebounce(15);   // Minimum HIGH (ms)
RotaryDial::setMaxGap(200);    // Maximum HIGH (ms) — optical
RotaryDial::setDigitEnd(800);  // End-of-digit timeout (ms)
```

---

## Quick example (mechanical — default)

```cpp
#include <RotaryDial.h>

void setup() {
    Serial.begin(115200);
    RotaryDial::begin(D0);   // mechanical mode (default)
    // On AVR (Nano/Uno): use RotaryDial::begin(2);
    Serial.println("Dial...");
}

void loop() {
    if (RotaryDial::available()) {
        Serial.println(RotaryDial::read());
    }
}
```

---

## Modes and timing

### Mechanical (`ROTARYDIAL_MECHANICAL`) — DEFAULT

```
Signal:  HIGH ┐    ┌─┐    ┌─┐    ┌─┐         ┌───────────┐
              └─15─┘ └─15─┘ └─15─┘ └─15──────┘           
              ms   ms   ms   ms          600ms
                                           ▲
                                     End of digit
```

- **MS_DEBOUNCE = 15ms** — floor: filters contact bounce
- **MS_DIGITEND = 600ms** — closes the digit after silence
- No ceiling (MS_MAXGAP = 0)

### Optical (`ROTARYDIAL_OPTICAL`)

Disc with 10 uniform slits + ~1/4 rest area. The sensor stays blocked (HIGH) during rest.

- **MS_DEBOUNCE = 50ms** — floor: filters the return pulses (quick return)
- **MS_MAXGAP = 200ms** — ceiling: filters out the false "1" from rest (845ms+)
- **MS_DIGITEND = 800ms** — closes the digit (176 < 800 < 1044)

---

## Calibration

Use the `DialCalibrator` example to find the ideal values for your dial:

1. Open `File > Examples > RotaryDial > DialCalibrator`
2. Dial 1, 2, 3 ... 9, 0 as requested
3. Copy the suggested values into `setDebounce()`, `setMaxGap()`, `setDigitEnd()`

The `pulse_timing` example prints the HIGH time of every pulse in real time — useful for debugging.

---

## Hardware

```
Dial ──── Interrupt pin (INPUT_PULLUP)

Mechanical: contacts between the pin and GND
Optical:    sensor with a digital output (LOW = light passing = slit)
```

**RP2040 (Pi Pico, PropMaker, etc.):** D0 = GPIO0  
**ESP32:** D0 = GPIO0  
**AVR (Nano/Uno):** use pin 2 or 3 (INT0/INT1)

---

## History

- **v0.0.1** — original by Tomas Mudrunka (mechanical only, AVR)
- **v1.0.0** — consolidation: mechanical + optical, multi-platform (AVR/RP2040/ESP32), pin D0, mechanical by default, IRAM_ATTR on ESP32
