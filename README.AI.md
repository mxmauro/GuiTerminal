# GuiTerminal AI Guidance

GuiTerminal's AI-oriented integration documentation is installed with the vcpkg package. The
package path is independent of whether the project calls its vcpkg installed root `installed`,
`.packages`, or something else.

## Use from a consuming project

Add this focused rule to the consuming project's `AGENTS.md`:

```md
## GuiTerminal

When a task integrates, configures, or uses GuiTerminal, locate the project's vcpkg installed root
and read `share/guiterminal/docs/ai-index.md` before changing the application. For example, a
project that installs packages in `.packages` uses:

.packages/<triplet>/share/guiterminal/docs/ai-index.md

Read only the linked topic documents needed for the task. Treat the installed GuiTerminal headers
as the API authority if they differ from the documentation.
```

The index routes integration, terminal-content, regions, and Direct2D custom-drawing work to the
corresponding files under `share/guiterminal/docs/ai/`.
