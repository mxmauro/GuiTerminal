---
name: "guiterminal-project"
description: "Project-specific guidance for working in the GuiTerminal repository: a Win32 C++ terminal control built around Buffer, Parser, Renderer, Control, a C API wrapper, a demo app, and CMake/vcpkg packaging. Use when modifying files in this repo, especially for terminal behavior, sub-regions, rendering, ANSI parsing, exported APIs, demo changes, or build/install updates."
---

# GuiTerminal Project

Read [AGENTS.md](../../../AGENTS.md) first and follow it over this skill when they overlap.

## Quick Start

Use this skill when the task is specific to this repository rather than generic C++ work.

Start by locating the change in the smallest layer that owns the behavior:

- `include/` defines the public C++ and C surfaces plus internal Buffer/Parser/Renderer contracts.
- `src/GuiTerminalBuffer.cpp` owns terminal state, region state, composition, cell mutation, and most terminal semantics.
- `src/GuiTerminalParser.cpp` translates ANSI text input into Buffer operations.
- `src/GuiTerminalRenderer.cpp` turns composed cells into Direct2D/DirectWrite output and scrollbar behavior.
- `src/GuiTerminalControl.cpp` is the Win32-facing facade and event bridge.
- `src/GuiTerminalC.cpp` mirrors the C++ surface for the exported C API.
- `demo/demo.cpp` is the manual validation app and should reflect new user-visible features.

Load [references/architecture.md](references/architecture.md) when you need the file map,
ownership boundaries, or cross-component relationships.

Load [references/workflows.md](references/workflows.md) when you need task-specific checklists,
edit scope guidance, or validation commands.

## Working Rules

- Keep the Win32-native design. Prefer `HRESULT`, `BOOL`, SAL, wide APIs, and the existing naming style.
- Preserve the public/internal split: public interfaces in `include/`, implementation in `src/`.
- When you change a public operation in C++, check whether the C wrapper and demo should change too.
- When you change the exported C API, update `include/GuiTerminalC.h`, `src/GuiTerminalC.cpp`, and `GuiTerminal.def` together.
- Treat the demo as a real consumer. If behavior changes materially, update or extend it.
- Use `apply_patch` for edits, keep CRLF, and run `utils/fixeol.bat` on touched files before finishing.
- Keep lines within the repo's 140-character limit and pack wrapped signatures/calls efficiently instead of using one-argument-per-line formatting by default.

## Change Map

For most tasks, use this order:

1. Confirm the owning layer.
2. Change the header only if the API or internal contract truly changes.
3. Update the implementation in the matching `.cpp`.
4. Propagate the change across adjacent surfaces:
   - C++ API change: update `include/GuiTerminalControl.h`, `src/GuiTerminalControl.cpp`, and usually `include/GuiTerminalC.h` plus `src/GuiTerminalC.cpp`.
   - Buffer semantic change: check Parser, Control, Renderer, and demo impact.
   - Rendering change: confirm whether the change belongs in composition (`Buffer`) or drawing (`Renderer`).
5. Update the demo for visible features or new manual test coverage.
6. Run the relevant validation commands from `references/workflows.md`.

## High-Value Reminders

- Region-local cursor semantics are intentionally one-based for ANSI-facing cursor APIs.
- Composition and visibility rules belong in `Buffer`; the renderer should draw the composed snapshot, not re-decide occlusion.
- ANSI parsing here is intentionally small and explicit; extend it conservatively and route behavior through Buffer operations.
- Packaging changes must keep CMake, Visual Studio projects, and vcpkg expectations aligned.
