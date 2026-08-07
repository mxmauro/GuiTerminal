# GuiTerminal Agent Notes

## Scope
- These instructions apply to the whole repository.
- Follow the existing Windows-first C++ style instead of introducing generic cross-platform conventions.
- Preserve the current architecture: public headers in `include/`, implementation in `src/`,
  demo code in `demo/`, build and packaging files at the repo root, `cmake/`, `scripts/`,
  and `vcpkg/`.

## Working principles
- Think before coding. State assumptions explicitly, surface ambiguity or tradeoffs instead of picking silently, and ask for clarification when requirements or nearby code are unclear.
- Simplicity first. Make the smallest change that solves the task. Do not add speculative abstractions, configurability, or broader refactors that were not requested.
- Surgical changes. Keep diffs limited to the requested behavior, tests, and directly required project or documentation updates. Do not normalize unrelated formatting or fix adjacent issues unless they block the task. Remove only code or declarations that your change made unused.
- Goal-driven execution. Define the verification target before editing, prefer focused build or test checks that prove the changed behavior, and broaden validation only as the change scope requires.

## File format
- Use CRLF line endings for text files. This repository enforces CRLF in `.gitattributes`, and `utils/fixeol.bat` exists to normalize files.
- Keep text files ASCII or UTF-8 unless a file already uses another encoding.
- Do not introduce trailing whitespace.

## General coding style
- Use Allman style as the baseline: place opening braces on their own line for declarations,
  definitions, and control-flow blocks.
- Always enclose `if`, `do`, `while`, `for`, and `else` bodies in curly braces, including
  one-line bodies.
- Indent with 4 spaces in handwritten code.
- Avoid tabs in handwritten files. If a file is already generator-produced or consistently
  tab-aligned throughout, leave that style in place instead of normalizing it opportunistically.
- Keep lines at or below 140 characters. Split long statements and parameter lists for readability.
- Exception: keep the opening brace on the same line for `typedef struct` and `typedef enum` declarations, for example
  `typedef struct Name_s {`.
- All project-owned structs and enums use typedef declarations with `_s` and `_e` tags respectively.
- Do not indent declarations or definitions inside `extern "C"` blocks.
- Use compact namespace formatting in handwritten code: `namespace X {`, and do not add an extra indentation level solely for namespace scope.
- Comment every namespace and `extern "C"` closing brace with the scope it closes, for example `} // namespace GuiTerminal`.
- Keep exactly one blank line between function definitions.
- Keep a blank line between other logical sections, but avoid excessive vertical whitespace.
- Use the existing separator comment style when it helps structure a file:

```cpp
// -----------------------------------------------------------------------------
```

- Prefer sparse, functional comments. Do not add narration for obvious code.
- Avoid style-only rewrites. Match the surrounding file instead of normalizing unrelated formatting.

## C++ and Win32 conventions
- This codebase is Windows-native. Prefer Win32 and COM-style types and APIs where the
  project already uses them: `HRESULT`, `BOOL`, `VOID`, `INT`, `DWORD`, `COLORREF`, `HWND`,
  `RECT`, `SIZE`.
- Use `WIN32_LEAN_AND_MEAN` before including `windows.h` in headers and source files that need it.
- Prefer wide-character Win32 APIs and wide string literals: `CreateWindowExW`, `L"..."`, `WCHAR`, `LPCWSTR`.
- Keep SAL annotations on public and internal APIs when touching signatures: `_In_`, `_Out_`,
  `_In_opt_`, `_Out_opt_`, `_In_z_`, `_In_reads_(...)`, etc.
- Continue returning Win32-style error codes. Use `E_POINTER`, `E_INVALIDARG`,
  `E_OUTOFMEMORY`, `E_UNEXPECTED`, `HRESULT_FROM_WIN32(...)`, and `SUCCEEDED` / `FAILED`
  as appropriate.
- Use `TRUE` / `FALSE` for `BOOL` values and `nullptr` for pointers.

## Naming
- Types, classes, structs, enums, namespaces, and methods use PascalCase: `Buffer`, `CursorState`, `CreateRegion`, `GuiTerminal::Internals`.
- The C API uses the exported prefix form already established in
  `include/GuiTerminalC.h`: `GuiTerminalControl_Create`, `GuiTerminalControl_WriteRegion`,
  etc.
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
- Internal project includes in `.cpp` files currently use quoted relative Windows paths such as
  `"..\\include\\GuiTerminalBuffer.h"`. Match the surrounding file instead of restyling
  include paths.
- Order file-scope declarations as typedefs, constants, variables, static helper declarations, functions, and static helper implementations.
- File-local helper declarations should stay `static` near the top of the translation unit before the main namespace or function bodies.
- Within each access level, separate typedefs, methods, and variables into their own explicitly labeled access blocks.
- Prefer explicit types over `auto`.
- Exception: `auto` is acceptable for iterator-style loops, `if`/`switch` init-statements,
  and similar cases where the deduced type is obvious and the alternative is noisier,
  matching existing patterns such as `for (const auto& ...)` and `if (auto it = ...; ...)`.
- Initialize variables explicitly, often at declaration time or immediately before first use:

```cpp
HRESULT hr;
hr = S_OK;

GuiTerminal::Control::Config sConfig;
sConfig = GuiTerminal::Control::Config{};
```

- Keep long parameter lists split across lines with the continuation aligned one character after the outer opening parenthesis.
- Apply the same parenthesis alignment rule to multiline expressions and declarations.
- When wrapping declarations, calls, or expressions, fill each line as much as practical up to the
  140-character limit instead of using one-argument-per-line formatting unless readability clearly
  requires it.

## Control flow and implementation patterns
- Prefer early returns for argument validation and failure paths.
- When a function accepts optional handles or pointers, follow the established normalization
  pattern, for example defaulting a null region handle to the root region.
- Preserve the existing `switch` formatting with `case` labels indented inside the block and `break;` on its own line.
- Use standard-library helpers exactly as this codebase already does when macro collisions are
  possible, for example `(std::min)(...)` and `(std::max)(...)`.
- Prefer classic fixed-size arrays over `std::array` for file-local lookup tables.
- Keep exception handling narrow and compatibility-focused. Where the code catches allocation
  failures, continue mapping `std::bad_alloc` to `E_OUTOFMEMORY` and unexpected exceptions
  to `E_UNEXPECTED`.

## Expression parentheses
- Use parentheses to clarify precedence, not mechanically around every subexpression.
- Avoid redundant parentheses in simple homogeneous boolean chains such as `a && b && c` or `a || b || c` when precedence is obvious.
- Add parentheses when mixing `&&` and `||` in the same expression.
- Wrap unary negation subexpressions in parentheses when they appear alongside other boolean terms, including expressions with more than two subexpressions:
  `if ((!var1) || (!var2))`
  `if (var1 && (!var2))`
  `if (var1 && (!var2) && var3)`
  `if ((!var1) || var2 || (!var3))`
- A standalone negation does not need extra grouping:
  `if (!var)`
- Add parentheses when a unary operator such as `!` applies to only part of a larger expression and the scope could be misread.
- Prefer `if ((!ptr) || (*ptr == 0))` over `if (!ptr || *ptr == 0)` when the grouped intent is clearer.
- Prefer `return ((a && b) || (c && d)) ? TRUE : FALSE;` over
  `return (a && b || c && d) ? TRUE : FALSE;` when mixed boolean operators are involved.
- Do not add extra grouping that does not improve readability.

## Headers and API surface
- Keep `#pragma once` in headers.
- Do not expose implementation details from `src/` through public headers unless the API change requires it.
- Preserve namespace structure and the distinction between public API types and `Internals`.
- For DLL export/import macros, follow the existing preprocessor style in `include/GuiTerminalC.h`.
- When the exported C API changes, update all export surfaces in the same change:
  `include/GuiTerminalC.h`, `src/GuiTerminalC.cpp`, and `GuiTerminal.def`.

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
- Preserve CRLF after editing. If needed, run `utils/fixeol.bat` or `utils/eolconverter.ps1` on touched text files.
- Do not reformat entire files unless explicitly requested.
- Do not replace Win32 types with STL or platform-neutral substitutes just for style.
- Do not remove SAL annotations, `noexcept`, or existing export/import macros without a real functional reason.
