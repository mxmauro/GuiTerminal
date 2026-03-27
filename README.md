# GuiTerminal

GuiTerminal is a native C++ terminal control for Win32 desktop applications. It provides a custom terminal-like surface that can be embedded into your own windowed UI and rendered with Direct2D and DirectWrite.

The library is intended for applications that need terminal behavior without hosting an external console window. Typical use cases include embedded shells, device consoles, developer tools, log viewers, retro-style interfaces, and applications that want ANSI-style formatted text inside a standard Win32 GUI.

![Demo screenshot](screenshot.webp "Demo screenshot")

## What the library does

GuiTerminal gives you a reusable control that:

- Creates and manages a terminal surface inside a Win32 window.
- Maintains a character-cell buffer with per-cell foreground and background colors, and style attributes.
- Parses ANSI escape sequences for cursor movement, text styling, and color changes.
- Renders the terminal contents with Direct2D and DirectWrite.
- Supports writing to the full terminal or to sub-regions inside the terminal.
- Supports scrolling and region-based updates.

## NuGet package

### Contents

- Public headers under `build/native/include`
- `GuiTerminal.lib` library for both `Win32` and `x64` platforms and `Debug` and `Release` configurations.
- Native MSBuild props that add include paths, library paths, and the required linker inputs.

### Pack locally

```powershell
.\nuget\pack.ps1 -Version 1.0.0
```

## LICENSE

[MIT](/LICENSE)
