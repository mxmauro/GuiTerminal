# GuiTerminal Architecture

## Core Layers

### Public C++ surface

- `include/GuiTerminalControl.h`
- `src/GuiTerminalControl.cpp`

`Control` is the main user-facing class. It owns:

- `Internals::Buffer` for terminal state
- `Internals::Parser` instances on write paths
- `Internals::Renderer` for drawing and hit testing
- Win32 event handling, presentation, scrolling, DPI refresh, and mouse callback dispatch

Use this layer when behavior is exposed to applications or tied to the control window.

### Public C surface

- `include/GuiTerminalC.h`
- `src/GuiTerminalC.cpp`

This is a thin wrapper around `GuiTerminal::Control`. Keep it in sync with public C++ changes.
If the exported API changes, update both declaration and forwarding implementation.

### Buffer and terminal model

- `include/GuiTerminalBuffer.h`
- `src/GuiTerminalBuffer.cpp`

This is the main state machine for terminal contents. It owns:

- terminal dimensions and default attributes
- per-region local buffers and region metadata
- region ordering / z-order
- region relocation, resize, clipping, scrolling, and coordinate conversion
- composed snapshots used by rendering
- cell-level draw helpers such as line/box/shading support
- terminal operations such as erase, fill, attribute resets, and SGR application

If a bug is about what a cell should contain, where a region should appear, how overlap works,
or how terminal state mutates over time, start here.

### ANSI parser

- `include/GuiTerminalParser.h`
- `src/GuiTerminalParser.cpp`

The parser is intentionally narrow:

- state machine: `Ground`, `Escape`, `Csi`
- dispatches cursor, erase, SGR, scroll, and save/restore commands into `Buffer`
- does not own rendering or storage

If ANSI input is accepted but behaves incorrectly, inspect parser dispatch and then the target
Buffer operation.

### Renderer

- `include/GuiTerminalRenderer.h`
- `src/GuiTerminalRenderer.cpp`

The renderer owns:

- Direct2D / DirectWrite device objects
- font metrics and cell geometry
- viewport layout and scrollbars
- client-pixel-to-cell hit testing
- drawing of the composed snapshot

Important boundary:

- visibility, occlusion, and final cell choice should already be resolved by `Buffer::GetSnapshot`
- `Renderer` should mainly transform cells into pixels

### Demo

- `demo/demo.cpp`
- `demo/demo.rc`

The demo is the fastest manual validation surface for:

- text styles and ANSI parsing
- region overlap and movement
- clipping / offscreen behavior
- line and box drawing helpers
- mouse callbacks and cell coordinate reporting

Use it to make feature behavior obvious, not just to keep the project building.

## Build and Packaging Files

### CMake

- `CMakeLists.txt`
- `cmake/GuiTerminalConfig.cmake.in`

This defines:

- the `GuiTerminal` library
- optional `GuiTerminalDemo`
- install/export behavior
- different install surfaces for static vs shared builds

### Visual Studio projects

- `GuiTerminal.slnx`
- `GuiTerminal.vcxproj`
- `GuiTerminalDll.vcxproj`
- `demo/GuiTerminalDemo.vcxproj`

These are still first-class local build paths. Do not assume CMake is the only supported flow.

### vcpkg port

- `vcpkg/ports/guiterminal/portfile.cmake`
- `vcpkg/ports/guiterminal/vcpkg.json`

Touch this when package shape, install headers, or linkage expectations change.

## Typical Ownership Decisions

### "Should this be in Buffer or Renderer?"

Put it in `Buffer` if it changes:

- which cells are visible
- how regions interact
- what glyph or attribute a cell should contain
- scroll/fill/erase/box/line semantics

Put it in `Renderer` if it changes:

- how cells map to pixels
- font metrics or DPI scaling
- scrollbar visuals or hit testing
- Direct2D / DirectWrite drawing behavior

### "Should this be in Parser or Buffer?"

Put it in `Parser` if it changes:

- which escape sequence maps to which operation
- CSI parameter handling
- parser state transitions

Put it in `Buffer` if it changes:

- what the operation actually does to terminal state

