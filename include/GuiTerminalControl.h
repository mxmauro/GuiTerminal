#pragma once

#include "GuiTerminalBuffer.h"
#include "GuiTerminalRenderer.h"
#include <cstdarg>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

// -----------------------------------------------------------------------------

namespace GuiTerminal {

using CustomDrawCallback = std::function<VOID(_In_ DrawContext &drawContext, _In_ RegionHandle hRegion)>;
using RegionDestroyCallback = std::function<VOID(_In_ RegionHandle hRegion)>;
using CustomDrawResourceCleanupCallback = std::function<VOID(_In_ CustomDrawResourceCleanupReason cleanupReason)>;

// -----------------------------------------------------------------------------

/**
 * @brief Owns a GuiTerminal control attached to a Win32 window.
 */
class Control
{
  public:
    typedef struct Config_s
    {
        INT iRows{25};
        INT iCols{80};
        LPCWSTR szFontFamilyW{L"Consolas"};
        FLOAT fFontSize{12.0f};
        COLORREF crDefaultForeground{RGB(204U, 204U, 204U)};
        COLORREF crDefaultBackground{RGB(12U, 12U, 12U)};
    } Config;

    typedef enum StyleFlags_e : DWORD
    {
        StyleNone = 0U,
        StyleBold = 1U << 0,
        StyleUnderline = 1U << 1,
        StyleBlink = 1U << 2,
        StyleInverse = 1U << 3,
        StyleItalic = 1U << 4
    } StyleFlags;

    typedef enum StrokeType_e : DWORD
    {
        StrokeSingleLine = 0U,
        StrokeDoubleLine,
        StrokeShadeLight,
        StrokeShadeMedium,
        StrokeShadeDark,
        StrokeSolidBlock
    } StrokeType;

    typedef enum BoxSideFlags_e : DWORD
    {
        BoxSideNone = 0U,
        BoxSideTopDouble = 1U << 0,
        BoxSideRightDouble = 1U << 1,
        BoxSideBottomDouble = 1U << 2,
        BoxSideLeftDouble = 1U << 3
    } BoxSideFlags;

    typedef enum CursorStyle_e : DWORD
    {
        CursorBlock = 0U,
        CursorUnderscore,
        CursorBarLeft
    } CursorStyle;

  public:
    /** @brief Prevents copying a control. */
    Control(const Control &) = delete;
    /** @brief Prevents moving a control. */
    Control(Control &&) = delete;
    /** @brief Releases the control and its associated resources. */
    ~Control() noexcept = default;

    /** @brief Prevents copy assignment. */
    Control &operator=(const Control &) = delete;
    /** @brief Prevents move assignment. */
    Control &operator=(Control &&) = delete;

    /**
     * @brief Creates and attaches a control to a window.
     * @param hWnd Target window that owns the control.
     * @param configControl Initial terminal dimensions, font, and colors.
     * @param lplpControl Receives the created control; it remains owned by the window.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    static HRESULT Create(_In_ HWND hWnd, _In_ const Config &configControl, _Out_ Control **lplpControl) noexcept;
    /**
     * @brief Processes window messages owned by a control.
     * @param lplResult Receives the result when the message is handled.
     * @return TRUE when the caller must return @p lplResult; otherwise FALSE.
     */
    static BOOL WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT *lplResult) noexcept;
    /** @brief Returns the control associated with a window, or nullptr when none is attached. */
    static Control *GetControl(_In_ HWND hWnd);

    /** @brief Clears the root terminal region. */
    VOID Clear() noexcept;

    /** @brief Scrolls the root terminal region by the specified number of lines. */
    VOID Scroll(_In_ INT iLineCount) noexcept;
    /** @brief Moves a rectangle of root-region cells and fills the vacated cells. */
    VOID Move(_In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY,
              _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

    /** @brief Fills a root-region rectangle with a character and its attributes. */
    VOID Fill(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
              _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a horizontal stroke in the root region. */
    VOID DrawHorizontalLine(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                            _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a vertical stroke in the root region. */
    VOID DrawVerticalLine(_In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                          _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a box in the root region, using double-line edges selected by flags. */
    VOID DrawBox(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                 _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

    /**
     * @brief Writes ANSI-aware text at the root region cursor.
     * @param szTextW Null-terminated text, which may contain supported ANSI escape sequences.
     */
    VOID Write(_In_z_ LPCWSTR szTextW) noexcept;
    /** @brief Formats and writes ANSI-aware text at the root region cursor. */
    VOID Print(_In_z_ LPCWSTR szFormatW, ...) noexcept;
    /** @brief Writes variadic ANSI-aware text using an existing argument list. */
    VOID PrintV(_In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept;

    /** @brief Associates application-defined context with the root region. */
    HRESULT SetContext(_In_opt_ PVOID lpContext) noexcept;
    /** @brief Returns the application-defined context associated with the root region. */
    PVOID GetContext() const noexcept;

    /**
     * @brief Creates a cell-based child region.
     * @param iX Region origin column in the parent region's cell coordinates.
     * @param iY Region origin row in the parent region's cell coordinates.
     * @param iWidth Region width in positive cell units.
     * @param iHeight Region height in positive cell units.
     * @param lphRegion Receives the new region handle.
     * @param hRegionParent Optional parent; nullptr selects the root region.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                         _In_opt_ RegionHandle hRegionParent = nullptr) noexcept;
    /**
     * @brief Creates a child region rendered by a custom draw callback.
     * @param fnDrawCallback Callback invoked during painting; captured state must remain valid while the region exists.
     * @param hRegionParent Optional parent; nullptr selects the root region.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT CreateCustomDrawRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                                   _In_ const CustomDrawCallback &fnDrawCallback, _In_opt_ RegionHandle hRegionParent = nullptr) noexcept;
    /**
     * @brief Destroys a region and all of its child regions.
     * @param hRegion Region to destroy. Registered destruction callbacks run before removal.
     */
    VOID DestroyRegion(_In_ RegionHandle hRegion) noexcept;

    /** @brief Clears a region; a null handle denotes the root region. */
    VOID ClearRegion(_In_opt_ RegionHandle hRegion) noexcept;
    /**
     * @brief Requests a repaint after updating a region or its custom draw state.
     * @param hRegion Region that changed; nullptr may be used when the whole control changed.
     */
    VOID InvalidateRegion(_In_opt_ RegionHandle hRegion) noexcept;

    /** @brief Scrolls a region; a null handle denotes the root region. */
    VOID ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;
    /** @brief Moves a rectangle of cells within a region and fills the vacated cells. */
    VOID MoveRegion(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                    _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                    _In_ DWORD dwStyleFlags) noexcept;

    /** @brief Fills a region-local rectangle with a character and its attributes. */
    VOID FillRegion(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW,
                    _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a horizontal stroke using coordinates local to a region. */
    VOID DrawRegionHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType,
                                  _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a vertical stroke using coordinates local to a region. */
    VOID DrawRegionVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType,
                                _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a box using coordinates local to a region. */
    VOID DrawRegionBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                       _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

    /** @brief Writes ANSI-aware text at a region-local cursor. */
    VOID WriteRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szTextW) noexcept;
    /** @brief Formats and writes ANSI-aware text at a region-local cursor. */
    VOID PrintRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, ...) noexcept;
    /** @brief Writes variadic ANSI-aware text to a region using an existing argument list. */
    VOID PrintRegionV(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept;

    /**
     * @brief Moves and resizes a region within its parent.
     * @param iX New origin column in the parent region's cell coordinates.
     * @param iY New origin row in the parent region's cell coordinates.
     * @param iWidth New width in positive cell units.
     * @param iHeight New height in positive cell units.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept;
    /** @brief Brings a region in front of its siblings. */
    HRESULT BringRegionToFront(_In_ RegionHandle hRegion) noexcept;
    /** @brief Sends a region behind its siblings. */
    HRESULT SendRegionToBack(_In_ RegionHandle hRegion) noexcept;
    /**
     * @brief Places a region immediately after a sibling, or last among its siblings when the reference is null.
     * @param hRegion Region to reorder.
     * @param hRegionReference Optional sibling after which to place @p hRegion.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept;

    /** @brief Associates application-defined context with a region. */
    HRESULT SetRegionContext(_In_opt_ RegionHandle hRegion, _In_opt_ PVOID lpContext) noexcept;
    /** @brief Returns the application-defined context associated with a region. */
    PVOID GetRegionContext(_In_opt_ RegionHandle hRegion) const noexcept;

    /**
     * @brief Sets a callback invoked before a region is destroyed.
     * @param fnCallback Callback to install, or an empty function to remove it.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT SetRegionDestroyCallback(_In_ RegionHandle hRegion, _In_ const RegionDestroyCallback &fnCallback) noexcept;
    /**
     * @brief Sets the callback that releases custom-draw resources when they become invalid.
     * @param fnCallback Callback to install, or an empty function to remove it.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT SetCustomDrawRegionResourceCleanup(_In_ RegionHandle hRegion,
                                               _In_ const CustomDrawResourceCleanupCallback &fnCallback) noexcept;

    /** @brief Returns the first root-level region, or nullptr when no region exists. */
    RegionHandle GetFirstRegion() const noexcept;
    /** @brief Returns the last root-level region, or nullptr when no region exists. */
    RegionHandle GetLastRegion() const noexcept;
    /** @brief Returns the next sibling region. */
    RegionHandle GetNextRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    /** @brief Returns the previous sibling region. */
    RegionHandle GetPreviousRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    /** @brief Returns the first child of a region. */
    RegionHandle GetChildFirstRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
    /** @brief Returns the last child of a region. */
    RegionHandle GetChildLastRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
    /** @brief Returns a region's parent, or nullptr for a root-level region. */
    RegionHandle GetParentRegion(_In_ RegionHandle hRegion) const noexcept;

    /**
     * @brief Retrieves a region's location and size in its parent coordinates.
     * @param lpiX Optional origin-column output.
     * @param lpiY Optional origin-row output.
     * @param lpiWidth Optional width output.
     * @param lpiHeight Optional height output.
     */
    VOID GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                           _Out_opt_ LPINT lpiHeight) const noexcept;

    /**
     * @brief Converts terminal cell coordinates to coordinates local to a region.
     * @return TRUE when the terminal cell lies inside the region; otherwise FALSE.
     */
    BOOL ConvertToRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal, _Out_opt_ LPINT lpiColRegion,
                                    _Out_opt_ LPINT lpiRowRegion) const noexcept;
    /**
     * @brief Converts region-local cell coordinates to terminal coordinates.
     * @return TRUE when the region-local cell lies inside the region; otherwise FALSE.
     */
    BOOL ConvertFromRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion, _Out_opt_ LPINT lpiColTerminal,
                                      _Out_opt_ LPINT lpiRowTerminal) const noexcept;

    /** @brief Shows the cursor in a region; a null handle denotes the root region. */
    VOID ShowCursor(_In_opt_ RegionHandle hRegion) noexcept;
    /** @brief Hides the cursor. */
    VOID HideCursor() noexcept;
    /** @brief Selects the visual style used to draw the cursor. */
    VOID SetCursorStyle(_In_ CursorStyle style) noexcept;

    /**
     * @brief Resizes the terminal grid while preserving content where possible.
     * @param iCols New positive root-grid width in cells.
     * @param iRows New positive root-grid height in cells.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT ResizeTerminal(_In_ INT iCols, _In_ INT iRows) noexcept;
    /** @brief Retrieves the terminal grid dimensions in columns and rows. */
    VOID GetTerminalSize(_Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows) const noexcept;

    /**
     * @brief Retrieves the client size required to display the full terminal grid.
     * @param lpSize Receives the required client size in pixels.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT GetPreferredClientSize(_Out_ LPSIZE lpSize) const noexcept;
    /**
     * @brief Retrieves the window size required to display the full terminal grid.
     * @param bHasMenu TRUE when the target window has a menu.
     * @return S_OK on success; otherwise a Win32-style failure code.
     */
    HRESULT GetPreferredWindowSize(_Out_ LPSIZE lpSize, _In_opt_ BOOL bHasMenu = FALSE) const noexcept;

    /** @brief Retrieves the size of one terminal cell in pixels. */
    HRESULT GetCellSize(_Out_ LPSIZE lpSize) const noexcept;
    /**
     * @brief Retrieves the client rectangle occupied by a terminal cell.
     * @return TRUE when the cell is visible; otherwise FALSE.
     */
    BOOL GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept;
    /**
     * @brief Converts a client pixel position to terminal cell coordinates.
     * @return TRUE when the position lies over a terminal cell; otherwise FALSE.
     */
    BOOL GetCellFromPosition(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept;

  private:
    typedef enum ScrollBarPart_e
    {
        ScrollBarPartNone = 0,
        ScrollBarPartVerticalThumb,
        ScrollBarPartHorizontalThumb
    } ScrollBarPart;

  private:
    Control() noexcept = default;

  private:
    HRESULT Initialize(_In_ HWND hWnd, _In_ const Config &configControl) noexcept;
    HRESULT Present() noexcept;
    HRESULT ResizeRenderTarget(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;
    VOID UpdateScrollBars() noexcept;
    BOOL HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept;
    BOOL HandleMouseLeave() noexcept;
    BOOL HandleLeftButtonDown(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbBeginCapture) noexcept;
    BOOL HandleLeftButtonUp() noexcept;
    BOOL HandleMouseWheel(_In_ SHORT iDelta) noexcept;
    VOID RefreshDpi() noexcept;
    VOID ToggleBlink() noexcept;
    HRESULT StartBlinkThread() noexcept;
    VOID StopBlinkThread() noexcept;
    static VOID BlinkThreadEntry(_In_ Control *lpControl) noexcept;

  private:
    static std::mutex m_mutex;
    static std::unordered_map<HWND, Control *> m_mapControls;
    HWND m_hWnd{};
    HANDLE m_hBlinkStopEvent{};
    INT m_iCols{};
    INT m_iRows{};
    BOOL m_bTrackingMouse{FALSE};
    BOOL m_bDraggingScrollBar{FALSE};
    ScrollBarPart m_scrollBarPartDragging{ScrollBarPartNone};
    INT m_iScrollDragOriginX{};
    INT m_iScrollDragOriginY{};
    INT m_iScrollOffsetOriginX{};
    INT m_iScrollOffsetOriginY{};
    std::thread m_threadBlink;
    Internals::Buffer m_sBuffer;
    Internals::Renderer m_sRenderer;
};

} // namespace GuiTerminal
