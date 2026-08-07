#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoLayout.h"
#include "DemoScenes.h"
#include <windows.h>

// -----------------------------------------------------------------------------

HRESULT DemoInitializeBoxesScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                 _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept
{
    if ((!lpGuiTerminal) || (!hRegionScene) || (!lphCursorRegion))
    {
        return E_POINTER;
    }
    *lphCursorRegion = hRegionScene;
    lpGuiTerminal->FillRegion(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(35U, 26U, 0U), RGB(48U, 34U, 0U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble, RGB(255U, 210U, 120U),
                                 RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[1;4H\x1b[48;2;48;34;0m\x1b[1;97m Boxes and crossing lines \x1b[0m"
                                             L"\x1b[2;3HGlobal-style drawing helpers also work inside a full overlay region.");
    lpGuiTerminal->DrawRegionBox(hRegionScene, 4, 4, 42, 9,
                                 GuiTerminal::Control::BoxSideLeftDouble | GuiTerminal::Control::BoxSideTopDouble, RGB(255U, 220U, 150U),
                                 RGB(72U, 48U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 18, 6, 36, 7,
                                 GuiTerminal::Control::BoxSideRightDouble | GuiTerminal::Control::BoxSideBottomDouble,
                                 RGB(255U, 240U, 200U), RGB(90U, 58U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionScene, 8, 8, 70, GuiTerminal::Control::StrokeSingleLine, RGB(255U, 230U, 170U),
                                            RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionVerticalLine(hRegionScene, 28, 3, 11, GuiTerminal::Control::StrokeDoubleLine, RGB(255U, 245U, 200U),
                                          RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionScene, 82, 5, 44, GuiTerminal::Control::StrokeShadeLight, RGB(240U, 240U, 240U),
                                            RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionScene, 82, 7, 44, GuiTerminal::Control::StrokeShadeMedium, RGB(240U, 240U, 240U),
                                            RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionScene, 82, 9, 44, GuiTerminal::Control::StrokeShadeDark, RGB(240U, 240U, 240U),
                                            RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionScene, 82, 11, 44, GuiTerminal::Control::StrokeSolidBlock, RGB(240U, 240U, 240U),
                                            RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionVerticalLine(hRegionScene, 112, 4, 9, GuiTerminal::Control::StrokeSingleLine, RGB(180U, 225U, 255U),
                                          RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(
        hRegionScene, L"\x1b[6;8Hsingle + double joins\x1b[10;10Hcrossing strokes\x1b[5;84Hlight shade"
                      L"\x1b[7;84Hmedium shade\x1b[9;84Hdark shade\x1b[11;84Hsolid block\x1b[13;84Hmixed line families keep intersections");
    return S_OK;
}
