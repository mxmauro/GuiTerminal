---
name: cmake-maintenance
description: "Maintain CMake targets, source lists, dependencies, package exports, install rules, and proportionate validation."
---

# CMake Maintenance

Use this skill when adding, renaming, or removing CMake-tracked sources; changing target definitions, dependencies, install
rules, package exports, or CMake build validation. Read repository guidance for target topology, supported generators, package
layout, and project-specific formatting.

## Target and package maintenance

- Keep target source lists, include directories, compile features, compile definitions, and link dependencies consistent with
  the target's public/private interface.
- When a public package surface changes, update the corresponding install rules and exported package configuration together.
- Keep dependency metadata consistent with the build files that consume it. Do not modify generated build trees, cache files,
  or installed artifacts.

## Validation

- Reconfigure when CMake logic, dependencies, options, or generated package files change.
- Build an affected configuration with a repository-supported generator and toolchain; use project guidance rather than
  assuming the host default generator is valid.
- Review install/export changes as both a build-tree and install-tree consumer when package behavior is affected.
