# GuiTerminal Workflows

## Common Change Types

### Public API change

Touch, as needed:

- `include/GuiTerminalControl.h`
- `src/GuiTerminalControl.cpp`
- `include/GuiTerminalC.h`
- `src/GuiTerminalC.cpp`
- `demo/demo.cpp` if the feature is user-visible
- `README.md` if the public feature set or usage changed

Check for:

- SAL consistency
- wide-character / Win32 style consistency
- matching names between C++ and C surfaces
- error-path behavior remaining `HRESULT` / `BOOL` friendly

### Buffer / terminal semantics change

Touch, as needed:

- `include/GuiTerminalBuffer.h`
- `src/GuiTerminalBuffer.cpp`
- `src/GuiTerminalParser.cpp` if ANSI dispatch is involved
- `demo/demo.cpp` for visible coverage

Check for:

- region-local versus terminal-global coordinates
- resize and allocation failure behavior staying transactional where required
- snapshot composition remaining renderer-agnostic
- interactions with scrolling, erase, fill, and cursor movement

### Rendering or hit-testing change

Touch, as needed:

- `include/GuiTerminalRenderer.h`
- `src/GuiTerminalRenderer.cpp`
- `src/GuiTerminalControl.cpp` if control event handling or presentation changes
- `demo/demo.cpp` if the visual result should be demonstrated

Check for:

- DPI scaling
- scrollbar visibility and offsets
- viewport clipping
- whether the logic should really live in Buffer instead

### Packaging or install change

Touch, as needed:

- `CMakeLists.txt`
- `cmake/GuiTerminalConfig.cmake.in`
- `vcpkg/ports/guiterminal/*`
- Visual Studio project files only if the local build path also needs updating

Check for:

- static vs shared install surface
- exported target shape
- header install set
- demo build constraints when shared builds are enabled

## Validation

### Standard validation for source changes

1. Run EOL normalization:

```powershell
& .\utils\fixeol.bat
```

2. Build the Visual Studio solution:

```powershell
& 'C:\Lenguaje\Microsoft Visual Studio 2026\MSBuild\Current\Bin\MSBuild.exe' 'GuiTerminal.slnx' /t:Build /p:Configuration=Debug /p:Platform=x64
```

3. If user-visible behavior changed, inspect or update `demo/demo.cpp` accordingly.

### When to expand validation

- If CMake or install logic changed, run the relevant CMake configure/build path too.
- If vcpkg packaging changed, inspect the port files and package shape.
- If parser or rendering behavior changed, add or adjust explicit demo coverage so the behavior is observable.

## Editing Checklist

- Read `AGENTS.md` before editing.
- Prefer the smallest owning layer.
- Keep C++ and C wrappers aligned.
- Keep CRLF line endings.
- Keep source lines within 140 characters.
- Use dense wrapping up to the width limit rather than one-item-per-line formatting unless readability demands it.
- Do not do style-only rewrites outside the changed area.
