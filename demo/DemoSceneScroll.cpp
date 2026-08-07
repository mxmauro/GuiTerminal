#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoLayout.h"
#include "DemoScenes.h"
#include <stdio.h>
#include <windows.h>

// -----------------------------------------------------------------------------

HRESULT DemoInitializeScrollScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                  _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept
{
    GuiTerminal::RegionHandle hRegionLog;
    WCHAR szBufferW[160];
    INT iLine;
    HRESULT hr;

    if ((!lpGuiTerminal) || (!hRegionScene) || (!lphCursorRegion))
    {
        return E_POINTER;
    }
    *lphCursorRegion = hRegionScene;
    hRegionLog = nullptr;
    lpGuiTerminal->FillRegion(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(225U, 240U, 255U), RGB(0U, 24U, 52U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, GuiTerminal::Control::BoxSideLeftDouble,
                                 RGB(150U, 220U, 255U), RGB(0U, 24U, 52U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[1;4H\x1b[48;2;0;24;52m\x1b[1;97m Scroll scene \x1b[0m"
                                             L"\x1b[2;3HWriteRegion keeps the newest lines visible inside a smaller child region."
                                             L"\x1b[3;3HThe pane below receives more lines than it can display.");
    hr = lpGuiTerminal->CreateRegion(3, 4, 70, 10, &hRegionLog, hRegionScene);
    if (FAILED(hr))
    {
        return hr;
    }
    lpGuiTerminal->FillRegion(hRegionLog, 0, 0, 70, 10, L' ', RGB(220U, 220U, 220U), RGB(8U, 12U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionLog, 0, 0, 70, 10,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideRightDouble, RGB(180U, 220U, 255U),
                                 RGB(8U, 12U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionLog, L"\x1b[1;3H\x1b[38;5;117mBuffered output\x1b[0m");
    for (iLine = 1; iLine <= 18; iLine++)
    {
        swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]),
                   L"\x1b[38;5;%dmline %02d  scroll sample 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n", 80 + (iLine % 8), iLine);
        lpGuiTerminal->WriteRegion(hRegionLog, szBufferW);
    }
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[5;80H\x1b[38;5;117mWhat to inspect\x1b[0m"
                                             L"\x1b[7;80H- child-region clipping\x1b[8;80H- retained background colors"
                                             L"\x1b[9;80H- line wrapping in a bounded region\x1b[10;80H- cursor targeting per scene");
    *lphCursorRegion = hRegionLog;
    return S_OK;
}
