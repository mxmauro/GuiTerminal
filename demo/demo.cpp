#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "..\include\GuiTerminalControl.h"

#define TERMINAL_COLS        160
#define TERMINAL_ROWS        25
#define TERMINAL_FONT_FAMILY L"Cascadia Mono"
#define TERMINAL_FONT_SIZE   12.0f
#define DEMO_TIMER_ID        2U

#define WINDOW_CLASS_NAME    L"SampleGuiTerminalWindow"
#define WINDOW_TITLE         L"Sample GuiTerminal"

// -----------------------------------------------------------------------------

typedef struct DemoRegionMotion
{
    GuiTerminal::RegionHandle hRegion;
    INT iX;
    INT iY;
    INT iWidth;
    INT iHeight;
    INT iMinX;
    INT iMaxX;
    INT iMinY;
    INT iMaxY;
    INT iDeltaX;
    INT iDeltaY;
} DemoRegionMotion;

typedef struct DemoState
{
    GuiTerminal::Control* lpGuiTerminal;
    GuiTerminal::RegionHandle hRegionStatus;
    DemoRegionMotion sMotionBack;
    DemoRegionMotion sMotionFront;
    DemoRegionMotion sMotionBanner;
    DemoRegionMotion sMotionOffscreen;
    GuiTerminal::RegionHandle rghZOrder[4];
    INT iZOrderCount;
} DemoState;

typedef struct DemoMouseCallbackContext
{
    GuiTerminal::RegionHandle hRegionStatus;
} DemoMouseCallbackContext;

static HRESULT EnablePerMonitorDpiAwareness() noexcept;

static HRESULT CreateMainWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_z_ LPCWSTR szWindowClassW, _In_z_ LPCWSTR szTitleW,
                                _Out_ HWND* lphWnd) noexcept;

static LRESULT CALLBACK MainWndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam) noexcept;
static LRESULT HandleCreate(_In_ HWND hWnd) noexcept;
static LRESULT HandleDemoTimer(_In_ HWND hWnd) noexcept;
static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown, _In_ LPCWSTR szButtonNameW,
                                 _In_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept;

static HRESULT ResizeWindowToPreferredClientArea(_In_ HWND hWnd, _In_ GuiTerminal::Control* lpGuiTerminal) noexcept;

static HRESULT RunDemo(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static VOID DrawRegionFrames(_In_ GuiTerminal::Control* lpGuiTerminal, _In_opt_ GuiTerminal::RegionHandle hRegionStatus,
                             _In_opt_ GuiTerminal::RegionHandle hRegionBack, _In_opt_ GuiTerminal::RegionHandle hRegionFront,
                             _In_opt_ GuiTerminal::RegionHandle hRegionBanner,
                             _In_opt_ GuiTerminal::RegionHandle hRegionOffscreen) noexcept;
static VOID UpdateRegionMotion(_Inout_ DemoRegionMotion* lpsMotion) noexcept;
static VOID BringDemoRegionToFront(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegion) noexcept;
static GuiTerminal::RegionHandle HitTestDemoRegion(_In_ INT iCol, _In_ INT iRow) noexcept;
static BOOL IsCellInsideMotion(_In_ const DemoRegionMotion* lpsMotion, _In_ INT iCol, _In_ INT iRow) noexcept;
static VOID WriteMouseStatus(_In_ GuiTerminal::Control* lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW,
                             _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept;

// -----------------------------------------------------------------------------

static DemoMouseCallbackContext g_sDemoMouseCallbackContext{};
static DemoState g_sDemoState{};

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

static HRESULT CreateMainWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_z_ LPCWSTR szWindowClassW, _In_z_ LPCWSTR szTitleW,
                                _Out_ HWND* lphWnd) noexcept
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

static LRESULT CALLBACK MainWndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam) noexcept
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
            return HandleMouseButton(hWnd, TRUE, TRUE, L"Left", L"Down", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_LBUTTONUP:
            return HandleMouseButton(hWnd, TRUE, FALSE, L"Left", L"Up", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_RBUTTONDOWN:
            return HandleMouseButton(hWnd, FALSE, TRUE, L"Right", L"Down", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_RBUTTONUP:
            return HandleMouseButton(hWnd, FALSE, FALSE, L"Right", L"Up", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_MBUTTONDOWN:
            return HandleMouseButton(hWnd, FALSE, TRUE, L"Middle", L"Down", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_MBUTTONUP:
            return HandleMouseButton(hWnd, FALSE, FALSE, L"Middle", L"Up", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_TIMER:
            if (static_cast<UINT_PTR>(wParam) == DEMO_TIMER_ID)
            {
                return HandleDemoTimer(hWnd);
            }
            break;
        case WM_DESTROY:
            KillTimer(hWnd, DEMO_TIMER_ID);
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
    if (SetTimer(hWnd, DEMO_TIMER_ID, 1000U, nullptr) == 0U)
    {
        return -1;
    }
    return 0;
}

static LRESULT HandleDemoTimer(_In_ HWND hWnd) noexcept
{
    UpdateRegionMotion(&g_sDemoState.sMotionBack);
    UpdateRegionMotion(&g_sDemoState.sMotionFront);
    UpdateRegionMotion(&g_sDemoState.sMotionBanner);
    UpdateRegionMotion(&g_sDemoState.sMotionOffscreen);
    InvalidateRect(hWnd, nullptr, FALSE);
    return 0;
}

static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown, _In_ LPCWSTR szButtonNameW,
                                 _In_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept
{
    GuiTerminal::Control* lpGuiTerminal;
    GuiTerminal::RegionHandle hRegionHit;
    INT iCol;
    INT iRow;

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
        hRegionHit = HitTestDemoRegion(iCol, iRow);
        if (hRegionHit)
        {
            BringDemoRegionToFront(lpGuiTerminal, hRegionHit);
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
    GuiTerminal::RegionHandle hRegionStatus;
    GuiTerminal::RegionHandle hRegionBack;
    GuiTerminal::RegionHandle hRegionFront;
    GuiTerminal::RegionHandle hRegionBanner;
    GuiTerminal::RegionHandle hRegionOffscreen;
    WCHAR szBufferW[256];
    INT iIndex;
    HRESULT hr;

    hRegionStatus = nullptr;
    lpGuiTerminal->Clear();
    g_sDemoState = DemoState{};
    g_sDemoState.lpGuiTerminal = lpGuiTerminal;

    lpGuiTerminal->Write(L"\x1b[1;97mWWin32 + Direct2D/DirectWrite terminal demo\x1b[0m\r\n"
                         L"\x1b[38;2;255;170;40mTruecolor foreground\x1b[0m  "
                         L"\x1b[48;2;0;96;160;97mtruecolor background\x1b[0m  "
                         L"\x1b[4munderline\x1b[24m  "
                         L"\x1b[1mbold\x1b[22m  "
                         L"\x1b[5mblink\x1b[25m\r\n"
                         L"\x1b[32mGreen\x1b[0m "
                          L"\x1b[33mYellow\x1b[0m "
                          L"\x1b[34mBlue\x1b[0m "
                          L"\x1b[91mBright red\x1b[0m "
                          L"\x1b[38;5;141m256-color\x1b[0m\r\n"
                          L"\x1b[s\x1b[6;5HPositioned at row 6 col 5\x1b[u\r\n"
                          L"\x1b[5;1H\x1b[2KLeft: overlapping regions and region borders. "
                          L"Right: global lines, rectangles, clipping, and out-of-bounds draws.");

    hr = lpGuiTerminal->CreateRegion(0, 23, TERMINAL_COLS, 2, &hRegionStatus);
    if (FAILED(hr))
    {
        return hr;
    }
    g_sDemoMouseCallbackContext.hRegionStatus = hRegionStatus;
    g_sDemoState.hRegionStatus = hRegionStatus;
    lpGuiTerminal->WriteRegion(hRegionStatus, L"\x1b[100;97m Mouse events \x1b[0m\r\n"
                                              L"Left/Right/Middle up/down. Outside cells reports col=-1 row=-1.");

    hr = lpGuiTerminal->CreateRegion(4, 7, 34, 13, &hRegionBack);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(22, 9, 34, 13, &hRegionFront);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(-6, 6, 26, 4, &hRegionBanner);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(154, 10, 12, 5, &hRegionOffscreen);
    if (FAILED(hr))
    {
        return hr;
    }

    g_sDemoState.sMotionBack = DemoRegionMotion{ hRegionBack, 4, 7, 34, 13, 2, 24, 7, 7, 1, 0 };
    g_sDemoState.sMotionFront = DemoRegionMotion{ hRegionFront, 22, 9, 34, 13, 22, 42, 9, 9, -1, 0 };
    g_sDemoState.sMotionBanner = DemoRegionMotion{ hRegionBanner, -6, 6, 26, 4, -6, -6, 6, 12, 0, 1 };
    g_sDemoState.sMotionOffscreen = DemoRegionMotion{ hRegionOffscreen, 154, 10, 12, 5, 132, 154, 10, 18, -1, 1 };

    lpGuiTerminal->WriteRegion(hRegionBack, L"\x1b[44;97m Back region \x1b[0m\r\n");
    lpGuiTerminal->WriteRegion(hRegionFront, L"\x1b[42;30m Front region \x1b[0m\r\n");
    lpGuiTerminal->WriteRegion(hRegionBanner, L"\x1b[42;97m OFFSCREEN BANNER \x1b[0m\r\n"
                                               L"\x1b[42;97m clipped on left \x1b[0m");
    lpGuiTerminal->WriteRegion(hRegionOffscreen, L"\x1b[45;97m edge region \x1b[0m\r\nmoves in/out\r\nof bounds");
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionBack, -3, 4, 40, GuiTerminal::Control::StrokeSingleLine, RGB(255U, 220U, 160U),
                                            RGB(0U, 20U, 70U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionVerticalLine(hRegionBack, 16, -2, 18, GuiTerminal::Control::StrokeDoubleLine, RGB(255U, 220U, 160U),
                                          RGB(0U, 20U, 70U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionBack, 2, 10, 20, GuiTerminal::Control::StrokeShadeMedium, RGB(190U, 190U, 190U),
                                            RGB(0U, 20U, 70U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionVerticalLine(hRegionFront, 10, -2, 18, GuiTerminal::Control::StrokeSolidBlock, RGB(70U, 40U, 40U),
                                          RGB(120U, 200U, 120U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawRegionHorizontalLine(hRegionFront, 0, 6, 34, GuiTerminal::Control::StrokeShadeDark, RGB(50U, 80U, 50U),
                                            RGB(120U, 200U, 120U), GuiTerminal::Control::StyleNone);

    for (iIndex = 1; iIndex <= 14; iIndex++)
    {
        swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L" \x1b[38;5;81mback\x1b[0m line %d 0123456789\r\n", iIndex);
        lpGuiTerminal->WriteRegion(hRegionBack, szBufferW);
    }

    lpGuiTerminal->WriteRegion(hRegionFront,
                               L"\x1b[30;102m opaque spaces demo \x1b[0m\r\n"
                               L"\x1b[30;102m                \x1b[0m\r\n"
                               L"\x1b[30;102m top region     \x1b[0m\r\n");
    for (iIndex = 1; iIndex <= 10; iIndex++)
    {
        swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L" \x1b[38;2;255;120;120mfront\x1b[0m line %d\r\n", iIndex);
        lpGuiTerminal->WriteRegion(hRegionFront, szBufferW);
    }

    lpGuiTerminal->Write(L"\x1b[7;86H\x1b[1;97mGlobal drawing helpers\x1b[0m"
                         L"\x1b[8;86Hsingle/double joins, shading and clipping");
    lpGuiTerminal->DrawBox(85, 9, 66, 12, GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideLeftDouble,
                           RGB(220U, 220U, 220U), RGB(15U, 15U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawHorizontalLine(82, 12, 74, GuiTerminal::Control::StrokeSingleLine, RGB(255U, 210U, 150U), RGB(15U, 15U, 20U),
                                      GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawVerticalLine(101, 8, 16, GuiTerminal::Control::StrokeDoubleLine, RGB(150U, 220U, 255U), RGB(15U, 15U, 20U),
                                    GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawHorizontalLine(90, 15, 28, GuiTerminal::Control::StrokeShadeLight, RGB(180U, 180U, 180U), RGB(15U, 15U, 20U),
                                      GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawHorizontalLine(90, 16, 28, GuiTerminal::Control::StrokeShadeMedium, RGB(200U, 200U, 200U), RGB(15U, 15U, 20U),
                                      GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawHorizontalLine(90, 17, 28, GuiTerminal::Control::StrokeShadeDark, RGB(220U, 220U, 220U), RGB(15U, 15U, 20U),
                                      GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawHorizontalLine(90, 18, 28, GuiTerminal::Control::StrokeSolidBlock, RGB(255U, 255U, 255U), RGB(15U, 15U, 20U),
                                      GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawBox(146, 10, 22, 7,
                           GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideRightDouble |
                               GuiTerminal::Control::BoxSideBottomDouble,
                           RGB(255U, 180U, 120U), RGB(15U, 15U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawHorizontalLine(150, 20, 24, GuiTerminal::Control::StrokeDoubleLine, RGB(255U, 200U, 160U), RGB(15U, 15U, 20U),
                                      GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawVerticalLine(158, 18, 10, GuiTerminal::Control::StrokeSingleLine, RGB(190U, 210U, 255U), RGB(15U, 15U, 20U),
                                    GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawBox(120, 20, 0, 4, GuiTerminal::Control::BoxSideLeftDouble | GuiTerminal::Control::BoxSideRightDouble,
                           RGB(150U, 255U, 180U), RGB(15U, 15U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->DrawBox(124, 20, 10, 0, GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble,
                           RGB(150U, 255U, 180U), RGB(15U, 15U, 20U), GuiTerminal::Control::StyleNone);
    lpGuiTerminal->Write(L"\x1b[11;88Hbox clipped on right"
                         L"\x1b[13;88Hsingle + double joins"
                         L"\x1b[19;88Hshade strokes overwrite"
                         L"\x1b[21;126Hzero width -> vertical"
                         L"\x1b[22;126Hzero height -> horizontal");

    lpGuiTerminal->SendRegionToBack(hRegionBack);
    lpGuiTerminal->BringRegionToFront(hRegionFront);
    lpGuiTerminal->BringRegionToFront(hRegionBanner);
    BringDemoRegionToFront(lpGuiTerminal, hRegionFront);
    BringDemoRegionToFront(lpGuiTerminal, hRegionBanner);
    g_sDemoState.rghZOrder[0] = hRegionBack;
    g_sDemoState.rghZOrder[1] = hRegionOffscreen;
    g_sDemoState.rghZOrder[2] = hRegionFront;
    g_sDemoState.rghZOrder[3] = hRegionBanner;
    g_sDemoState.iZOrderCount = 4;
    DrawRegionFrames(lpGuiTerminal, hRegionStatus, hRegionBack, hRegionFront, hRegionBanner, hRegionOffscreen);

    return S_OK;
}

static VOID DrawRegionFrames(_In_ GuiTerminal::Control* lpGuiTerminal, _In_opt_ GuiTerminal::RegionHandle hRegionStatus,
                             _In_opt_ GuiTerminal::RegionHandle hRegionBack, _In_opt_ GuiTerminal::RegionHandle hRegionFront,
                             _In_opt_ GuiTerminal::RegionHandle hRegionBanner, _In_opt_ GuiTerminal::RegionHandle hRegionOffscreen) noexcept
{
    if (!lpGuiTerminal)
    {
        return;
    }
    UNREFERENCED_PARAMETER(hRegionStatus);

    if (hRegionBack)
    {
        lpGuiTerminal->DrawRegionBox(hRegionBack, 0, 0, 34, 13, GuiTerminal::Control::BoxSideNone, RGB(180U, 220U, 255U), RGB(0U, 20U, 70U),
                                     GuiTerminal::Control::StyleNone);
    }
    if (hRegionFront)
    {
        lpGuiTerminal->DrawRegionBox(hRegionFront, 0, 0, 34, 13,
                                     GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideRightDouble,
                                     RGB(20U, 20U, 20U), RGB(120U, 200U, 120U), GuiTerminal::Control::StyleNone);
    }
    if (hRegionBanner)
    {
        lpGuiTerminal->DrawRegionBox(hRegionBanner, 0, 0, 26, 4,
                                     GuiTerminal::Control::BoxSideTopDouble | GuiTerminal::Control::BoxSideBottomDouble,
                                     RGB(255U, 255U, 255U), RGB(0U, 140U, 0U), GuiTerminal::Control::StyleNone);
    }
    if (hRegionOffscreen)
    {
        lpGuiTerminal->DrawRegionBox(hRegionOffscreen, 0, 0, 12, 5,
                                     GuiTerminal::Control::BoxSideLeftDouble | GuiTerminal::Control::BoxSideRightDouble,
                                     RGB(255U, 255U, 255U), RGB(90U, 0U, 90U), GuiTerminal::Control::StyleNone);
    }
}

static VOID UpdateRegionMotion(_Inout_ DemoRegionMotion* lpsMotion) noexcept
{
    HRESULT hr;

    if (!lpsMotion || !g_sDemoState.lpGuiTerminal || !lpsMotion->hRegion)
    {
        return;
    }

    if (lpsMotion->iDeltaX != 0)
    {
        if ((lpsMotion->iX + lpsMotion->iDeltaX) < lpsMotion->iMinX || (lpsMotion->iX + lpsMotion->iDeltaX) > lpsMotion->iMaxX)
        {
            lpsMotion->iDeltaX = -lpsMotion->iDeltaX;
        }
    }
    if (lpsMotion->iDeltaY != 0)
    {
        if (((lpsMotion->iY + lpsMotion->iDeltaY) < lpsMotion->iMinY) ||
            ((lpsMotion->iY + lpsMotion->iDeltaY) > lpsMotion->iMaxY))
        {
            lpsMotion->iDeltaY = -lpsMotion->iDeltaY;
        }
    }

    lpsMotion->iX += lpsMotion->iDeltaX;
    lpsMotion->iY += lpsMotion->iDeltaY;
    hr = g_sDemoState.lpGuiTerminal->RelocateRegion(
        lpsMotion->hRegion,
        lpsMotion->iX,
        lpsMotion->iY,
        lpsMotion->iWidth,
        lpsMotion->iHeight);
    if (FAILED(hr))
    {
        lpsMotion->iX -= lpsMotion->iDeltaX;
        lpsMotion->iY -= lpsMotion->iDeltaY;
        lpsMotion->iDeltaX = -lpsMotion->iDeltaX;
        lpsMotion->iDeltaY = -lpsMotion->iDeltaY;
    }
}

static VOID BringDemoRegionToFront(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::RegionHandle hRegion) noexcept
{
    INT iIndex;
    INT iFoundIndex;

    if (!lpGuiTerminal || !hRegion)
    {
        return;
    }

    lpGuiTerminal->BringRegionToFront(hRegion);
    iFoundIndex = -1;
    for (iIndex = 0; iIndex < g_sDemoState.iZOrderCount; ++iIndex)
    {
        if (g_sDemoState.rghZOrder[iIndex] == hRegion)
        {
            iFoundIndex = iIndex;
            break;
        }
    }
    if (iFoundIndex < 0)
    {
        return;
    }
    for (iIndex = iFoundIndex; iIndex < g_sDemoState.iZOrderCount - 1; ++iIndex)
    {
        g_sDemoState.rghZOrder[iIndex] = g_sDemoState.rghZOrder[iIndex + 1];
    }
    g_sDemoState.rghZOrder[g_sDemoState.iZOrderCount - 1] = hRegion;
}

static GuiTerminal::RegionHandle HitTestDemoRegion(_In_ INT iCol, _In_ INT iRow) noexcept
{
    INT iIndex;

    if (iCol < 0 || iRow < 0)
    {
        return nullptr;
    }

    for (iIndex = g_sDemoState.iZOrderCount - 1; iIndex >= 0; --iIndex)
    {
        if (g_sDemoState.rghZOrder[iIndex] == g_sDemoState.sMotionBanner.hRegion &&
            IsCellInsideMotion(&g_sDemoState.sMotionBanner, iCol, iRow) != FALSE)
        {
            return g_sDemoState.sMotionBanner.hRegion;
        }
        if (g_sDemoState.rghZOrder[iIndex] == g_sDemoState.sMotionFront.hRegion &&
            IsCellInsideMotion(&g_sDemoState.sMotionFront, iCol, iRow) != FALSE)
        {
            return g_sDemoState.sMotionFront.hRegion;
        }
        if (g_sDemoState.rghZOrder[iIndex] == g_sDemoState.sMotionBack.hRegion &&
            IsCellInsideMotion(&g_sDemoState.sMotionBack, iCol, iRow) != FALSE)
        {
            return g_sDemoState.sMotionBack.hRegion;
        }
        if (g_sDemoState.rghZOrder[iIndex] == g_sDemoState.sMotionOffscreen.hRegion &&
            IsCellInsideMotion(&g_sDemoState.sMotionOffscreen, iCol, iRow) != FALSE)
        {
            return g_sDemoState.sMotionOffscreen.hRegion;
        }
    }
    return nullptr;
}

static BOOL IsCellInsideMotion(_In_ const DemoRegionMotion* lpsMotion, _In_ INT iCol, _In_ INT iRow) noexcept
{
    if (!lpsMotion || !lpsMotion->hRegion)
    {
        return FALSE;
    }

    return ((iCol >= lpsMotion->iX) && (iCol < (lpsMotion->iX + lpsMotion->iWidth)) &&
            (iRow >= lpsMotion->iY) && (iRow < (lpsMotion->iY + lpsMotion->iHeight))) ? TRUE : FALSE;
}

static VOID WriteMouseStatus(_In_ GuiTerminal::Control* lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW,
                             _In_ INT iX, _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept
{
    WCHAR szBufferW[256];

    if (!lpGuiTerminal || !g_sDemoMouseCallbackContext.hRegionStatus)
    {
        return;
    }

    swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L"%-6ls %-4ls col=%d row=%d px=(%d,%d)",
               szButtonNameW, szActionNameW, iCol, iRow, iX, iY);

    lpGuiTerminal->ClearRegion(g_sDemoMouseCallbackContext.hRegionStatus);
    lpGuiTerminal->WriteRegion(g_sDemoMouseCallbackContext.hRegionStatus, L"\x1b[100;97m Mouse events \x1b[0m\r\n");
    lpGuiTerminal->WriteRegion(g_sDemoMouseCallbackContext.hRegionStatus, szBufferW);
}
