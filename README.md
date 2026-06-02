# Sake — Atari Kernel Emulator for AmigaOS

Sake maps Atari ST system calls to native AmigaOS libraries:
- **GEMDOS** (Trap #1) → `dos.library` / `exec.library`
- **BIOS** (Trap #2) → AmigaOS equivalents
- **XBIOS** (Trap #2, extended) → AmigaOS equivalents  
- **AES** (Trap #2, $C8) → `intuition.library` / `gadtools.library`
- **VDI** (Trap #2, $C9) → `graphics.library`

## Build

Requires [PortablE](https://github.com/portablE/portablE).

    PortablE Sake.e OS=AmigaOS3

## Usage

    Sake <prg_file>

Loads an Atari ST PRG file into memory. File extension is optional.

    Sake

Runs the built-in test suite (GEMDOS function tests).

## Status

Trap dispatchers and function stubs for all five Atari ST trap interfaces:

| Trap   | Functions |
|--------|-----------|
| GEMDOS | 30+ — file I/O, console, directory, memory, process, date/time |
| BIOS   | 12 — console I/O, tick calibration, drive map, keyboard shift |
| XBIOS  | 18 — screen, mouse, timer, palette, floppy, RS232, random, cookie jar |
| AES    | 60+ — window, menu, form, event, graphics, file selector, object |
| VDI    | v_opnvwk/v_clsvwk stub — remaining drawing ops return 1 |

Notable implementations:
- **Pexec ($2D) LoadSeg mode 0** parses Atari PRG headers and loads text/data/bss
  segments via `AllocMem`.
- **Font registration** maps the full Atari ST character set (256 glyphs, 8x16 and
  8x8) to native AmigaOS `TextFont` structures via `AddFont()`. Bitmap data is
  embedded as a static glyph table matching the ST ROM layout.

Hardware-level BIOS calls (Rwabs, Flop*, DMA, MFP) and most VDI drawing return
`E_ERROR` or 0 — these need Amiga-side emulation of ST hardware.

AES window/form/menu calls are wired to `intuition.library` and `gadtools.library`
and ready for progressive implementation.

The trap handler (inline ASM) is omitted due to PortablE limitations.

## References

- https://github.com/emutos/emutos/blob/master/doc/status.txt
- Atari ST GEMDOS/BIOS/XBIOS reference
