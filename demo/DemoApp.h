#pragma once

#include "..\include\GuiTerminalControl.h"

HRESULT DemoInitialize(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept;
VOID DemoHandleMouseButton(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown,
                           _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept;
