# Sake — Atari Kernel Emulator for AmigaOS

Sake maps Atari ST GEMDOS, BIOS, and XBIOS trap calls to native AmigaOS
`dos.library` / `exec.library` functions, allowing Atari ST executables to
run on AmigaOS through a thin emulation layer.

## Build

Requires [PortablE](https://github.com/portablE/portablE).

    PortablE Sake.e OS=AmigaOS3

## Usage

    Sake <prg_file>

Loads an Atari ST PRG file into memory. File extension is optional.

    Sake

Runs the built-in test suite (GEMDOS function tests).

## Status

Trap dispatchers and function stubs for all three Atari ST trap types:

| Trap   | Functions |
|--------|-----------|
| GEMDOS | 30+ — file I/O, console, directory, memory, process, date/time |
| BIOS   | 12 — console I/O, tick calibration, drive map, keyboard shift |
| XBIOS  | 18 — screen, mouse, timer, palette, floppy, RS232, random, cookie jar |

Notable implementations: **Pexec ($2D) LoadSeg mode 0** parses Atari PRG headers
and loads text/data/bss segments via `AllocMem`.

Hardware-level calls (Rwabs, Flop*, DMA, MFP, screen) return `E_ERROR` or 0 —
these need Amiga-side emulation of ST hardware. The trap handler (module-level
ASM) is omitted due to PortablE limitations.

## References

- https://github.com/emutos/emutos/blob/master/doc/status.txt
- Atari ST GEMDOS/BIOS/XBIOS reference
