#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoApp.h"
#include "DemoHelpers.h"
#include "DemoLayout.h"
#include "DemoScenes.h"
#include <stdio.h>
#include <windows.h>

// -----------------------------------------------------------------------------

typedef enum DemoSceneId_e {
    DemoSceneScroll = 0,
    DemoSceneBoxes,
    DemoSceneNested,
    DemoSceneMove
} DemoSceneId;

typedef struct DemoSceneDefinition_s {
    LPCWSTR szButtonCaptionTopW;
    LPCWSTR szButtonCaptionBottomW;
    COLORREF crButtonForeground;
    COLORREF crButtonBackground;
} DemoSceneDefinition;

typedef struct DemoSceneState_s {
    GuiTerminal::RegionHandle hRegion;
    GuiTerminal::RegionHandle hCursorRegion;
    GuiTerminal::Control::CursorStyle cursorStyle;
} DemoSceneState;

typedef struct DemoState_s {
    GuiTerminal::RegionHandle hRegionStatus;
    DemoSceneState arrScenes[DEMO_SCENE_COUNT];
    INT iActiveScene;
} DemoState;

// -----------------------------------------------------------------------------

static VOID DrawTopRowSamples(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept;
static VOID DrawButtons(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept;
static VOID DrawButton(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ INT iSceneIndex, _In_ BOOL bActive) noexcept;
static HRESULT InitializeScenes(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept;
static VOID SelectScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ INT iSceneIndex) noexcept;
static INT HitTestButton(_In_ INT iCol, _In_ INT iRow) noexcept;
static VOID GetButtonRect(_In_ INT iSceneIndex, _Out_ LPINT lpiX, _Out_ LPINT lpiY, _Out_ LPINT lpiWidth, _Out_ LPINT lpiHeight) noexcept;
static VOID WriteMouseStatus(_In_ GuiTerminal::Control *lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW,
                             _In_ INT iX, _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept;

// -----------------------------------------------------------------------------

static const DemoSceneDefinition g_arrSceneDefinitions[DEMO_SCENE_COUNT] = {
    {L"SCENE 1", L"SCROLL", RGB(16U, 28U, 40U), RGB(120U, 205U, 255U)},
    {L"SCENE 2", L"BOXES", RGB(48U, 24U, 0U), RGB(255U, 196U, 90U)},
    {L"SCENE 3", L"NESTED", RGB(18U, 18U, 18U), RGB(132U, 230U, 126U)},
    {L"SCENE 4", L"MOVE", RGB(255U, 255U, 255U), RGB(176U, 90U, 216U)}};

static DemoState g_sDemoState{};

// -----------------------------------------------------------------------------

HRESULT DemoInitialize(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept
{
    HRESULT hr;

    if (!lpGuiTerminal)
    {
        return E_POINTER;
    }

    lpGuiTerminal->Clear();
    g_sDemoState = DemoState{};
    g_sDemoState.iActiveScene = DemoSceneScroll;
    DrawTopRowSamples(lpGuiTerminal);
    hr = lpGuiTerminal->CreateRegion(0, DEMO_STATUS_Y, TERMINAL_COLS, 1, &g_sDemoState.hRegionStatus);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = InitializeScenes(lpGuiTerminal);
    if (FAILED(hr))
    {
        return hr;
    }
    DrawButtons(lpGuiTerminal);
    SelectScene(lpGuiTerminal, DemoSceneScroll);
    WriteMouseStatus(lpGuiTerminal, L"Ready", L"", -1, -1, -1, -1);
    return S_OK;
}

VOID DemoHandleMouseButton(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown,
                           _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept
{
    INT iCol;
    INT iRow;
    INT iSceneHit;

    if (!lpGuiTerminal)
    {
        return;
    }
    iCol = -1;
    iRow = -1;
    if (lpGuiTerminal->GetCellFromPosition(iX, iY, &iCol, &iRow) == FALSE)
    {
        iCol = -1;
        iRow = -1;
    }
    if ((bLeftButton != FALSE) && (bButtonDown != FALSE))
    {
        iSceneHit = HitTestButton(iCol, iRow);
        if (iSceneHit >= 0)
        {
            SelectScene(lpGuiTerminal, iSceneHit);
        }
    }
    WriteMouseStatus(lpGuiTerminal, szButtonNameW, szActionNameW, iX, iY, iCol, iRow);
}

static VOID DrawTopRowSamples(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept
{
    lpGuiTerminal->Write(L"\x1b[1;1HGuiTerminal demo: \x1b[38;2;255;170;40mTruecolor FG\x1b[0m  "
                         L"\x1b[48;2;0;96;160m\x1b[97mTruecolor BG\x1b[0m  \x1b[4munderline\x1b[24m  "
                         L"\x1b[1mbold\x1b[22m  \x1b[5mblink\x1b[25m  \x1b[32mgreen\x1b[0m  \x1b[33myellow\x1b[0m  "
                         L"\x1b[34mblue\x1b[0m  \x1b[91mbright red\x1b[0m  \x1b[38;5;141m256-color\x1b[0m");
}

static VOID DrawButtons(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept
{
    INT iSceneIndex;
    for (iSceneIndex = 0; iSceneIndex < DEMO_SCENE_COUNT; iSceneIndex++)
    {
        DrawButton(lpGuiTerminal, iSceneIndex, (g_sDemoState.iActiveScene == iSceneIndex) ? TRUE : FALSE);
    }
}

static VOID DrawButton(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ INT iSceneIndex, _In_ BOOL bActive) noexcept
{
    const DemoSceneDefinition *lpsDefinition;
    COLORREF crBackground;
    COLORREF crForeground;
    INT iX;
    INT iY;
    INT iWidth;
    INT iHeight;

    lpsDefinition = &g_arrSceneDefinitions[iSceneIndex];
    GetButtonRect(iSceneIndex, &iX, &iY, &iWidth, &iHeight);
    crBackground = (bActive != FALSE)
                       ? lpsDefinition->crButtonBackground
                       : RGB(GetRValue(lpsDefinition->crButtonBackground) / 2U, GetGValue(lpsDefinition->crButtonBackground) / 2U,
                             GetBValue(lpsDefinition->crButtonBackground) / 2U);
    crForeground = (bActive != FALSE) ? RGB(255U, 255U, 255U) : lpsDefinition->crButtonForeground;
    lpGuiTerminal->Fill(iX, iY, iWidth, iHeight, L' ', crForeground, crBackground,
                        (bActive != FALSE) ? GuiTerminal::Control::StyleBold : GuiTerminal::Control::StyleNone);
    DemoWriteCenteredText(lpGuiTerminal, iX, iY, iWidth, lpsDefinition->szButtonCaptionTopW, crForeground, crBackground, bActive);
    DemoWriteCenteredText(lpGuiTerminal, iX, iY + 1, iWidth, lpsDefinition->szButtonCaptionBottomW, crForeground, crBackground, bActive);
}

static HRESULT InitializeScenes(_In_ GuiTerminal::Control *lpGuiTerminal) noexcept
{
    GuiTerminal::RegionHandle hRegionScene;
    GuiTerminal::RegionHandle hCursorRegion;
    HRESULT hr;
    INT iSceneIndex;

    for (iSceneIndex = 0; iSceneIndex < DEMO_SCENE_COUNT; iSceneIndex++)
    {
        hRegionScene = nullptr;
        hCursorRegion = nullptr;
        hr = lpGuiTerminal->CreateRegion(DEMO_SCENE_X, DEMO_SCENE_Y, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, &hRegionScene);
        if (FAILED(hr))
        {
            return hr;
        }
        g_sDemoState.arrScenes[iSceneIndex].hRegion = hRegionScene;
        g_sDemoState.arrScenes[iSceneIndex].hCursorRegion = hRegionScene;
        switch (iSceneIndex)
        {
        case DemoSceneScroll:
            hr = DemoInitializeScrollScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
            g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorUnderscore;
            break;
        case DemoSceneBoxes:
            hr = DemoInitializeBoxesScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
            g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorBarLeft;
            break;
        case DemoSceneNested:
            hr = DemoInitializeNestedScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
            g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorBlock;
            break;
        case DemoSceneMove:
            hr = DemoInitializeMoveScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
            g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorBarLeft;
            break;
        default:
            hr = E_UNEXPECTED;
            break;
        }
        if (FAILED(hr))
        {
            return hr;
        }
        if (hCursorRegion)
        {
            g_sDemoState.arrScenes[iSceneIndex].hCursorRegion = hCursorRegion;
        }
    }
    return S_OK;
}

static VOID SelectScene(_In_ GuiTerminal::Control *lpGuiTerminal, _In_ INT iSceneIndex) noexcept
{
    GuiTerminal::RegionHandle hRegionCursor;
    g_sDemoState.iActiveScene = iSceneIndex;
    lpGuiTerminal->BringRegionToFront(g_sDemoState.arrScenes[iSceneIndex].hRegion);
    DrawButtons(lpGuiTerminal);
    lpGuiTerminal->SetCursorStyle(g_sDemoState.arrScenes[iSceneIndex].cursorStyle);
    hRegionCursor = g_sDemoState.arrScenes[iSceneIndex].hCursorRegion;
    lpGuiTerminal->ShowCursor(hRegionCursor ? hRegionCursor : g_sDemoState.arrScenes[iSceneIndex].hRegion);
}

static INT HitTestButton(_In_ INT iCol, _In_ INT iRow) noexcept
{
    INT iSceneIndex;
    INT iButtonX;
    if ((iCol < 0) || (iRow < 0))
    {
        return -1;
    }
    for (iSceneIndex = 0; iSceneIndex < DEMO_SCENE_COUNT; iSceneIndex++)
    {
        iButtonX = DEMO_BUTTON_X + (iSceneIndex * (DEMO_BUTTON_WIDTH + DEMO_BUTTON_GAP));
        if ((iCol >= iButtonX) && (iCol < (iButtonX + DEMO_BUTTON_WIDTH)) && (iRow >= DEMO_BUTTON_Y) &&
            (iRow < (DEMO_BUTTON_Y + DEMO_BUTTON_HEIGHT)))
        {
            return iSceneIndex;
        }
    }
    return -1;
}

static VOID GetButtonRect(_In_ INT iSceneIndex, _Out_ LPINT lpiX, _Out_ LPINT lpiY, _Out_ LPINT lpiWidth, _Out_ LPINT lpiHeight) noexcept
{
    *lpiX = DEMO_BUTTON_X + (iSceneIndex * (DEMO_BUTTON_WIDTH + DEMO_BUTTON_GAP));
    *lpiY = DEMO_BUTTON_Y;
    *lpiWidth = DEMO_BUTTON_WIDTH;
    *lpiHeight = DEMO_BUTTON_HEIGHT;
}

static VOID WriteMouseStatus(_In_ GuiTerminal::Control *lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW,
                             _In_ INT iX, _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept
{
    WCHAR szBufferW[256];
    swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]),
               L"\x1b[1;1H\x1b[100;97m Mouse events \x1b[0m %-6ls %-4ls col=%d row=%d px=(%d,%d)", szButtonNameW, szActionNameW, iCol, iRow,
               iX, iY);
    lpGuiTerminal->ClearRegion(g_sDemoState.hRegionStatus);
    lpGuiTerminal->WriteRegion(g_sDemoState.hRegionStatus, szBufferW);
}
