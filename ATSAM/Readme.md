# ATSAMD20G17 Firmware for Energy Monitoring using ADE9000

This firmware is designed to interface the **Analog Devices ADE9000** energy metering IC with the **ATSAMD20G17** microcontroller. The application enables precise measurement and conversion of various electrical parameters, including energy values, THD (Total Harmonic Distortion), and angle measurements, with support for per-channel CT-ratio configuration.


## Features

- Interfacing with ADE9000 via SPI for energy data acquisition
- CT-ratio parsing and configuration per input channel
- Energy data conversion from raw register values
- Accurate reading of:
  - RMS Voltage and Current
  - Active, Reactive, and Apparent Power
  - Total Harmonic Distortion (THD) for voltage and current
  - Power Factor and Angle
- Multi-channel support with per-phase calibration
- Lightweight and efficient firmware tailored for ATSAMD20G17


## Microcontroller

**Microcontroller**: ATSAMD20G17  
**Architecture**: ARM Cortex-M0+  
**Interface Used**: SPI (to communicate with ADE9000)


## ADE9000 Overview

The ADE9000 is a highly integrated, multichannel energy metering IC capable of measuring various power parameters. It provides waveform sampling, power calculation, THD, and angle measurements over an SPI interface.


## CT-Ratio Configuration

Each channel's current transformer (CT) ratio can be configured via the user interface (e.g., serial input or stored config file). The firmware parses and applies the CT ratio for scaling the current and power values.

```c
// Example: Setting CT ratio for Channel A
set_ct_ratio(CHANNEL_A, 100);
Ensure the ratios are configured before initiating measurement operations.
```
#### THD and Angle Measurement
The firmware reads the following from ADE9000:
```

Voltage THD (THD_VA, THD_VB, THD_VC)
Current THD (THD_IA, THD_IB, THD_IC)
Voltage-Current Phase Angle (ANGLE_A, ANGLE_B, ANGLE_C)
```

Raw values are converted to percentage or degrees using vendor-specified scaling.

#### Energy Value Conversion
Energy values read from ADE9000 are scaled using the formula:

```
RealEnergy = (RegisterValue × CalibrationConstant) / ScalingFactor;
// These constants are set based on board calibration and ADE9000 datasheet guidelines.
```
## Build Instructions

### Prerequisites
- Atmel Studio or ARM GCC Toolchain
- OpenOCD or a compatible programmer/debugger
- ATSAMD20G17 Dev Board or custom PCB(SCM Board integrated with ADE9000)

### Flashing the Firmware
- Connect the programmer to the ATSAMD20G17 via SWD.
- Build the firmware using Atmel Studio or make (if using CLI).
- Flash the .hex or .bin file using your preferred flashing tool.

## Configuration
### SPI Setup
- Mode: SPI Mode 3
- Frequency: 1 MHz (initial), can be increased after initialization
- Chip Select: Configurable via config.h
## UART Communication

The firmware supports serial communication over UART for querying channel data and configuring calibration parameters. Communication is done using simple JSON-formatted messages.


### Channel Data

#### Transmitting
To request data from a specific channel:
channel-N\n

**Note:** `N` is a natural number from `1` to `12`.
```
channel-1
```
#### Receiving
Example JSON response from the firmware:
```json
{
  "ID": 1,
  "status": true,
  "temperature": 27.5,
  "data": {
    "phase_a": {
      "general": {
        "line_voltage": 0,
        "rms_voltage": 0,
        "frequency": 0,
        "current": 0
      },
      "power": {
        "factor": 0,
        "active": 0,
        "reactive": 0,
        "apparent": 0
      },
      "energy": {
        "active": { "positive": 0, "negative": 0 },
        "reactive": { "positive": 0, "negative": 0 }
      },
      "thd": {
        "current": 0,
        "voltage": 0,
        "angle": 0
      }
    },
    ...
  }
}
```
### CT Ratio Configuration
#### Transmitting

```
{
  "command": "set-ct-ratio",
  "ct_ratio": {
    "channel-1": 200,
    "channel-2": 200,
    ...
    "channel-12": 300
  }
}
```

#### Receiving

```
"CT Ratio updated successfully!"
```
### Current Offset Configuration
#### Transmitting

```
{
  "command": "set-i-offset",
  "i_offset": {
    "channel-1": 200,
    ...
    "channel-12": 300
  }
}
```
#### Receiving
```
"Current Offset updated successfully!"
```

### Voltage Offset Configuration
#### Transmitting

```
{
  "command": "set-v-offset",
  "v_offset": {
    "channel-1": 200,
    ...
    "channel-12": 300
  }
}
```

#### Receiving
```
"Voltage Offset updated successfully!"
```
## Troubleshooting
###### Incorrect values: Check CT-ratio configuration and ensure correct ADE9000 calibration constants are used.


## References
> ADE9000 Datasheet

> ATSAMD20G17 Datasheet

---

