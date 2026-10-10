# Create and use normal regions

A normal region is a cell grid positioned inside the root terminal or another region. It clips its
children and owns independent cells, cursor state, ANSI state, and zero-based local drawing
coordinates.

## Create and draw a panel

The location passed to `CreateRegion` is in the parent region's cell coordinates. Pass `nullptr` as
the parent for a root-level region.

```cpp
GuiTerminal::RegionHandle statusRegion;
HRESULT hr;

statusRegion = nullptr;
hr = terminal->CreateRegion(2, 26, 96, 4, &statusRegion);
if (FAILED(hr))
{
    return hr;
}

terminal->FillRegion(statusRegion, 0, 0, 96, 4, L' ', RGB(230U, 235U, 245U), RGB(32U, 38U, 54U),
                     GuiTerminal::Control::StyleNone);
terminal->DrawRegionBox(statusRegion, 0, 0, 96, 4, GuiTerminal::Control::BoxSideTopDouble,
                        RGB(150U, 190U, 255U), RGB(32U, 38U, 54U), GuiTerminal::Control::StyleNone);
terminal->WriteRegion(statusRegion, L"\x1b[2;3HConnected\x1b[0m");
```

All region drawing starts at `(0, 0)` because region methods are local to `statusRegion`. ANSI
cursor positions, such as `2;3H`, remain one-based within the region.

## Hierarchy, layering, and state

Pass a region handle as the final `CreateRegion` argument to create a child. The child is positioned
in its parent and clipped by every ancestor. `RelocateRegion` moves or resizes a region,
`SetRegionVisible` hides or reveals a whole subtree, and `DestroyRegion` removes a region with all
descendants.

Sibling order controls overlap. Use `BringRegionToFront`, `SendRegionToBack`, or `MoveRegionAfter`
to change it. Associate application state with a region through `SetRegionContext` and
`GetRegionContext`; use `SetRegionDestroyCallback` when that state needs region-lifetime cleanup.

`GetRegionLocation` returns parent-relative geometry. `ConvertToRegionCoordinates` and
`ConvertFromRegionCoordinates` map root-terminal cells to and from region-local cells, returning
`FALSE` for coordinates outside the region.
