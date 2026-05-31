#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "..\include\GuiTerminalControl.h"

#define TERMINAL_COLS            160
#define TERMINAL_ROWS            25
#define TERMINAL_FONT_FAMILY     L"Cascadia Mono"
#define TERMINAL_FONT_SIZE       12.0f

#define WINDOW_CLASS_NAME        L"SampleGuiTerminalWindow"
#define WINDOW_TITLE             L"Sample GuiTerminal"

#define DEMO_SCENE_COUNT         4
#define DEMO_BUTTON_X            3
#define DEMO_BUTTON_Y            2
#define DEMO_BUTTON_WIDTH        36
#define DEMO_BUTTON_HEIGHT       2
#define DEMO_BUTTON_GAP          3
#define DEMO_SCENE_X             1
#define DEMO_SCENE_Y             5
#define DEMO_SCENE_WIDTH         (TERMINAL_COLS - 2)
#define DEMO_SCENE_HEIGHT        17
#define DEMO_STATUS_Y            24

// -----------------------------------------------------------------------------

enum DemoSceneId
{
    DemoSceneScroll = 0,
    DemoSceneBoxes,
    DemoSceneNested,
    DemoSceneMove
};

typedef struct DemoSceneDefinition
{
    LPCWSTR szButtonCaptionTopW;
    LPCWSTR szButtonCaptionBottomW;
    COLORREF crButtonForeground;
    COLORREF crButtonBackground;
} DemoSceneDefinition;

typedef struct DemoSceneState
{
    GuiTerminal::RegionHandle hRegion;
    GuiTerminal::RegionHandle hCursorRegion;
    GuiTerminal::Control::CursorStyle cursorStyle;
} DemoSceneState;

typedef struct DemoState
{
    GuiTerminal::Control* lpGuiTerminal;
    GuiTerminal::RegionHandle hRegionStatus;
    DemoSceneState arrScenes[DEMO_SCENE_COUNT];
    INT iActiveScene;
} DemoState;

// -----------------------------------------------------------------------------

static HRESULT EnablePerMonitorDpiAwareness() noexcept;

static HRESULT CreateMainWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_z_ LPCWSTR szWindowClassW,
                                _In_z_ LPCWSTR szTitleW, _Out_ HWND* lphWnd) noexcept;

static LRESULT CALLBACK MainWndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam,
                                    _In_ LPARAM lParam) noexcept;
static LRESULT HandleCreate(_In_ HWND hWnd) noexcept;
static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown, _In_z_ LPCWSTR szButtonNameW,
                                 _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept;

static HRESULT ResizeWindowToPreferredClientArea(_In_ HWND hWnd, _In_ GuiTerminal::Control* lpGuiTerminal) noexcept;

static HRESULT RunDemo(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static VOID DrawTopRowSamples(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static VOID DrawButtons(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static VOID DrawButton(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ INT iSceneIndex,
                       _In_ BOOL bActive) noexcept;
static HRESULT InitializeScenes(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static HRESULT InitializeScrollScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                     _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept;
static HRESULT InitializeBoxesScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                    _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept;
static HRESULT InitializeNestedScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                     _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept;
static HRESULT InitializeMoveScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                   _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept;
static VOID SelectScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ INT iSceneIndex) noexcept;
static INT HitTestButton(_In_ INT iCol, _In_ INT iRow) noexcept;
static VOID GetButtonRect(_In_ INT iSceneIndex, _Out_ LPINT lpiX, _Out_ LPINT lpiY,
                          _Out_ LPINT lpiWidth, _Out_ LPINT lpiHeight) noexcept;
static VOID WriteCenteredText(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ INT iCol, _In_ INT iRow, _In_ INT iWidth,
                              _In_z_ LPCWSTR szTextW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                              _In_ BOOL bBold) noexcept;
static VOID WriteMouseStatus(_In_ GuiTerminal::Control* lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW,
                             _In_ INT iX, _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept;

// -----------------------------------------------------------------------------

static const DemoSceneDefinition g_arrSceneDefinitions[DEMO_SCENE_COUNT] =
{
    { L"SCENE 1", L"SCROLL", RGB(16U, 28U, 40U), RGB(120U, 205U, 255U) },
    { L"SCENE 2", L"BOXES", RGB(48U, 24U, 0U), RGB(255U, 196U, 90U) },
    { L"SCENE 3", L"NESTED", RGB(18U, 18U, 18U), RGB(132U, 230U, 126U) },
    { L"SCENE 4", L"MOVE", RGB(255U, 255U, 255U), RGB(176U, 90U, 216U) }
};

static DemoState g_sDemoState{};

// -----------------------------------------------------------------------------

INT APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLineW, _In_ INT nCmdShow)
{
    INT iExitCode;
    HRESULT hr;
    HWND hWnd;
    MSG msg;

    iExitCode = 0;
    hr = S_OK;
    hWnd = {};
    msg = MSG{};
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLineW);

    hr = EnablePerMonitorDpiAwareness();
    if (FAILED(hr))
    {
        return static_cast<INT>(hr);
    }

    hr = CreateMainWindow(hInstance, nCmdShow, WINDOW_CLASS_NAME, WINDOW_TITLE, &hWnd);
    if (FAILED(hr))
    {
        return static_cast<INT>(hr);
    }

    while (GetMessageW(&msg, nullptr, 0U, 0U) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    iExitCode = static_cast<INT>(msg.wParam);
    return iExitCode;
}

static HRESULT EnablePerMonitorDpiAwareness() noexcept
{
    HRESULT hr;

    hr = S_OK;
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if ((hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) || (hr == E_ACCESSDENIED))
        {
            hr = S_OK;
        }
    }
    return hr;
}

static HRESULT CreateMainWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_z_ LPCWSTR szWindowClassW,
                                _In_z_ LPCWSTR szTitleW, _Out_ HWND* lphWnd) noexcept
{
    WNDCLASSEXW sWcExW;
    HWND hWnd;

    if (!lphWnd)
    {
        return E_POINTER;
    }
    *lphWnd = nullptr;

    memset(&sWcExW, 0, sizeof(sWcExW));
    sWcExW.cbSize = sizeof(sWcExW);
    sWcExW.style = CS_HREDRAW | CS_VREDRAW;
    sWcExW.lpfnWndProc = MainWndProc;
    sWcExW.cbClsExtra = 0;
    sWcExW.cbWndExtra = 0;
    sWcExW.hInstance = hInstance;
    sWcExW.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WINDOWSPROJECT1));
    sWcExW.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    sWcExW.hbrBackground = nullptr;
    sWcExW.lpszMenuName = nullptr;
    sWcExW.lpszClassName = szWindowClassW;
    sWcExW.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_SMALL));
    if (RegisterClassExW(&sWcExW) == FALSE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hWnd = CreateWindowExW(0, szWindowClassW, szTitleW, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                           nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    *lphWnd = hWnd;
    return S_OK;
}

static LRESULT CALLBACK MainWndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam,
                                    _In_ LPARAM lParam) noexcept
{
    LRESULT lResult;

    if (GuiTerminal::Control::WndProc(hWnd, uMessage, wParam, lParam, &lResult) != FALSE)
    {
        return lResult;
    }

    switch (uMessage)
    {
        case WM_CREATE:
            return HandleCreate(hWnd);
        case WM_LBUTTONDOWN:
            return HandleMouseButton(hWnd, TRUE, TRUE, L"Left", L"Down",
                                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_LBUTTONUP:
            return HandleMouseButton(hWnd, TRUE, FALSE, L"Left", L"Up",
                                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_RBUTTONDOWN:
            return HandleMouseButton(hWnd, FALSE, TRUE, L"Right", L"Down",
                                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_RBUTTONUP:
            return HandleMouseButton(hWnd, FALSE, FALSE, L"Right", L"Up",
                                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_MBUTTONDOWN:
            return HandleMouseButton(hWnd, FALSE, TRUE, L"Middle", L"Down",
                                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_MBUTTONUP:
            return HandleMouseButton(hWnd, FALSE, FALSE, L"Middle", L"Up",
                                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}

static LRESULT HandleCreate(_In_ HWND hWnd) noexcept
{
    GuiTerminal::Control* lpGuiTerminal;
    GuiTerminal::Control::Config configTerminal;
    HRESULT hr;

    lpGuiTerminal = nullptr;
    configTerminal = GuiTerminal::Control::Config{};
    configTerminal.iRows = TERMINAL_ROWS;
    configTerminal.iCols = TERMINAL_COLS;
    configTerminal.szFontFamilyW = TERMINAL_FONT_FAMILY;
    configTerminal.fFontSize = TERMINAL_FONT_SIZE;
    hr = GuiTerminal::Control::Create(hWnd, configTerminal, &lpGuiTerminal);
    if (FAILED(hr))
    {
        return -1;
    }
    hr = ResizeWindowToPreferredClientArea(hWnd, lpGuiTerminal);
    if (FAILED(hr))
    {
        return -1;
    }
    hr = RunDemo(lpGuiTerminal);
    if (FAILED(hr))
    {
        return -1;
    }
    return 0;
}

static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown, _In_z_ LPCWSTR szButtonNameW,
                                 _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept
{
    GuiTerminal::Control* lpGuiTerminal;
    INT iCol;
    INT iRow;
    INT iSceneHit;

    lpGuiTerminal = GuiTerminal::Control::GetControl(hWnd);
    if (!lpGuiTerminal)
    {
        return 0;
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
    InvalidateRect(hWnd, nullptr, FALSE);
    return 0;
}

static HRESULT ResizeWindowToPreferredClientArea(_In_ HWND hWnd, _In_ GuiTerminal::Control* lpGuiTerminal) noexcept
{
    SIZE sizeClient;
    RECT rcWindow;
    HRESULT hr;

    hr = lpGuiTerminal->GetPreferredWindowSize(&sizeClient);
    if (FAILED(hr))
    {
        return hr;
    }

    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = sizeClient.cx;
    rcWindow.bottom = sizeClient.cy;

    if (SetWindowPos(hWnd, nullptr, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) == FALSE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    return S_OK;
}

static HRESULT RunDemo(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept
{
    HRESULT hr;

    if (!lpGuiTerminal)
    {
        return E_POINTER;
    }

    lpGuiTerminal->Clear();
    g_sDemoState = DemoState{};
    g_sDemoState.lpGuiTerminal = lpGuiTerminal;
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

static VOID DrawTopRowSamples(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept
{
    if (!lpGuiTerminal)
    {
        return;
    }

    lpGuiTerminal->Write(L"\x1b[1;1H"
                         L"GuiTerminal demo: "
                         L"\x1b[38;2;255;170;40mTruecolor FG\x1b[0m  "
                         L"\x1b[48;2;0;96;160m\x1b[97mTruecolor BG\x1b[0m  "
                         L"\x1b[4munderline\x1b[24m  "
                         L"\x1b[1mbold\x1b[22m  "
                         L"\x1b[5mblink\x1b[25m  "
                         L"\x1b[32mgreen\x1b[0m  "
                         L"\x1b[33myellow\x1b[0m  "
                         L"\x1b[34mblue\x1b[0m  "
                         L"\x1b[91mbright red\x1b[0m  "
                         L"\x1b[38;5;141m256-color\x1b[0m");
}

static VOID DrawButtons(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept
{
    INT iSceneIndex;

    if (!lpGuiTerminal)
    {
        return;
    }

    for (iSceneIndex = 0; iSceneIndex < DEMO_SCENE_COUNT; iSceneIndex++)
    {
        DrawButton(lpGuiTerminal, iSceneIndex, (g_sDemoState.iActiveScene == iSceneIndex) ? TRUE : FALSE);
    }
}

static VOID DrawButton(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ INT iSceneIndex, _In_ BOOL bActive) noexcept
{
    const DemoSceneDefinition* lpsDefinition;
    COLORREF crBackground;
    COLORREF crForeground;
    INT iX;
    INT iY;
    INT iWidth;
    INT iHeight;

    if ((!lpGuiTerminal) || (iSceneIndex < 0) || (iSceneIndex >= DEMO_SCENE_COUNT))
    {
        return;
    }

    lpsDefinition = &g_arrSceneDefinitions[iSceneIndex];
    GetButtonRect(iSceneIndex, &iX, &iY, &iWidth, &iHeight);
    crBackground = (bActive != FALSE) ? lpsDefinition->crButtonBackground :
                                        RGB(GetRValue(lpsDefinition->crButtonBackground) / 2U,
                                            GetGValue(lpsDefinition->crButtonBackground) / 2U,
                                            GetBValue(lpsDefinition->crButtonBackground) / 2U);
    crForeground = (bActive != FALSE) ? RGB(255U, 255U, 255U) : lpsDefinition->crButtonForeground;

    lpGuiTerminal->FillArea(iX, iY, iWidth, iHeight, L' ', crForeground, crBackground,
                            (bActive != FALSE) ? GuiTerminal::Control::StyleBold : GuiTerminal::Control::StyleNone);
    WriteCenteredText(lpGuiTerminal, iX, iY, iWidth, lpsDefinition->szButtonCaptionTopW,
                      crForeground, crBackground, bActive);
    WriteCenteredText(lpGuiTerminal, iX, iY + 1, iWidth, lpsDefinition->szButtonCaptionBottomW,
                      crForeground, crBackground, bActive);
}

static HRESULT InitializeScenes(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept
{
    GuiTerminal::RegionHandle hRegionScene;
    GuiTerminal::RegionHandle hCursorRegion;
    HRESULT hr;
    INT iSceneIndex;

    if (!lpGuiTerminal)
    {
        return E_POINTER;
    }

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
        g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorUnderscore;

        switch (iSceneIndex)
        {
            case DemoSceneScroll:
                hr = InitializeScrollScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
                g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorUnderscore;
                break;
            case DemoSceneBoxes:
                hr = InitializeBoxesScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
                g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorBarLeft;
                break;
            case DemoSceneNested:
                hr = InitializeNestedScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
                g_sDemoState.arrScenes[iSceneIndex].cursorStyle = GuiTerminal::Control::CursorBlock;
                break;
            case DemoSceneMove:
                hr = InitializeMoveScene(lpGuiTerminal, hRegionScene, &hCursorRegion);
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

static HRESULT InitializeScrollScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                     _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept
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
    lpGuiTerminal->FillRegionArea(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(225U, 240U, 255U), RGB(0U, 24U, 52U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, GuiTerminal::Control::BoxSideLeftDouble,
                                 RGB(150U, 220U, 255U), RGB(0U, 24U, 52U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[1;4H\x1b[48;2;0;24;52m\x1b[1;97m Scroll scene \x1b[0m"
                               L"\x1b[2;3HWriteRegion keeps the newest lines visible inside a smaller child region."
                               L"\x1b[3;3HThe pane below receives more lines than it can display.");

    hr = lpGuiTerminal->CreateRegion(3, 4, 70, 10, &hRegionLog, hRegionScene);
    if (FAILED(hr))
    {
        return hr;
    }

    lpGuiTerminal->FillRegionArea(hRegionLog, 0, 0, 70, 10, L' ', RGB(220U, 220U, 220U), RGB(8U, 12U, 20U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionLog, 0, 0, 70, 10,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideRightDouble,
                                 RGB(180U, 220U, 255U), RGB(8U, 12U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionLog, L"\x1b[1;3H\x1b[38;5;117mBuffered output\x1b[0m");

    for (iLine = 1; iLine <= 18; iLine++)
    {
        swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]),
                   L"\x1b[38;5;%dmline %02d  scroll sample 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n",
                   80 + (iLine % 8), iLine);
        lpGuiTerminal->WriteRegion(hRegionLog, szBufferW);
    }

    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[5;80H\x1b[38;5;117mWhat to inspect\x1b[0m"
                               L"\x1b[7;80H- child-region clipping"
                               L"\x1b[8;80H- retained background colors"
                               L"\x1b[9;80H- line wrapping in a bounded region"
                               L"\x1b[10;80H- cursor targeting per scene");

    *lphCursorRegion = hRegionLog;
    return S_OK;
}

static HRESULT InitializeBoxesScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                    _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept
{
    if ((!lpGuiTerminal) || (!hRegionScene) || (!lphCursorRegion))
    {
        return E_POINTER;
    }

    *lphCursorRegion = hRegionScene;
    lpGuiTerminal->FillRegionArea(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(35U, 26U, 0U), RGB(48U, 34U, 0U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble,
                                 RGB(255U, 210U, 120U), RGB(48U, 34U, 0U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[1;4H\x1b[48;2;48;34;0m\x1b[1;97m Boxes and crossing lines \x1b[0m"
                               L"\x1b[2;3HGlobal-style drawing helpers also work inside a full overlay region.");

    lpGuiTerminal->DrawRegionBox(hRegionScene, 4, 4, 42, 9,
                                 GuiTerminal::Control::BoxSideLeftDouble | GuiTerminal::Control::BoxSideTopDouble,
                                 RGB(255U, 220U, 150U), RGB(72U, 48U, 0U), GuiTerminal::Control::StyleNone);
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
    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[6;8Hsingle + double joins"
                               L"\x1b[10;10Hcrossing strokes"
                               L"\x1b[5;84Hlight shade"
                               L"\x1b[7;84Hmedium shade"
                               L"\x1b[9;84Hdark shade"
                               L"\x1b[11;84Hsolid block"
                               L"\x1b[13;84Hmixed line families keep intersections");
    return S_OK;
}

static HRESULT InitializeNestedScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                     _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept
{
    GuiTerminal::RegionHandle hRegionParent;
    GuiTerminal::RegionHandle hRegionBack;
    GuiTerminal::RegionHandle hRegionFront;
    GuiTerminal::RegionHandle hRegionBadge;
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
    lpGuiTerminal->FillRegionArea(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(18U, 18U, 18U), RGB(16U, 44U, 20U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, GuiTerminal::Control::BoxSideRightDouble,
                                 RGB(132U, 230U, 126U), RGB(16U, 44U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[1;4H\x1b[48;2;16;44;20m\x1b[1;97m Nested regions and local cursors \x1b[0m"
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

    lpGuiTerminal->FillRegionArea(hRegionParent, 0, 0, 54, 10, L' ', RGB(225U, 245U, 225U), RGB(24U, 62U, 28U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionParent, 0, 0, 54, 10,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideLeftDouble,
                                 RGB(225U, 255U, 225U), RGB(24U, 62U, 28U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionParent, L"\x1b[2;3HParent region");

    lpGuiTerminal->FillRegionArea(hRegionBack, 0, 0, 24, 6, L' ', RGB(20U, 20U, 20U), RGB(220U, 226U, 228U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionBack, 0, 0, 24, 6, GuiTerminal::Control::BoxSideBottomDouble, RGB(20U, 20U, 20U),
                                 RGB(220U, 226U, 228U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionBack, L"\x1b[2;3Hback child\r\n\x1b[4;3Hcursor target A");

    lpGuiTerminal->FillRegionArea(hRegionFront, 0, 0, 24, 5, L' ', RGB(30U, 20U, 0U), RGB(255U, 228U, 130U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionFront, 0, 0, 24, 5,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideRightDouble,
                                 RGB(50U, 32U, 0U), RGB(255U, 228U, 130U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionFront, L"\x1b[2;3Hfront child\r\n\x1b[4;3Hcursor target B");

    lpGuiTerminal->FillRegionArea(hRegionBadge, 0, 0, 12, 3, L' ', RGB(255U, 255U, 255U), RGB(180U, 56U, 56U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionBadge, 0, 0, 12, 3,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble,
                                 RGB(255U, 255U, 255U), RGB(180U, 56U, 56U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionBadge, L"\x1b[2;3Hbadge");

    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[6;72HVisible layering:"
                               L"\x1b[8;72H- parent host"
                               L"\x1b[9;72H- back child"
                               L"\x1b[10;72H- front child"
                               L"\x1b[11;72H- grandchild badge");

    *lphCursorRegion = hRegionBadge;
    return S_OK;
}

static HRESULT InitializeMoveScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegionScene,
                                   _Out_ GuiTerminal::RegionHandle* lphCursorRegion) noexcept
{
    GuiTerminal::RegionHandle hRegionPanel;
    HRESULT hr;

    if ((!lpGuiTerminal) || (!hRegionScene) || (!lphCursorRegion))
    {
        return E_POINTER;
    }

    *lphCursorRegion = hRegionScene;
    hRegionPanel = nullptr;
    lpGuiTerminal->FillRegionArea(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT, L' ', RGB(245U, 235U, 255U), RGB(52U, 18U, 72U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionScene, 0, 0, DEMO_SCENE_WIDTH, DEMO_SCENE_HEIGHT,
                                 GuiTerminal::Control::BoxSideLeftDouble | GuiTerminal::Control::BoxSideRightDouble,
                                 RGB(220U, 180U, 255U), RGB(52U, 18U, 72U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene,
                               L"\x1b[1;4H\x1b[48;2;52;18;72m\x1b[1;97m Move and MoveRegion \x1b[0m"
                               L"\x1b[2;3HThis scene includes in-bounds and clipped moves so vacated cells stay obvious.");

    lpGuiTerminal->FillRegionArea(hRegionScene, 6, 4, 36, 3, L'.', RGB(220U, 200U, 255U), RGB(76U, 28U, 102U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[5;9HSOURCE CELLS");
    lpGuiTerminal->MoveRegion(hRegionScene, 6, 4, 14, 1, 26, 6, L'=', RGB(255U, 240U, 180U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[8;9HMoveRegion() leaves '=' behind");
    lpGuiTerminal->FillRegionArea(hRegionScene, 124, 4, 24, 3, L'~', RGB(255U, 232U, 255U), RGB(112U, 36U, 140U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[5;127HCLIP RIGHT");
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[6;126Hsource >>>>>>>>");
    lpGuiTerminal->MoveRegion(hRegionScene, 124, 4, 24, 3, 144, 5, L'!', RGB(255U, 210U, 160U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->FillRegionArea(hRegionScene, 4, 12, 20, 2, L'%', RGB(255U, 245U, 200U), RGB(94U, 38U, 120U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[13;6HCLIP LEFT");
    lpGuiTerminal->WriteRegion(hRegionScene, L"\x1b[14;6H<<<<<< source");
    lpGuiTerminal->MoveRegion(hRegionScene, 4, 12, 20, 2, -5, 13, L'?', RGB(255U, 220U, 180U), RGB(52U, 18U, 72U),
                              GuiTerminal::Control::StyleBold);
    lpGuiTerminal->FillRegionArea(hRegionScene, 52, 11, 18, 4, L'@', RGB(255U, 240U, 220U), RGB(104U, 46U, 130U),
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

    lpGuiTerminal->FillRegionArea(hRegionPanel, 0, 0, 48, 6, L'+', RGB(24U, 24U, 24U), RGB(214U, 184U, 238U),
                                  GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionBox(hRegionPanel, 0, 0, 48, 6,
                                 GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble,
                                 RGB(40U, 20U, 60U), RGB(214U, 184U, 238U), GuiTerminal::Control::StyleNone);
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

static VOID SelectScene(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ INT iSceneIndex) noexcept
{
    GuiTerminal::RegionHandle hRegionCursor;

    if ((!lpGuiTerminal) || (iSceneIndex < 0) || (iSceneIndex >= DEMO_SCENE_COUNT))
    {
        return;
    }

    g_sDemoState.iActiveScene = iSceneIndex;
    lpGuiTerminal->BringRegionToFront(g_sDemoState.arrScenes[iSceneIndex].hRegion);
    DrawButtons(lpGuiTerminal);
    lpGuiTerminal->SetCursorStyle(g_sDemoState.arrScenes[iSceneIndex].cursorStyle);
    hRegionCursor = g_sDemoState.arrScenes[iSceneIndex].hCursorRegion;
    lpGuiTerminal->ShowCursor(hRegionCursor ? hRegionCursor : g_sDemoState.arrScenes[iSceneIndex].hRegion);
}

static INT HitTestButton(_In_ INT iCol, _In_ INT iRow) noexcept
{
    INT iButtonX;
    INT iButtonY;
    INT iButtonWidth;
    INT iButtonHeight;
    INT iSceneIndex;

    if (iCol < 0 || iRow < 0)
    {
        return -1;
    }

    for (iSceneIndex = 0; iSceneIndex < DEMO_SCENE_COUNT; iSceneIndex++)
    {
        GetButtonRect(iSceneIndex, &iButtonX, &iButtonY, &iButtonWidth, &iButtonHeight);
        if ((iCol >= iButtonX) && (iCol < (iButtonX + iButtonWidth)) &&
            (iRow >= iButtonY) && (iRow < (iButtonY + iButtonHeight)))
        {
            return iSceneIndex;
        }
    }
    return -1;
}

static VOID GetButtonRect(_In_ INT iSceneIndex, _Out_ LPINT lpiX, _Out_ LPINT lpiY,
                          _Out_ LPINT lpiWidth, _Out_ LPINT lpiHeight) noexcept
{
    if (lpiX)
    {
        *lpiX = DEMO_BUTTON_X + (iSceneIndex * (DEMO_BUTTON_WIDTH + DEMO_BUTTON_GAP));
    }
    if (lpiY)
    {
        *lpiY = DEMO_BUTTON_Y;
    }
    if (lpiWidth)
    {
        *lpiWidth = DEMO_BUTTON_WIDTH;
    }
    if (lpiHeight)
    {
        *lpiHeight = DEMO_BUTTON_HEIGHT;
    }
}

static VOID WriteCenteredText(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ INT iCol, _In_ INT iRow, _In_ INT iWidth,
                              _In_z_ LPCWSTR szTextW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                              _In_ BOOL bBold) noexcept
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

    lpGuiTerminal->Print(L"\x1b[%d;%dH\x1b[38;2;%u;%u;%um\x1b[48;2;%u;%u;%um%ls%ls\x1b[0m",
                         iRow + 1, iStartCol + 1,
                         static_cast<unsigned>(GetRValue(crForeground)), static_cast<unsigned>(GetGValue(crForeground)),
                         static_cast<unsigned>(GetBValue(crForeground)),
                         static_cast<unsigned>(GetRValue(crBackground)), static_cast<unsigned>(GetGValue(crBackground)),
                         static_cast<unsigned>(GetBValue(crBackground)),
                         (bBold != FALSE) ? L"\x1b[1m" : L"", szTextW);
}

static VOID WriteMouseStatus(_In_ GuiTerminal::Control* lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW,
                             _In_ INT iX, _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept
{
    WCHAR szBufferW[256];

    if ((!lpGuiTerminal) || (!g_sDemoState.hRegionStatus))
    {
        return;
    }

    swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]),
               L"\x1b[1;1H\x1b[100;97m Mouse events \x1b[0m %-6ls %-4ls col=%d row=%d px=(%d,%d)",
               szButtonNameW, szActionNameW, iCol, iRow, iX, iY);

    lpGuiTerminal->ClearRegion(g_sDemoState.hRegionStatus);
    lpGuiTerminal->WriteRegion(g_sDemoState.hRegionStatus, szBufferW);
}
