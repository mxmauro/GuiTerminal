# Integrate a GuiTerminal control

## Link and include

Use the exported CMake target rather than a library filename:

```cmake
find_package(GuiTerminal CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE GuiTerminal::GuiTerminal)
```

For static packages, include `GuiTerminalControl.h`. The exported target supplies Direct2D,
DirectWrite, Direct3D, and DXGI dependencies.

## Create the control and forward messages

Create the control during `WM_CREATE`. Give GuiTerminal the window messages first so it can manage
painting, sizing, DPI changes, scrollbars, and its mouse handling.

```cpp
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;

    if (GuiTerminal::Control::WndProc(hWnd, uMessage, wParam, lParam, &lResult) != FALSE)
    {
        return lResult;
    }

    switch (uMessage)
    {
        case WM_CREATE:
        {
            GuiTerminal::Control::Config config;
            GuiTerminal::Control *control;
            HRESULT hr;

            config = GuiTerminal::Control::Config{};
            config.iCols = 100;
            config.iRows = 32;
            config.szFontFamilyW = L"Cascadia Mono";
            config.fFontSize = 14.0f;
            config.crDefaultForeground = RGB(220U, 220U, 220U);
            config.crDefaultBackground = RGB(20U, 20U, 24U);
            control = nullptr;
            hr = GuiTerminal::Control::Create(hWnd, config, &control);
            if (FAILED(hr))
            {
                return -1;
            }
            control->Write(L"GuiTerminal is ready.\r\n");
            return 0;
        }
    }
    return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}
```

`Control::Create` attaches the control to the `HWND`. Use `Control::GetControl(hWnd)` to retrieve
it later. The control is released during `WM_NCDESTROY`; do not delete the returned pointer.

## Size the grid deliberately

`WM_SIZE` changes the render target but not the logical terminal grid. Call
`ResizeTerminal(cols, rows)` only when the number of terminal cells should change. Use
`GetPreferredClientSize` or `GetPreferredWindowSize` after creation when the window should fit an
exact initial grid.
