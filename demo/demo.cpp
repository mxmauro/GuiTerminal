#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "..\include\GuiTerminalControl.h"

#define TERMINAL_COLS        80
#define TERMINAL_ROWS        25
#define TERMINAL_FONT_FAMILY L"Cascadia Mono"
#define TERMINAL_FONT_SIZE   12.0f

#define WINDOW_CLASS_NAME    L"SampleGuiTerminalWindow"
#define WINDOW_TITLE         L"Sample GuiTerminal"

// -----------------------------------------------------------------------------

typedef struct DemoMouseCallbackContext
{
    GuiTerminal::RegionHandle hRegionStatus;
} DemoMouseCallbackContext;

static HRESULT EnablePerMonitorDpiAwareness() noexcept;

static HRESULT CreateMainWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_z_ LPCWSTR szWindowClassW,
                                _In_z_ LPCWSTR szTitleW, _Out_ HWND* lphWnd) noexcept;

static LRESULT CALLBACK MainWndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam) noexcept;
static LRESULT HandleCreate(_In_ HWND hWnd) noexcept;
static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ LPCWSTR szButtonNameW, _In_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept;

static HRESULT ResizeWindowToPreferredClientArea(_In_ HWND hWnd, _In_ GuiTerminal::Control* lpGuiTerminal) noexcept;

static HRESULT RunDemo(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static VOID WriteMouseStatus(_In_ GuiTerminal::Control* lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW, _In_ INT iX,
                             _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept;

// -----------------------------------------------------------------------------

static DemoMouseCallbackContext g_sDemoMouseCallbackContext{};

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

    hWnd = CreateWindowExW(0, szWindowClassW, szTitleW, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr,
                           hInstance, nullptr);
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
            return HandleMouseButton(hWnd, L"Left", L"Down", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_LBUTTONUP:
            return HandleMouseButton(hWnd, L"Left", L"Up", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_RBUTTONDOWN:
            return HandleMouseButton(hWnd, L"Right", L"Down", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_RBUTTONUP:
            return HandleMouseButton(hWnd, L"Right", L"Up", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_MBUTTONDOWN:
            return HandleMouseButton(hWnd, L"Middle", L"Down", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        case WM_MBUTTONUP:
            return HandleMouseButton(hWnd, L"Middle", L"Up", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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

static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ LPCWSTR szButtonNameW, _In_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept
{
    GuiTerminal::Control* lpGuiTerminal;
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
    GuiTerminal::RegionHandle hRegionLeft;
    GuiTerminal::RegionHandle hRegionRight;
    WCHAR szBufferW[256];
    INT iIndex;
    HRESULT hr;

    hRegionStatus = nullptr;
    lpGuiTerminal->Clear();

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
                         L"\x1b[5;1H\x1b[2KTwo independent scrolling regions below:");

    hr = lpGuiTerminal->CreateRegion(0, 23, TERMINAL_COLS, 2, &hRegionStatus);
    if (FAILED(hr))
    {
        return hr;
    }
    g_sDemoMouseCallbackContext.hRegionStatus = hRegionStatus;
    lpGuiTerminal->WriteRegion(hRegionStatus, L"\x1b[100;97m Mouse events \x1b[0m\r\n"
                                              L"Left/Right/Middle up/down. Outside cells reports col=-1 row=-1.");

    hr = lpGuiTerminal->CreateRegion(0, 7, 39, 15, &hRegionLeft);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(41, 7, 39, 15, &hRegionRight);
    if (FAILED(hr))
    {
        return hr;
    }

    lpGuiTerminal->WriteRegion(hRegionLeft, L"\x1b[44;97m Left region \x1b[0m\r\n");
    lpGuiTerminal->WriteRegion(hRegionRight, L"\x1b[42;30m Right region \x1b[0m\r\n");

    for (iIndex = 1; iIndex <= 20; iIndex++)
    {
        swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L"\x1b[38;5;81mleft\x1b[0m line %d\r\n", iIndex);
        lpGuiTerminal->WriteRegion(hRegionLeft, szBufferW);

        swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L"\x1b[38;2;255;120;120mright\x1b[0m line %d\r\n", iIndex);
        lpGuiTerminal->WriteRegion(hRegionRight, szBufferW);
    }

    return S_OK;
}

static VOID WriteMouseStatus(_In_ GuiTerminal::Control* lpGuiTerminal, _In_z_ LPCWSTR szButtonNameW, _In_z_ LPCWSTR szActionNameW, _In_ INT iX,
                             _In_ INT iY, _In_ INT iCol, _In_ INT iRow) noexcept
{
    WCHAR szBufferW[256];

    if (!lpGuiTerminal || !g_sDemoMouseCallbackContext.hRegionStatus)
    {
        return;
    }

    swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L"%-6ls %-4ls col=%d row=%d px=(%d,%d)", szButtonNameW, szActionNameW, iCol,
               iRow, iX, iY);

    lpGuiTerminal->ClearRegion(g_sDemoMouseCallbackContext.hRegionStatus);
    lpGuiTerminal->WriteRegion(g_sDemoMouseCallbackContext.hRegionStatus, L"\x1b[100;97m Mouse events \x1b[0m\r\n");
    lpGuiTerminal->WriteRegion(g_sDemoMouseCallbackContext.hRegionStatus, szBufferW);
}
