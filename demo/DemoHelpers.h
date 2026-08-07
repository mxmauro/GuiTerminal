#pragma once

#include "..\include\GuiTerminalControl.h"

VOID DemoWriteCenteredText(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ INT iCol, _In_ INT iRow, _In_ INT iWidth, _In_z_ LPCWSTR szTextW,
                           _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ BOOL bBold) noexcept;
