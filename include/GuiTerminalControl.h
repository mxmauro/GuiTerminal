#pragma once

#include "GuiTerminalBuffer.h"
#include "GuiTerminalRenderer.h"
#include <cstdarg>
#include <mutex>
#include <thread>

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    class Control
    {
    public:
        struct Config
        {
            INT iRows{ 25 };
            INT iCols{ 80 };
            LPCWSTR szFontFamilyW{ L"Consolas" };
            FLOAT fFontSize{ 12.0f };
            COLORREF crDefaultForeground{ RGB(204U, 204U, 204U) };
            COLORREF crDefaultBackground{ RGB(12U, 12U, 12U) };
        };

        enum StyleFlags : DWORD
        {
            StyleNone = 0U,
            StyleBold = 1U << 0,
            StyleUnderline = 1U << 1,
            StyleBlink = 1U << 2,
            StyleInverse = 1U << 3,
            StyleItalic = 1U << 4
        };

        enum StrokeType : DWORD
        {
            StrokeSingleLine = 0U,
            StrokeDoubleLine,
            StrokeShadeLight,
            StrokeShadeMedium,
            StrokeShadeDark,
            StrokeSolidBlock
        };

        enum BoxSideFlags : DWORD
        {
            BoxSideNone = 0U,
            BoxSideTopDouble = 1U << 0,
            BoxSideRightDouble = 1U << 1,
            BoxSideBottomDouble = 1U << 2,
            BoxSideLeftDouble = 1U << 3
        };

        enum CursorStyle : DWORD
        {
            CursorBlock = 0U,
            CursorUnderscore,
            CursorBarLeft
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
        static HRESULT Create(_In_ HWND hWnd, _In_ const Config& configControl, _Out_ Control** lplpControl) noexcept;

        // Handle terminal-related window messages and return TRUE when consumed.
        static BOOL WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT* lplResult) noexcept;

        // Get the control instance from the window handle.
        static Control* GetControl(_In_ HWND hWnd);

        // Clear the whole terminal.
        VOID Clear() noexcept;

        // Scroll the whole terminal by line count, or the default region when the handle is null.
        VOID Scroll(_In_ INT iLineCount) noexcept;

        // Fill an area inside the whole terminal with explicit colors and style flags.
        VOID FillArea(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                      _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Move an area inside the whole terminal and fill the vacated cells with the provided character and attributes.
        VOID Move(_In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY,
                  _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Draw a horizontal stroke in the whole terminal.
        VOID DrawHorizontalLine(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                                _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Draw a vertical stroke in the whole terminal.
        VOID DrawVerticalLine(_In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                              _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Draw a box in the whole terminal.
        VOID DrawBox(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                     _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

        // Write UTF-16 text to the default region.
        VOID Write(_In_z_ LPCWSTR szTextW) noexcept;
        // Format and write UTF-16 text to the default region.
        VOID Print(_In_z_ LPCWSTR szFormatW, ...) noexcept;
        // Format and write UTF-16 text to the default region.
        VOID PrintV(_In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept;
        // Associates a context pointer with the default region.
        HRESULT SetContext(_In_opt_ PVOID lpContext) noexcept;
        // Returns the context pointer associated with the default region.
        PVOID GetContext() const noexcept;

        // Create a region in cell coordinates relative to the specified parent, or the terminal when the parent is null.
        HRESULT CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion,
                             _In_opt_ RegionHandle hRegionParent = nullptr) noexcept;
        // Destroy a created region handle.
        VOID DestroyRegion(_In_ RegionHandle hRegion) noexcept;

        // Clear a region, or the default region when the handle is null.
        VOID ClearRegion(_In_opt_ RegionHandle hRegion) noexcept;

        // Scroll a region by line count, or the default region when the handle is null.
        VOID ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

        // Fill an area inside a region with explicit colors and style flags.
        VOID FillRegionArea(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                            _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                            _In_ DWORD dwStyleFlags) noexcept;
        // Move an area inside a region and fill the vacated cells with the provided character and attributes.
        VOID MoveRegion(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                        _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground,
                        _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Draw a horizontal stroke in a region.
        VOID DrawRegionHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType,
                                      _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Draw a vertical stroke in a region.
        VOID DrawRegionVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType,
                                    _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
        // Draw a box in a region.
        VOID DrawRegionBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                           _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                           _In_ DWORD dwStyleFlags) noexcept;

        // Write UTF-16 text to a specific region handle.
        VOID WriteRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szTextW) noexcept;
        // Format and write UTF-16 text to a specific region.
        VOID PrintRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, ...) noexcept;
        // Format and write UTF-16 text to a specific region.
        VOID PrintRegionV(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept;

        // Relocates the specified region.
        HRESULT RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept;
        // Moves the specified region to the top of its sibling z-order.
        HRESULT BringRegionToFront(_In_ RegionHandle hRegion) noexcept;
        // Moves the specified region behind its sibling regions.
        HRESULT SendRegionToBack(_In_ RegionHandle hRegion) noexcept;
        // Moves the specified region immediately after the reference sibling, or to the top when the reference handle is null.
        HRESULT MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept;
        // Associates a context pointer with the specified region, or the default region when the handle is null.
        HRESULT SetRegionContext(_In_opt_ RegionHandle hRegion, _In_opt_ PVOID lpContext) noexcept;
        // Returns the context pointer associated with the specified region, or the default region when the handle is null.
        PVOID GetRegionContext(_In_opt_ RegionHandle hRegion) const noexcept;
        // Returns the frontmost non-root region, or null when no created regions exist.
        RegionHandle GetFirstRegion() const noexcept;
        // Returns the backmost non-root region, or null when no created regions exist.
        RegionHandle GetLastRegion() const noexcept;
        // Returns the next region toward the back within the same sibling list, or the first root child when the handle is null.
        RegionHandle GetNextRegion(_In_opt_ RegionHandle hRegion) const noexcept;
        // Returns the previous region toward the front within the same sibling list, or the last root child when the handle is null.
        RegionHandle GetPreviousRegion(_In_opt_ RegionHandle hRegion) const noexcept;
        // Returns the frontmost child of the specified parent, or the frontmost root child when the handle is null.
        RegionHandle GetChildFirstRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
        // Returns the backmost child of the specified parent, or the backmost root child when the handle is null.
        RegionHandle GetChildLastRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
        // Returns the immediate parent region, or null when the parent is the terminal root.
        RegionHandle GetParentRegion(_In_ RegionHandle hRegion) const noexcept;

        // Gets the location of the specified region in cell coordinates relative to its immediate parent.
        VOID GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                               _Out_opt_ LPINT lpiHeight) const noexcept;
        // Return the current terminal grid size in cells.
        VOID GetTerminalSize(_Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows) const noexcept;

        // Translate zero-based terminal coordinates to zero-based region coordinates for a specific region.
        // Returns FALSE only when the region handle is invalid.
        BOOL ConvertToRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal,
                                        _Out_opt_ LPINT lpiColRegion, _Out_opt_ LPINT lpiRowRegion) const noexcept;

        // Translate zero-based region coordinates to zero-based terminal coordinates for a specific region.
        // Returns FALSE when the region handle is invalid or the translated result cannot fit in INT.
        BOOL ConvertFromRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion,
                                          _Out_opt_ LPINT lpiColTerminal, _Out_opt_ LPINT lpiRowTerminal) const noexcept;

        // Show the cursor for the specified region, or the root terminal cursor when the handle is null.
        VOID ShowCursor(_In_opt_ RegionHandle hRegion) noexcept;
        // Hide the rendered cursor without changing any logical region cursor positions.
        VOID HideCursor() noexcept;
        // Set the rendered cursor style.
        VOID SetCursorStyle(_In_ CursorStyle style) noexcept;

        // Resize the logical terminal grid.
        HRESULT ResizeTerminal(_In_ INT iCols, _In_ INT iRows) noexcept;

        // Return the size of a single cell in pixels.
        HRESULT GetCellSize(_Out_ LPSIZE lpSize) const noexcept;

        // Return the client-space rectangle for a zero-based cell, in pixels.
        BOOL GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept;

        // Convert a client-space pixel position to a zero-based cell coordinate.
        BOOL GetCellFromPosition(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept;

        // Return the preferred client size for the current terminal grid.
        HRESULT GetPreferredClientSize(_Out_ LPSIZE lpSize) const noexcept;

        // Return the preferred window size for the current terminal grid.
        HRESULT GetPreferredWindowSize(_Out_ LPSIZE lpSize, _In_opt_ BOOL bHasMenu = FALSE) const noexcept;

    private:
        enum WindowMessageId : UINT
        {
            WindowMessageBlinkRedraw = WM_APP + 1
        };

        enum ScrollBarPart
        {
            ScrollBarPartNone = 0,
            ScrollBarPartVerticalThumb,
            ScrollBarPartHorizontalThumb
        };

        // Initialize the terminal core and bind the renderer to the window.
        HRESULT Initialize(_In_ HWND hWnd, _In_ const Config& configControl) noexcept;

        // Draw the current terminal contents.
        HRESULT Present() noexcept;

        // Resize the underlying render target after WM_SIZE.
        HRESULT ResizeRenderTarget(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;

        // Update pixel-based scrolling and scroll bar visibility after size/content changes.
        VOID UpdateScrollBars() noexcept;

        // Update drag/hover state for custom scroll bars.
        BOOL HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept;
        BOOL HandleMouseLeave() noexcept;
        BOOL HandleLeftButtonDown(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbBeginCapture) noexcept;
        BOOL HandleLeftButtonUp() noexcept;
        BOOL HandleMouseWheel(_In_ SHORT iDelta) noexcept;

        // Refresh renderer DPI after WM_DPICHANGED.
        VOID RefreshDpi() noexcept;

        // Toggle the blink phase.
        VOID ToggleBlink() noexcept;
        HRESULT StartBlinkThread() noexcept;
        VOID StopBlinkThread() noexcept;
        static VOID BlinkThreadEntry(_In_ Control* lpControl) noexcept;

    private:
        mutable std::mutex m_mutex;
        HWND m_hWnd{};
        HANDLE m_hBlinkStopEvent{};
        INT m_iCols{};
        INT m_iRows{};
        BOOL m_bTrackingMouse{ FALSE };
        BOOL m_bDraggingScrollBar{ FALSE };
        ScrollBarPart m_scrollBarPartDragging{ ScrollBarPartNone };
        INT m_iScrollDragOriginX{};
        INT m_iScrollDragOriginY{};
        INT m_iScrollOffsetOriginX{};
        INT m_iScrollOffsetOriginY{};
        std::thread m_threadBlink;
        Internals::Buffer m_sBuffer;
        Internals::Renderer m_sRenderer;
    };
}
