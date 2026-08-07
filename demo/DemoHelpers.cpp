#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoHelpers.h"
#include <stdio.h>
#include <windows.h>

// -----------------------------------------------------------------------------

VOID DemoWriteCenteredText(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ INT iCol, _In_ INT iRow, _In_ INT iWidth, _In_z_ LPCWSTR szTextW,
                           _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ BOOL bBold) noexcept
{
    INT iTextLength;
    INT iStartCol;

    if ((!lpGuiTerminal) || (!szTextW))
    {
        return;
    }

    iTextLength = static_cast<INT>(wcslen(szTextW));
    iStartCol = iCol + ((iWidth - iTextLength) / 2);
    if (iStartCol < iCol)
    {
        iStartCol = iCol;
    }

    lpGuiTerminal->Print(L"\x1b[%d;%dH\x1b[38;2;%u;%u;%um\x1b[48;2;%u;%u;%um%ls%ls\x1b[0m", iRow + 1, iStartCol + 1,
                         static_cast<unsigned>(GetRValue(crForeground)), static_cast<unsigned>(GetGValue(crForeground)),
                         static_cast<unsigned>(GetBValue(crForeground)), static_cast<unsigned>(GetRValue(crBackground)),
                         static_cast<unsigned>(GetGValue(crBackground)), static_cast<unsigned>(GetBValue(crBackground)),
                         (bBold != FALSE) ? L"\x1b[1m" : L"", szTextW);
}
