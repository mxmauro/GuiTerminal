#pragma once

#include "GuiTerminalBuffer.h"
#include "GuiTerminalRenderer.h"
#include <mutex>

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    class Control
    {
    public:
        enum StyleFlags : DWORD
        {
            StyleNone = 0U,
            StyleBold = 1U << 0,
            StyleUnderline = 1U << 1,
            StyleBlink = 1U << 2,
            StyleInverse = 1U << 3,
            StyleItalic = 1U << 4
        };

    private:
        Control() noexcept = default;
    public:
        Control(const Control&) = delete;
        Control(Control&&) = delete;
        ~Control() noexcept = default;

        Control& operator=(const Control&) = delete;
        Control& operator=(Control&&) = delete;

        // Create, initialize, attach, and arm a terminal control for a window.
        static HRESULT Create(_In_ HWND hWnd, _In_ INT iRows, _In_ INT iCols, _Out_ Control** lplpControl) noexcept;

        // Handle terminal-related window messages and return TRUE when consumed.
        static BOOL WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT* lplResult) noexcept;

        // Clear the whole terminal.
        VOID Clear() noexcept;

        // Scroll the whole terminal by line count, or the default region when the handle is null.
        VOID Scroll(_In_ INT iLineCount) noexcept;

        // Fill an area inside the whole terminal with explicit colors and style flags.
        VOID FillArea(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                      _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

        // Write UTF-16 text to the default region.
        VOID Write(_In_z_ LPCWSTR szTextW) noexcept;
        // Format and write UTF-16 text to the default region.
        VOID Print(_In_z_ LPCWSTR szFormatW, ...) noexcept;

        // Create a region inside the terminal in cell coordinates.
        HRESULT CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion) noexcept;
        // Destroy a created region handle.
        VOID DestroyRegion(_In_ RegionHandle hRegion) noexcept;

        // Clear a region, or the default region when the handle is null.
        VOID ClearRegion(_In_opt_ RegionHandle hRegion) noexcept;

        // Scroll a region by line count, or the default region when the handle is null.
        VOID ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

        // Fill an area inside a region with explicit colors and style flags.
        VOID FillRegionArea(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                            _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

        // Write UTF-16 text to a specific region handle.
        VOID WriteRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szTextW) noexcept;
        // Format and write UTF-16 text to a specific region.
        VOID PrintRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, ...) noexcept;

        // Resize the logical terminal grid.
        HRESULT ResizeGuiTerminal(_In_ INT iCols, _In_ INT iRows) noexcept;

        // Return the preferred client size for the current terminal grid.
        HRESULT GetPreferredClientSize(_Out_ LPSIZE lpSize) const noexcept;

        // Return the preferred window size for the current terminal grid.
        HRESULT GetPreferredWindowSize(_Out_ LPSIZE lpSize) const noexcept;

    private:
        // Initialize the terminal core and bind the renderer to the window.
        HRESULT Initialize(_In_ HWND hWnd, _In_ INT iCols, _In_ INT iRows) noexcept;

        // Draw the current terminal contents.
        HRESULT Present() noexcept;

        // Resize the underlying render target after WM_SIZE.
        HRESULT ResizeRenderTarget(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;

        // Refresh renderer DPI after WM_DPICHANGED.
        VOID RefreshDpi() noexcept;

        // Toggle the blink phase.
        VOID ToggleBlink() noexcept;

    private:
        mutable std::mutex m_mutex;
        HWND m_hWnd{};
        UINT_PTR m_uiBlinkTimerId{ 1U };
        INT m_iCols{};
        INT m_iRows{};
        Internals::Buffer m_sBuffer;
        Internals::Renderer m_sRenderer;
    };
}

