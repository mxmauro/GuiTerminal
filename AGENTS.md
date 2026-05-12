# GuiTerminal Agent Notes

## Scope
- These instructions apply to the whole repository.
- Follow the existing Windows-first C++ style instead of introducing generic cross-platform conventions.
- Preserve the current architecture: public headers in `include/`, implementation in `src/`, demo code in `demo/`, build and packaging files at the repo root, `cmake/`, `scripts/`, and `vcpkg/`.

## File format
- Use CRLF line endings for text files. This repository enforces CRLF in `.gitattributes`, and `utils/fixeol.bat` exists to normalize files.
- Keep text files ASCII or UTF-8 unless a file already uses another encoding.
- Do not introduce trailing whitespace.

## General coding style
- Indent with 4 spaces in handwritten code.
- Avoid tabs in handwritten files. If a file is already generator-produced or consistently tab-aligned throughout, leave that style in place instead of normalizing it opportunistically.
- Use braces on their own lines for namespaces, classes, structs, functions, and control-flow blocks.
- Keep a blank line between logical sections, but avoid excessive vertical whitespace.
- Use the existing separator comment style when it helps structure a file:

```cpp
// -----------------------------------------------------------------------------
```

- Prefer sparse, functional comments. Do not add narration for obvious code.
- Avoid style-only rewrites. Match the surrounding file instead of normalizing unrelated formatting.

## C++ and Win32 conventions
- This codebase is Windows-native. Prefer Win32 and COM-style types and APIs where the project already uses them: `HRESULT`, `BOOL`, `VOID`, `INT`, `DWORD`, `COLORREF`, `HWND`, `RECT`, `SIZE`.
- Use `WIN32_LEAN_AND_MEAN` before including `windows.h` in headers and source files that need it.
- Prefer wide-character Win32 APIs and wide string literals: `CreateWindowExW`, `L"..."`, `WCHAR`, `LPCWSTR`.
- Keep SAL annotations on public and internal APIs when touching signatures: `_In_`, `_Out_`, `_In_opt_`, `_Out_opt_`, `_In_z_`, `_In_reads_(...)`, etc.
- Continue returning Win32-style error codes. Use `E_POINTER`, `E_INVALIDARG`, `E_OUTOFMEMORY`, `E_UNEXPECTED`, `HRESULT_FROM_WIN32(...)`, and `SUCCEEDED` / `FAILED` as appropriate.
- Use `TRUE` / `FALSE` for `BOOL` values and `nullptr` for pointers.

## Naming
- Types, classes, structs, enums, namespaces, and methods use PascalCase: `Buffer`, `CursorState`, `CreateRegion`, `GuiTerminal::Internals`.
- The C API uses the exported prefix form already established in `include/GuiTerminalC.h`: `GuiTerminalControl_Create`, `GuiTerminalControl_WriteRegion`, etc.
- Macros use all caps with underscores: `TERMINAL_COLS`, `WINDOW_CLASS_NAME`.
- Follow the existing Hungarian-style prefixes for variables and fields. Common examples in this repo:
- `i` for signed integers: `iRows`, `iCursorX`
- `u` for unsigned or size/count values: `uIndex`, `uParamsCount`
- `f` for floating-point values: `fFontSize`
- `b` for booleans: `bBlinkVisible`, `bWrapPending`
- `by` for bytes: `byRed`
- `dw` for `DWORD`: `dwStyleFlags`
- `cr` for colors: `crForeground`
- `ch` for characters/codepoints: `chCodepointW`
- `h` for handles: `hWnd`, `hRegion`
- `lp` / `lplp` / `lpi` / `lph` for pointers and out-pointers
- `sz...W` for wide strings and buffers: `szTextW`, `szFontFamilyW`, `szBufferW`
- `s` for structs or value objects: `sConfig`, `sDefaultRegion`
- Member fields use the `m_` prefix: `m_iCols`, `m_vecCells`, `m_mapRegions`
- Preserve established names instead of renaming for style reasons.

## Layout and declarations
- Keep local includes first, then standard library headers.
- Internal project includes in `.cpp` files currently use quoted relative Windows paths such as `"..\\include\\GuiTerminalBuffer.h"`. Match the surrounding file instead of restyling include paths.
- File-local helper declarations should stay `static` and appear near the top of the translation unit before the main namespace or function bodies.
- Prefer explicit types over `auto`.
- Exception: `auto` is acceptable for iterator-style loops, `if`/`switch` init-statements, and similar cases where the deduced type is obvious and the alternative is noisier, matching existing patterns such as `for (const auto& ...)` and `if (auto it = ...; ...)`.
- Initialize variables explicitly, often at declaration time or immediately before first use:

```cpp
HRESULT hr;
hr = S_OK;

GuiTerminal::Control::Config sConfig;
sConfig = GuiTerminal::Control::Config{};
```

- Keep long parameter lists split across lines with the continuation aligned for readability, matching the current style.

## Control flow and implementation patterns
- Prefer early returns for argument validation and failure paths.
- When a function accepts optional handles or pointers, follow the established normalization pattern, for example defaulting a null region handle to the root region.
- Preserve the existing `switch` formatting with `case` labels indented inside the block and `break;` on its own line.
- Use standard-library helpers exactly as this codebase already does when macro collisions are possible, for example `(std::min)(...)` and `(std::max)(...)`.
- Keep exception handling narrow and compatibility-focused. Where the code catches allocation failures, continue mapping `std::bad_alloc` to `E_OUTOFMEMORY` and unexpected exceptions to `E_UNEXPECTED`.

## Headers and API surface
- Keep `#pragma once` in headers.
- Do not expose implementation details from `src/` through public headers unless the API change requires it.
- Preserve namespace structure and the distinction between public API types and `Internals`.
- For DLL export/import macros, follow the existing preprocessor style in `include/GuiTerminalC.h`.

## Build files
- Keep CMake formatting consistent with the existing file:
- lowercase commands such as `project`, `target_link_libraries`, `install`
- 4-space indentation inside command argument blocks
- grouped argument lists with one item per line when the block is long
- Do not change target names, install layout, or MSVC-specific options unless the task requires it.

## Demo and sample code
- The demo is also Windows-native and should continue using Win32 message-loop patterns, wide APIs, and `HRESULT`-based helpers.
- Keep demo constants as upper-case macros unless the surrounding code establishes a different pattern.

## Editing rules for agents
- Make the smallest change that solves the task.
- Preserve CRLF after editing. If needed, run `utils/fixeol.bat` or `utils/eolconverter.ps1` on touched text files.
- Do not reformat entire files unless explicitly requested.
- Do not replace Win32 types with STL or platform-neutral substitutes just for style.
- Do not remove SAL annotations, `noexcept`, or existing export/import macros without a real functional reason.
