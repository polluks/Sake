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
| VDI    | 30+ — workstation, drawing, attribute, inquiry functions via SELECT dispatch |

Notable implementations:
- **Pexec ($2D) LoadSeg mode 0** parses Atari PRG headers and loads text/data/bss
  segments via `AllocMem`.
- **Font registration** maps the full Atari ST character set (256 glyphs, 8x16 and
  8x8) to native AmigaOS `TextFont` structures via `AddFont()`. Bitmap data is
  embedded as a static glyph table matching the ST ROM layout.
- **Mouse pointers** — four predefined shapes (arrow, hourglass, I-beam, pointing
  finger) initialized in pure PortablE with inline sprite data.
- **Minimal NATIVE** — inline C is largely avoided; AmigaOS calls like
  `SetPointer`/`ClearPointer` and `TextFont` struct setup are wrapped as
  single-line `IS NATIVE` PortablE declarations.

VDI implements colour palette (16 standard Atari colours via `vdi_rgb`),
line patterns, and tracks workstation state (handle, resolution, drawing
attributes, clip rectangle, cursor). Drawing operations read coordinate
pairs from the AES `ptrin` array and update `vdi_cur_x/y`.

Hardware-level BIOS calls (Rwabs, Flop*, DMA, MFP) return `E_ERROR` or 0 —
these need Amiga-side emulation of ST hardware.

VDI dispatch routes 30+ function numbers (opnvwk, clsvwk, pline, pmarker,
fillarea, bar, arc, ellipse, pieslice, circle, gtext, justif, show_c,
curup/down, curtext, rasterbox, rastercol, esc, exits, init, inquire
attributes for line/marker/text/fill/colour, cell array, xbit_image, mouse
form, and valuator/choice/string inquiry) to individual procedures via a
SELECT block, returning stubs where unimplemented.

Notable VDI state variables include `vdi_work_w/h`, `vdi_dev_w/h`,
`vdi_n_planes`, `vdi_line_type/width/color`, `vdi_fill_type/index/color`,
`vdi_marker_type/height/color`, `vdi_text_font/color/rotation`, `vdi_wr_mode`,
and `vdi_clip_x/y/w/h`. All initialized on workstation open and returned
via `intout[]` for GEM inquiry calls.

The trap handler (inline ASM) is omitted due to PortablE limitations.

## References

- https://github.com/emutos/emutos/blob/master/doc/status.txt
- Atari ST GEMDOS/BIOS/XBIOS reference
