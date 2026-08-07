#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoLayout.h"
#include "DemoScenes.h"
#include <windows.h>

// -----------------------------------------------------------------------------

static VOID DrawCustomChart(_In_ GuiTerminal::DrawContext &drawContext, _In_ GuiTerminal::RegionHandle hRegion) noexcept;

HRESULT DemoInitializeNestedScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                  _Out_ GuiTerminal::RegionHandle *lphCursorRegion) noexcept
{
    GuiTerminal::RegionHandle hRegionParent;
    GuiTerminal::RegionHandle hRegionBack;
    GuiTerminal::RegionHandle hRegionFront;
    GuiTerminal::RegionHandle hRegionBadge;
    GuiTerminal::RegionHandle hRegionChart;
    GuiTerminal::RegionHandle hRegionChartLabel;
    HRESULT hr;

    if ((!lpGuiTerminal) || (!hRegionScene) || (!lphCursorRegion))
    {
        return E_POINTER;
    }
    *lphCursorRegion = hRegionScene;
    hRegionParent = nullptr;
    hRegionBack = nullptr;
    hRegionFront = nullptr;
    hRegionBadge = nullptr;
    hRegionChart = nullptr;
    hRegionChartLabel = nullptr;
    lpGuiTerminal->FillRegion(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(18U, 18U, 18U), RGB(16U, 44U, 20U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, GuiTerminal::Control::BoxSideRightDouble,
                                 RGB(132U, 230U, 126U), RGB(16U, 44U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[1;4H\x1b[48;2;16;44;20m\x1b[1;97m Nested regions and local cursors \x1b[0m"
                                             L"\x1b[2;3HEach panel below is a real child region with its own clipping and write origin.");
    hr = lpGuiTerminal->CreateRegion(8, 4, 54, 10, &hRegionParent, hRegionScene);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(4, 2, 24, 6, &hRegionBack, hRegionParent);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(18, 3, 24, 5, &hRegionFront, hRegionParent);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(5, 1, 12, 3, &hRegionBadge, hRegionFront);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateCustomDrawRegion(108, 4, 46, 10, &hRegionChart, DrawCustomChart, hRegionScene);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(28, 6, 26, 4, &hRegionChartLabel, hRegionChart);
    if (FAILED(hr))
    {
        return hr;
    }
    lpGuiTerminal->FillRegion(hRegionParent, 0, 0, 54, 10, L' ', RGB(225U, 245U, 225U), RGB(24U, 62U, 28U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionParent, 0, 0, 54, 10,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideLeftDouble, RGB(225U, 255U, 225U),
                                 RGB(24U, 62U, 28U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionParent, L"\x1b[2;3HParent region");
    lpGuiTerminal->FillRegion(hRegionBack, 0, 0, 24, 6, L' ', RGB(20U, 20U, 20U), RGB(220U, 226U, 228U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionBack, 0, 0, 24, 6, GuiTerminal::Control::BoxSideBottomDouble, RGB(20U, 20U, 20U),
                                 RGB(220U, 226U, 228U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionBack, L"\x1b[2;3Hback child\r\n\x1b[4;3Hcursor target A");
    lpGuiTerminal->FillRegion(hRegionFront, 0, 0, 24, 5, L' ', RGB(30U, 20U, 0U), RGB(255U, 228U, 130U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionFront, 0, 0, 24, 5,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideRightDouble, RGB(50U, 32U, 0U),
                                 RGB(255U, 228U, 130U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionFront, L"\x1b[2;3Hfront child\r\n\x1b[4;3Hcursor target B");
    lpGuiTerminal->FillRegion(hRegionBadge, 0, 0, 12, 3, L' ', RGB(255U, 255U, 255U), RGB(180U, 56U, 56U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionBadge, 0, 0, 12, 3,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble, RGB(255U, 255U, 255U),
                                 RGB(180U, 56U, 56U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionBadge, L"\x1b[2;3Hbadge");
    lpGuiTerminal->FillRegion(hRegionChartLabel, 0, 0, 26, 4, L' ', RGB(24U, 24U, 24U), RGB(238U, 202U, 92U),
                              GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionChartLabel, 0, 0, 26, 4, GuiTerminal::Control::BoxSideTopDouble, RGB(24U, 24U, 24U),
                                 RGB(238U, 202U, 92U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionChartLabel, L"\x1b[2;2Hnormal child clips");
    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[6;72HVisible layering:\x1b[8;72H- parent host\x1b[9;72H- back child"
                               L"\x1b[10;72H- front child\x1b[11;72H- grandchild badge\x1b[13;72HCustom chart + clipped text child");
    *lphCursorRegion = hRegionBadge;
    return S_OK;
}

// -----------------------------------------------------------------------------

static VOID DrawCustomChart(_In_ GuiTerminal::DrawContext &drawContext, _In_ GuiTerminal::RegionHandle hRegion) noexcept
{
    const DWORD dwFrame = GetTickCount() / 10U;
    const FLOAT fWidth = static_cast<FLOAT>(drawContext.GetWidth());
    const FLOAT fHeight = static_cast<FLOAT>(drawContext.GetHeight());
    const FLOAT fWave1 = static_cast<FLOAT>(dwFrame % 40U) - 20.0f;
    const FLOAT fWave2 = static_cast<FLOAT>((dwFrame + 20U) % 40U) - 20.0f;
    const FLOAT fPoint1X = 18.0f;
    const FLOAT fPoint1Y = fHeight - 24.0f;
    const FLOAT fPoint2X = fWidth * 0.32f;
    const FLOAT fPoint2Y = (fHeight * 0.58f) + (fWave1 * 0.45f);
    const FLOAT fPoint3X = fWidth * 0.58f;
    const FLOAT fPoint3Y = (fHeight * 0.72f) + (fWave2 * 0.45f);
    const FLOAT fPoint4X = fWidth - 18.0f;
    const FLOAT fPoint4Y = (fHeight * 0.38f) + (fWave1 * 0.30f);
    const FLOAT fTextRotation = static_cast<FLOAT>(dwFrame % 360U);
    (void)hRegion;
    drawContext.Clear(RGB(20U, 38U, 66U));
    drawContext.DrawRectangle(D2D1::RectF(4.0f, 4.0f, fWidth - 4.0f, fHeight - 4.0f), RGB(130U, 220U, 255U), 4.0f);
    drawContext.Write(
        L"CUSTOM DRAW", D2D1::Point2F(fWidth / 2.0f, 8.0f), RGB(230U, 245U, 255U), L"Cascadia Mono", 12.0f, GuiTerminal::Control::StyleBold,
        static_cast<GuiTerminal::DrawContext::TextAlignment>(GuiTerminal::DrawContext::AlignTop | GuiTerminal::DrawContext::AlignCenter),
        fTextRotation);
    drawContext.DrawLine(12.0f, fHeight - 16.0f, fWidth - 12.0f, fHeight - 16.0f, RGB(90U, 150U, 190U), 1.0f);
    drawContext.FillRectangle(D2D1::RectF(14.0f, 44.0f, fWidth * 0.28f, fHeight - 30.0f), RGB(72U, 112U, 178U));
    drawContext.DrawRectangle(D2D1::RectF(14.0f, 44.0f, fWidth * 0.28f, fHeight - 30.0f), RGB(235U, 215U, 125U), 5.0f);
    drawContext.FillEllipse(D2D1::Ellipse(D2D1::Point2F(fWidth * 0.73f, fHeight * 0.38f), 22.0f, 16.0f), RGB(196U, 94U, 158U));
    drawContext.DrawEllipse(D2D1::Ellipse(D2D1::Point2F(fWidth * 0.73f, fHeight * 0.38f), 22.0f, 16.0f), RGB(255U, 224U, 246U), 3.0f);
    drawContext.DrawLine(fPoint1X, fPoint1Y, fPoint2X, fPoint2Y, RGB(102U, 230U, 172U), 3.0f);
    drawContext.DrawLine(fPoint2X, fPoint2Y, fPoint3X, fPoint3Y, RGB(102U, 230U, 172U), 3.0f);
    drawContext.DrawLine(fPoint3X, fPoint3Y, fPoint4X, fPoint4Y, RGB(102U, 230U, 172U), 3.0f);
    drawContext.DrawPoint(fPoint1X, fPoint1Y, RGB(240U, 255U, 255U), 5.0f);
    drawContext.DrawPoint(fPoint2X, fPoint2Y, RGB(240U, 255U, 255U), 5.0f);
    drawContext.DrawPoint(fPoint3X, fPoint3Y, RGB(240U, 255U, 255U), 5.0f);
    drawContext.DrawPoint(fPoint4X, fPoint4Y, RGB(240U, 255U, 255U), 5.0f);
    drawContext.BeginPath();
    drawContext.MoveTo(fWidth * 0.42f, 42.0f);
    drawContext.CubicBezierTo(fWidth * 0.50f, 18.0f, fWidth * 0.65f, 70.0f, fWidth * 0.76f, 42.0f);
    drawContext.QuadraticBezierTo(fWidth * 0.61f, 96.0f, fWidth * 0.42f, 42.0f);
    drawContext.ClosePath();
    drawContext.FillPath(RGB(72U, 150U, 202U));
    drawContext.StrokePath(RGB(210U, 246U, 255U), 2.0f);
}
