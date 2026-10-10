# Write and draw terminal content

## Text and ANSI output

`Write` and `Print` accept null-terminated wide strings and parse GuiTerminal's supported ANSI
sequences for cursor movement, erasing, and text rendition. `WriteRegion` and `PrintRegion` do the
same within a region's independent cursor state.

```cpp
terminal->Write(L"Normal text\r\n");
terminal->Write(L"\x1b[1;33mYellow bold text\x1b[0m\r\n");
terminal->Print(L"Processed %d records.\r\n", recordCount);
```

ANSI row and column positions are one-based. Use explicit cell operations when constructing panels
or other fixed geometry, and do not assume unsupported ANSI sequences are available.

## Cell-based drawing

Root-grid methods use zero-based cell coordinates:

```cpp
terminal->Fill(0, 0, 100, 1, L' ', RGB(255U, 255U, 255U), RGB(48U, 68U, 108U), GuiTerminal::Control::StyleNone);
terminal->DrawBox(2, 3, 40, 10,
                  GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble,
                  RGB(210U, 230U, 255U), RGB(24U, 30U, 48U), GuiTerminal::Control::StyleNone);
terminal->DrawHorizontalLine(3, 5, 38, GuiTerminal::Control::StrokeSingleLine,
                             RGB(210U, 230U, 255U), RGB(24U, 30U, 48U), GuiTerminal::Control::StyleNone);
```

Use `Clear`, `Scroll`, and `Move` for the root grid. `Fill`, line methods, and `DrawBox` write
styled cells. Style flags include bold, underline, blink, inverse, and italic. Stroke types include
single and double lines, shades, and solid blocks.

## Cursor and mouse geometry

- `ShowCursor(region)` selects a root or region cursor; `HideCursor` hides it; `SetCursorStyle`
  selects block, underscore, or left-bar rendering.
- `GetCellFromPosition(clientX, clientY, ...)` maps a client pixel position to zero-based root-cell
  coordinates. `GetCellPosition(col, row, ...)` returns a visible cell's client rectangle.
- `GetTerminalSize`, `GetCellSize`, `GetPreferredClientSize`, and `GetPreferredWindowSize` support
  application layout.

For a clipped panel or an independent write origin, use a normal region rather than manually
offsetting root-grid calls.
