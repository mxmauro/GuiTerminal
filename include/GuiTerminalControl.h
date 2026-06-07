#pragma once

#include "GuiTerminalBuffer.h"
#include "GuiTerminalRenderer.h"
#include <cstdarg>
#include <mutex>
#include <thread>
#include <unordered_map>

// -----------------------------------------------------------------------------

namespace GuiTerminal {

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

    static HRESULT Create(_In_ HWND hWnd, _In_ const Config& configControl, _Out_ Control** lplpControl) noexcept;
    static BOOL WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT* lplResult) noexcept;
    static Control* GetControl(_In_ HWND hWnd);

    VOID Clear() noexcept;
    VOID Scroll(_In_ INT iLineCount) noexcept;

    VOID FillArea(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                  _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID Move(_In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY,
              _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID DrawHorizontalLine(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                            _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID DrawVerticalLine(_In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                          _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID DrawBox(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                 _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

    VOID Write(_In_z_ LPCWSTR szTextW) noexcept;
    VOID Print(_In_z_ LPCWSTR szFormatW, ...) noexcept;
    VOID PrintV(_In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept;
    HRESULT SetContext(_In_opt_ PVOID lpContext) noexcept;
    PVOID GetContext() const noexcept;

    HRESULT CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion,
                         _In_opt_ RegionHandle hRegionParent = nullptr) noexcept;
    VOID DestroyRegion(_In_ RegionHandle hRegion) noexcept;
    VOID ClearRegion(_In_opt_ RegionHandle hRegion) noexcept;
    VOID ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

    VOID FillRegionArea(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                        _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                        _In_ DWORD dwStyleFlags) noexcept;
    VOID MoveRegion(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                    _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground,
                    _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID DrawRegionHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType,
                                  _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID DrawRegionVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType,
                                _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    VOID DrawRegionBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                       _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                       _In_ DWORD dwStyleFlags) noexcept;

    VOID WriteRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szTextW) noexcept;
    VOID PrintRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, ...) noexcept;
    VOID PrintRegionV(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept;

    HRESULT RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept;
    HRESULT BringRegionToFront(_In_ RegionHandle hRegion) noexcept;
    HRESULT SendRegionToBack(_In_ RegionHandle hRegion) noexcept;
    HRESULT MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept;
    HRESULT SetRegionContext(_In_opt_ RegionHandle hRegion, _In_opt_ PVOID lpContext) noexcept;
    PVOID GetRegionContext(_In_opt_ RegionHandle hRegion) const noexcept;
    RegionHandle GetFirstRegion() const noexcept;
    RegionHandle GetLastRegion() const noexcept;
    RegionHandle GetNextRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    RegionHandle GetPreviousRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    RegionHandle GetChildFirstRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
    RegionHandle GetChildLastRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
    RegionHandle GetParentRegion(_In_ RegionHandle hRegion) const noexcept;

    VOID GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                           _Out_opt_ LPINT lpiHeight) const noexcept;
    VOID GetTerminalSize(_Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows) const noexcept;

    BOOL ConvertToRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal,
                                    _Out_opt_ LPINT lpiColRegion, _Out_opt_ LPINT lpiRowRegion) const noexcept;
    BOOL ConvertFromRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion,
                                      _Out_opt_ LPINT lpiColTerminal, _Out_opt_ LPINT lpiRowTerminal) const noexcept;

    VOID ShowCursor(_In_opt_ RegionHandle hRegion) noexcept;
    VOID HideCursor() noexcept;
    VOID SetCursorStyle(_In_ CursorStyle style) noexcept;
    HRESULT ResizeTerminal(_In_ INT iCols, _In_ INT iRows) noexcept;
    HRESULT GetCellSize(_Out_ LPSIZE lpSize) const noexcept;
    BOOL GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept;
    BOOL GetCellFromPosition(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept;
    HRESULT GetPreferredClientSize(_Out_ LPSIZE lpSize) const noexcept;
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

    HRESULT Initialize(_In_ HWND hWnd, _In_ const Config& configControl) noexcept;
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
    static VOID BlinkThreadEntry(_In_ Control* lpControl) noexcept;

private:
    static std::mutex m_mutex;
    static std::unordered_map<HWND, Control*> m_mapControls;
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
