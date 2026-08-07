#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoLayout.h"
#include "DemoScenes.h"
#include <windows.h>

// -----------------------------------------------------------------------------

HRESULT DemoInitializeMoveScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept
{
    GuiTerminal::RegionHandle hRegionPanel;
    HRESULT hr;

    if ((!lpGuiTerminal) || (!hRegionScene) || (!lphCursorRegion))
    {
        return E_POINTER;
    }
    *lphCursorRegion = hRegionScene;
    hRegionPanel = nullptr;
    lpGuiTerminal->FillRegion(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(245U, 235U, 255U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT,
                                 GuiTerminal::Control::BoxSideLeftDouble | GuiTerminal::Control::BoxSideRightDouble, RGB(220U, 180U, 255U),
                                 RGB(52U, 18U, 72U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[1;4H\x1b[48;2;52;18;72m\x1b[1;97m Move and MoveRegion \x1b[0m"
                                             L"\x1b[2;3HThis scene includes in-bounds and clipped moves so vacated cells stay obvious.");
    lpGuiTerminal->FillRegion(hRegionScene, 6, 4, 36, 3, L'.', RGB(220U, 200U, 255U), RGB(76U, 28U, 102U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[5;9HSOURCE CELLS");
    lpGuiTerminal->MoveRegion(hRegionScene, 6, 4, 14, 1, 26, 6, L'=', RGB(255U, 240U, 180U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[8;9HMoveRegion() leaves '=' behind");
    lpGuiTerminal->FillRegion(hRegionScene, 124, 4, 24, 3, L'~', RGB(255U, 232U, 255U), RGB(112U, 36U, 140U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[5;127HCLIP RIGHT");
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[6;126Hsource >>>>>>>>");
    lpGuiTerminal->MoveRegion(hRegionScene, 124, 4, 24, 3, 144, 5, L'!', RGB(255U, 210U, 160U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->FillRegion(hRegionScene, 4, 12, 20, 2, L'%', RGB(255U, 245U, 200U), RGB(94U, 38U, 120U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[13;6HCLIP LEFT");
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[14;6H<<<<<< source");
    lpGuiTerminal->MoveRegion(hRegionScene, 4, 12, 20, 2, -5, 13, L'?', RGB(255U, 220U, 180U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->FillRegion(hRegionScene, 52, 11, 18, 4, L'@', RGB(255U, 240U, 220U), RGB(104U, 46U, 130U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[12;55HCLIP BOTTOM");
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[13;55Hvv source vv");
    lpGuiTerminal->MoveRegion(hRegionScene, 52, 11, 18, 4, 58, 15, L'/', RGB(255U, 220U, 170U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    hr = lpGuiTerminal->CreateRegion(82, 9, 48, 6, &hRegionPanel, hRegionScene);
    if (FAILED(hr))
    {
        return hr;
    }
    lpGuiTerminal->FillRegion(hRegionPanel, 0, 0, 48, 6, L'+', RGB(24U, 24U, 24U), RGB(214U, 184U, 238U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionPanel, 0, 0, 48, 6,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble, RGB(40U, 20U, 60U),
                                 RGB(214U, 184U, 238U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionPanel, L"\x1b[2;3Hchild panel");
    lpGuiTerminal->WriteRegion(hRegionPanel, L"\x1b[3;29Hclip top");
    lpGuiTerminal->WriteRegion(hRegionPanel, L"\x1b[4;4Hmove me");
    lpGuiTerminal->WriteRegion(hRegionPanel, L"\x1b[4;29H^^^^^^");
    lpGuiTerminal->MoveRegion(hRegionPanel, 3, 3, 7, 1, 37, 4, L'#', RGB(70U, 30U, 96U), RGB(214U, 184U, 238U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->MoveRegion(hRegionPanel, 28, 1, 14, 2, 40, -1, L'$', RGB(80U, 30U, 100U), RGB(214U, 184U, 238U),
                              GuiTerminal::Control::StyleBold);
    *lphCursorRegion = hRegionPanel;
    return S_OK;
}
