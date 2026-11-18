---
name: windows-cpp-style
description: Apply a Win32-aware C and C++ style when editing handwritten native Windows source or headers while preserving local SDK conventions.
---

# Windows C/C++ Style

Use this skill for handwritten native Windows C or C++ source and headers. Local `AGENTS.md`, `.editorconfig`, `.gitattributes`, and surrounding first-party code decide naming, encoding, line endings, and API conventions.

## Layout and declarations

- Use the repository's established indentation, encoding, and line endings. In the absence of local rules, use four spaces, a final newline, no trailing whitespace, and practical lines of at most 140 characters.
- Use Allman braces for declarations and control flow where the repository follows that style. Always brace `if`, `else`, `for`, `while`, and `do` bodies. Preserve same-line `typedef struct` or `typedef enum` forms where established.
- Keep namespaces, blank lines, wrapping, `switch` labels, separator comments, include order, and file-local helper placement consistent with adjacent code.
- Prefer explicit types; use `auto` only where the deduced type is clear and matches local practice. Initialize variables at declaration or immediately before first use, and prefer early validation/failure returns.

## Windows conventions

- Follow the codebase's established Win32 types and APIs. When it uses `HRESULT`, `BOOL`, SAL annotations, COM patterns, or wide-character APIs, preserve the associated success/failure and ownership conventions.
- Preserve local class, type, member, parameter, and variable naming, including Hungarian prefixes and legacy SDK patterns. Do not rename established symbols merely to modernize them.
- Retain API qualifiers and ABI-relevant details such as `extern "C"`, `noexcept`, calling conventions, packed layouts, bitfields, deleted operations, and binary-compatible public declarations unless the task explicitly changes the interface.

## Expressions and comments

- Use parentheses where they clarify precedence, especially mixed `&&`/`||` chains or unary negation in a larger expression; avoid mechanical redundant grouping.
- Write sparse functional comments that explain ownership, constraints, intent, or section purpose. Do not narrate obvious statements.
- Match the surrounding file and avoid style-only rewrites outside the code directly touched by the task.
