# rGBA

My first GBA emulator, written in C.

## Status

Work in progress. ARM7TDMI core, memory bus, I/O registers and PPU are under
active development; see the `v2-rewrite` branch for the current architecture.

## Build

```
cmake -S . -B build -G Ninja
cmake --build build
```

## Running: BIOS and ROMs are not included

This repository does **not** ship a GBA BIOS image or any commercial game ROM.
Those files are copyrighted by their respective owners and cannot be
redistributed here.

To run the emulator you need to supply your own files:

| File | Where to put it | How to obtain it |
|------|-----------------|------------------|
| GBA BIOS (16 KiB) | `roms/gba_bios.bin` | Dump it yourself from hardware you own |
| Game ROM | `roms/` | Dump it yourself from a cartridge you own |

Open-source test ROMs that *are* included (`roms/haltcnt.gba`) are freely
redistributable homebrew used for hardware-behaviour tests.

## Hardware documentation

This project is written against publicly available documentation. The
documents themselves are not copied into this repository - please read them at
the source:

- **GBATEK** (GBA hardware reference, by Martin Korth) -
  https://problemkaputt.de/gbatek.htm
- **ARM7TDMI Technical Reference Manual** (ARM Ltd.) -
  https://developer.arm.com/documentation/ddi0210/c/
- **ARM Architecture Reference Manual (ARMv4T)** (ARM Ltd.) -
  https://developer.arm.com/documentation/ddi0100/latest/

## Dependencies

SDL2 is used for video/input output. Install it from your platform's package
manager or from https://www.libsdl.org/ - its headers are not vendored into
this repository.
