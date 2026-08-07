#pragma once

#include "..\include\GuiTerminalControl.h"

HRESULT DemoInitializeScrollScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                  _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept;
HRESULT DemoInitializeBoxesScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                 _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept;
HRESULT DemoInitializeNestedScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                  _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept;
HRESULT DemoInitializeMoveScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept;
