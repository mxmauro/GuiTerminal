# GuiTerminal AI integration index

This documentation helps an AI integrate and use GuiTerminal in a Windows desktop application.
It is installed by vcpkg at `share/guiterminal/docs/`. Read this file first, then open only the
topic relevant to the task.

## Choose the supported API

- Static GuiTerminal packages expose the C++ API. Include `GuiTerminalControl.h` and use
  `GuiTerminal::Control` plus `GuiTerminal::DrawContext`.
- Shared GuiTerminal packages expose `GuiTerminalC.h` only. This guide focuses on the C++ API;
  use the installed C header as the authority for the C wrapper.
- Although static packages also install `GuiTerminal::Internals` headers, application code should
  use `Control` and `DrawContext`. `Buffer`, `Parser`, and `Renderer` are lower-level library
  contracts, not normal integration entry points.

The installed headers are authoritative if they differ from this documentation.

## Read the needed topic

- [Integration](ai/integration.md): CMake, control creation, window-procedure forwarding, lifetime,
  and grid sizing.
- [Terminal content](ai/terminal-content.md): ANSI text, cell drawing, cursor state, and geometry.
- [Regions](ai/regions.md): child regions, clipping, layering, context, and coordinate conversion.
- [Custom drawing](ai/custom-drawing.md): Direct2D callbacks, invalidation, resource cleanup, and
  synchronization.

## Rules that apply to every integration

- Create the control during `WM_CREATE`, and forward window messages to `Control::WndProc` before
  application-specific handling.
- The window owns the control after `Control::Create`; retrieve it with `Control::GetControl` and
  do not delete it directly.
- Cell geometry uses zero-based coordinates. ANSI cursor coordinates are one-based. Custom-draw
  coordinates are device-independent pixels (DIPs).
- Check `HRESULT` from operations that create, resize, move, show, reorder, or register callbacks.
- Custom-draw and resource-cleanup callbacks run under GuiTerminal's control lock. Do not call
  `Control` methods from either callback.
