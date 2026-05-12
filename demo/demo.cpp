#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "..\include\GuiTerminalControl.h"

#define TERMINAL_COLS        80
#define TERMINAL_ROWS        25
#define TERMINAL_FONT_FAMILY L"Consolas"
#define TERMINAL_FONT_SIZE   18.0f
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

static HRESULT ResizeWindowToPreferredClientArea(_In_ HWND hWnd, _In_ GuiTerminal::Control* lpGuiTerminal) noexcept;

static HRESULT RunDemo(_In_ GuiTerminal::Control* lpGuiTerminal) noexcept;
static VOID CALLBACK DemoMouseClickCallback(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::Control::MouseClickEvent eEvent,
                                            _In_ DWORD dwKeyFlags, _In_ INT iCol, _In_ INT iRow, _In_ INT iX, _In_ INT iY,
                                            _In_opt_ LPVOID lpContext) noexcept;
static LPCWSTR DescribeMouseClickEvent(_In_ GuiTerminal::Control::MouseClickEvent eEvent) noexcept;

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

    lpGuiTerminal->Write(L"\x1b[1;97mWin32 + Direct2D/DirectWrite terminal demo\x1b[0m\r\n"
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
                         L"\x1b[8;1H\x1b[2KTwo independent scrolling regions below:");

    hr = lpGuiTerminal->CreateRegion(0, 8, TERMINAL_COLS, 2, &hRegionStatus);
    if (FAILED(hr))
    {
        return hr;
    }
    g_sDemoMouseCallbackContext.hRegionStatus = hRegionStatus;
    lpGuiTerminal->SetMouseClickCallback(DemoMouseClickCallback, &g_sDemoMouseCallbackContext);
    lpGuiTerminal->WriteRegion(hRegionStatus, L"\x1b[100;97m Mouse events \x1b[0m\r\n"
                                              L"Click inside the terminal grid with left, right or middle buttons.");

    hr = lpGuiTerminal->CreateRegion(0, 10, 39, 15, &hRegionLeft);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = lpGuiTerminal->CreateRegion(41, 10, 39, 15, &hRegionRight);
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

static VOID CALLBACK DemoMouseClickCallback(_In_ GuiTerminal::Control* lpGuiTerminal, _In_ GuiTerminal::Control::MouseClickEvent eEvent,
                                            _In_ DWORD dwKeyFlags, _In_ INT iCol, _In_ INT iRow, _In_ INT iX, _In_ INT iY,
                                            _In_opt_ LPVOID lpContext) noexcept
{
    DemoMouseCallbackContext* lpDemoMouseCallbackContext;
    WCHAR szBufferW[256];

    lpDemoMouseCallbackContext = reinterpret_cast<DemoMouseCallbackContext*>(lpContext);
    if ((!lpGuiTerminal) || (!lpDemoMouseCallbackContext) || (!lpDemoMouseCallbackContext->hRegionStatus))
    {
        return;
    }

    swprintf_s(szBufferW, sizeof(szBufferW) / sizeof(szBufferW[0]), L"%-10s col=%d row=%d px=(%d,%d) ctrl=%d alt=%d",
               DescribeMouseClickEvent(eEvent), iCol, iRow, iX, iY,
               ((dwKeyFlags & GuiTerminal::Control::MouseKeyFlagControl) != 0U) ? 1 : 0,
               ((dwKeyFlags & GuiTerminal::Control::MouseKeyFlagAlt) != 0U) ? 1 : 0);

    lpGuiTerminal->ClearRegion(lpDemoMouseCallbackContext->hRegionStatus);
    lpGuiTerminal->WriteRegion(lpDemoMouseCallbackContext->hRegionStatus, L"\x1b[100;97m Mouse events \x1b[0m\r\n");
    lpGuiTerminal->WriteRegion(lpDemoMouseCallbackContext->hRegionStatus, szBufferW);
}

static LPCWSTR DescribeMouseClickEvent(_In_ GuiTerminal::Control::MouseClickEvent eEvent) noexcept
{
    switch (eEvent)
    {
        case GuiTerminal::Control::MouseClickEventLeftDown:
            return L"LeftDown";
        case GuiTerminal::Control::MouseClickEventLeftUp:
            return L"LeftUp";
        case GuiTerminal::Control::MouseClickEventRightDown:
            return L"RightDown";
        case GuiTerminal::Control::MouseClickEventRightUp:
            return L"RightUp";
        case GuiTerminal::Control::MouseClickEventMiddleDown:
            return L"MiddleDown";
        case GuiTerminal::Control::MouseClickEventMiddleUp:
            return L"MiddleUp";
    }
    return L"Unknown";
}
