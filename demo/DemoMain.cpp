#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "DemoApp.h"
#include "DemoLayout.h"
#include "resource.h"
#include <windows.h>
#include <windowsx.h>

#define WINDOW_CLASS_NAME L"SampleGuiTerminalWindow"
#define WINDOW_TITLE L"Sample GuiTerminal"
#define ANIMATION_TIMER_ID 1U
#define ANIMATION_TIMER_INTERVAL 10U

// -----------------------------------------------------------------------------

static HRESULT EnablePerMonitorDpiAwareness() noexcept;
static LRESULT CALLBACK MainWndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam) noexcept;
static LRESULT HandleCreate(_In_ HWND hWnd) noexcept;
static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown, _In_z_ LPCWSTR szButtonNameW,
                                 _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept;

// -----------------------------------------------------------------------------

INT APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLineW, _In_ INT nCmdShow)
{
    WNDCLASSEXW sWcExW;
    HWND hWnd;
    MSG msg;
    HRESULT hr;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLineW);
    hr = EnablePerMonitorDpiAwareness();
    if (FAILED(hr))
    {
        return static_cast<INT>(hr);
    }
    sWcExW = WNDCLASSEXW{};
    sWcExW.cbSize = sizeof(sWcExW);
    sWcExW.style = CS_HREDRAW | CS_VREDRAW;
    sWcExW.lpfnWndProc = MainWndProc;
    sWcExW.hInstance = hInstance;
    sWcExW.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WINDOWSPROJECT1));
    sWcExW.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    sWcExW.lpszClassName = WINDOW_CLASS_NAME;
    sWcExW.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_SMALL));
    if (RegisterClassExW(&sWcExW) == FALSE)
    {
        return static_cast<INT>(HRESULT_FROM_WIN32(GetLastError()));
    }
    hWnd = CreateWindowExW(0, WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr,
                           nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return static_cast<INT>(HRESULT_FROM_WIN32(GetLastError()));
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    msg = MSG{};
    while (GetMessageW(&msg, nullptr, 0U, 0U) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<INT>(msg.wParam);
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
    case WM_TIMER:
        if (wParam == ANIMATION_TIMER_ID)
        {
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
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
    case WM_DESTROY:
        KillTimer(hWnd, ANIMATION_TIMER_ID);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}

static LRESULT HandleCreate(_In_ HWND hWnd) noexcept
{
    GuiTerminal::Control *lpGuiTerminal;
    GuiTerminal::Control::Config sConfig;
    SIZE sizeClient;
    HRESULT hr;

    sConfig = GuiTerminal::Control::Config{};
    sConfig.iRows = TERMINAL_ROWS;
    sConfig.iCols = TERMINAL_COLS;
    sConfig.szFontFamilyW = TERMINAL_FONT_FAMILY;
    sConfig.fFontSize = TERMINAL_FONT_SIZE;
    lpGuiTerminal = nullptr;
    hr = GuiTerminal::Control::Create(hWnd, sConfig, &lpGuiTerminal);
    if (FAILED(hr))
    {
        return -1;
    }
    hr = lpGuiTerminal->GetPreferredWindowSize(&sizeClient);
    if (FAILED(hr))
    {
        return -1;
    }
    if (SetWindowPos(hWnd, nullptr, 0, 0, sizeClient.cx, sizeClient.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) == FALSE)
    {
        return -1;
    }
    if (FAILED(DemoInitialize(lpGuiTerminal)))
    {
        return -1;
    }
    return SetTimer(hWnd, ANIMATION_TIMER_ID, ANIMATION_TIMER_INTERVAL, nullptr) ? 0 : -1;
}

static LRESULT HandleMouseButton(_In_ HWND hWnd, _In_ BOOL bLeftButton, _In_ BOOL bButtonDown, _In_z_ LPCWSTR szButtonNameW,
                                 _In_z_ LPCWSTR szActionNameW, _In_ INT iX, _In_ INT iY) noexcept
{
    DemoHandleMouseButton(GuiTerminal::Control::GetControl(hWnd), bLeftButton, bButtonDown, szButtonNameW, szActionNameW, iX, iY);
    InvalidateRect(hWnd, nullptr, FALSE);
    return 0;
}
