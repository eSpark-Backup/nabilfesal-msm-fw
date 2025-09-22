# Flashing firmware to ATSAMD20G17 using J-Link Commander

This guide explains how to flash firmware into ATSAMS20G17 using **SEGGER J-Link Commander (JLinkExe / JLink Commander)**.

---

## Prerequisites

- **SEGGER J-Link software package** installed  
  - Download: [SEGGER J-Link Downloads](https://www.segger.com/downloads/jlink/)  
- A **J-Link programmer/debugger** connected to your target board  
- The target device powered and properly connected (SWD/JTAG interface)  
- Your firmware file in `.elf` format, [firmware download link](https://github.com/eSpark-Consultants/nabilfesal-msm-fw/tree/Parameters_Calibration/ATSAM/Binaries/production)

---

## Steps

### 1. Open J-Link Commander
- Launch `JLink Commander`
- Type `connect` and press `Enter`
- Select `ATSAMD20G17` MCU
- Type `s` to select SWD mode
- Press `Enter` to select the default speed
- Once connection is successful, type `loadfile <path of the .elf file>` and `Enter`
- Once complete, type `reset` and press `Enter`
- And type `go` and press `Enter`
- Now you're good to use!