# GuiTerminal Agent Notes

## Scope

- These instructions apply to the whole repository.
- Preserve the current architecture: public headers in `include/`, implementation in `src/`,
  demo code in `demo/`, and build and packaging files at the repository root, `cmake/`,
  `scripts/`, and `vcpkg/`.

## Required Guidance

- Use `windows-working-principles` for shared engineering practices.
- Use `windows-cpp-style` for handwritten C++ source and headers.
- Use `cmake-maintenance` for CMake target, dependency, package, and validation work.
- Use `windows-msbuild-maintenance` when maintaining Visual Studio project metadata or file lists.
- Use `guiterminal-project` for this repository's architecture, public API surfaces, Win32 conventions,
  demo behavior, and project-specific build rules.
