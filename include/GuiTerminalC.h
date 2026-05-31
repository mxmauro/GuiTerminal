#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// -----------------------------------------------------------------------------

#if defined(GUITERMINAL_SHARED)
#  if defined(GUITERMINAL_BUILD_DLL)
#    define GUITERMINAL_CONTROL_API __declspec(dllexport)
#  else
#    define GUITERMINAL_CONTROL_API __declspec(dllimport)
#  endif
#else
#  define GUITERMINAL_CONTROL_API
#endif

// -----------------------------------------------------------------------------

enum GuiTerminalStyleFlags
{
    GuiTerminalStyleNone = 0U,
    GuiTerminalStyleBold = 1U << 0,
    GuiTerminalStyleUnderline = 1U << 1,
    GuiTerminalStyleBlink = 1U << 2,
    GuiTerminalStyleInverse = 1U << 3,
    GuiTerminalStyleItalic = 1U << 4
};

enum GuiTerminalStrokeType
{
    GuiTerminalStrokeSingleLine = 0U,
    GuiTerminalStrokeDoubleLine,
    GuiTerminalStrokeShadeLight,
    GuiTerminalStrokeShadeMedium,
    GuiTerminalStrokeShadeDark,
    GuiTerminalStrokeSolidBlock
};

enum GuiTerminalBoxSideFlags
{
    GuiTerminalBoxSideNone = 0U,
    GuiTerminalBoxSideTopDouble = 1U << 0,
    GuiTerminalBoxSideRightDouble = 1U << 1,
    GuiTerminalBoxSideBottomDouble = 1U << 2,
    GuiTerminalBoxSideLeftDouble = 1U << 3
};

enum GuiTerminalCursorStyle
{
    GuiTerminalCursorBlock = 0U,
    GuiTerminalCursorUnderscore,
    GuiTerminalCursorBarLeft
};

typedef struct GuiTerminalControlConfig
{
    INT iRows;
    INT iCols;
    LPCWSTR szFontFamilyW;
    FLOAT fFontSize;
    COLORREF crDefaultForeground;
    COLORREF crDefaultBackground;
} GuiTerminalControlConfig;

typedef struct GuiTerminalControl_s GuiTerminalControl;
typedef struct GuiTerminalRegion_s* GuiTerminalRegion;

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_Create(_In_ HWND hWnd, _In_ const GuiTerminalControlConfig* lpConfig, _Out_ GuiTerminalControl** lplpControl);
GUITERMINAL_CONTROL_API
BOOL GuiTerminalControl_WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT* lplResult);
GUITERMINAL_CONTROL_API
GuiTerminalControl* GuiTerminalControl_GetFromWindow(_In_ HWND hWnd);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Clear(_In_ GuiTerminalControl* lpControl);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Scroll(_In_ GuiTerminalControl* lpControl, _In_ INT iLineCount);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_FillArea(_In_ GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                 _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Move(_In_ GuiTerminalControl* lpControl, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                             _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground,
                             _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawHorizontalLine(_In_ GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth,
                                           _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                           _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawVerticalLine(_In_ GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iHeight,
                                         _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                         _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawBox(_In_ GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Write(_In_ GuiTerminalControl* lpControl, _In_z_ LPCWSTR szTextW);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Print(_In_ GuiTerminalControl* lpControl, _In_z_ LPCWSTR szFormatW, ...);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintV(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szFormatW, _In_ va_list argList);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_SetContext(_In_ GuiTerminalControl *lpControl, _In_opt_ PVOID lpContext);
GUITERMINAL_CONTROL_API
PVOID GuiTerminalControl_GetContext(_In_ const GuiTerminalControl *lpControl);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_CreateRegion(_In_ GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                        _Out_ GuiTerminalRegion* lphRegion, _In_opt_ GuiTerminalRegion hRegionParent);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ClearRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DestroyRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ScrollRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iLineCount);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_FillRegionArea(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                       _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                                       _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_MoveRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iSourceX, _In_ INT iSourceY,
                                   _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW,
                                   _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawRegionHorizontalLine(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX,
                                                 _In_ INT iY, _In_ INT iWidth, _In_ GuiTerminalStrokeType strokeType,
                                                 _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawRegionVerticalLine(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                               _In_ INT iHeight, _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground,
                                               _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawRegionBox(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                      _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                                      _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_WriteRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szTextW);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW, ...);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintRegionV(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW,
                                     _In_ va_list argList);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_RelocateRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                          _In_ INT iWidth, _In_ INT iHeight);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_BringRegionToFront(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_SendRegionToBack(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_MoveRegionAfter(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                           _In_opt_ GuiTerminalRegion hRegionReference);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_SetRegionContext(_In_ GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion,
                                            _In_opt_ PVOID lpContext);
GUITERMINAL_CONTROL_API
PVOID GuiTerminalControl_GetRegionContext(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
GuiTerminalRegion GuiTerminalControl_GetFirstRegion(_In_ const GuiTerminalControl *lpControl);
GUITERMINAL_CONTROL_API
GuiTerminalRegion GuiTerminalControl_GetLastRegion(_In_ const GuiTerminalControl *lpControl);
GUITERMINAL_CONTROL_API
// Returns the next region toward the back within the same sibling list, or the first root child when the handle is null.
GuiTerminalRegion GuiTerminalControl_GetNextRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
// Returns the previous region toward the front within the same sibling list, or the last root child when the handle is null.
GuiTerminalRegion GuiTerminalControl_GetPreviousRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
// Returns the frontmost child of the specified parent, or the frontmost root child when the handle is null.
GuiTerminalRegion GuiTerminalControl_GetChildFirstRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegionParent);
GUITERMINAL_CONTROL_API
// Returns the backmost child of the specified parent, or the backmost root child when the handle is null.
GuiTerminalRegion GuiTerminalControl_GetChildLastRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegionParent);
GUITERMINAL_CONTROL_API
// Returns the immediate parent region, or null when the parent is the terminal root.
GuiTerminalRegion GuiTerminalControl_GetParentRegion(_In_ const GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_GetRegionLocation(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _Out_opt_ LPINT lpiX,
                                          _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth, _Out_opt_ LPINT lpiHeight);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_GetTerminalSize(_In_ const GuiTerminalControl *lpControl, _Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows);
GUITERMINAL_CONTROL_API
// Translate zero-based terminal coordinates to zero-based region coordinates for a specific region.
// Returns FALSE only when the region handle is invalid.
BOOL GuiTerminalControl_ConvertToRegionCoordinates(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                   _In_ INT iColTerminal, _In_ INT iRowTerminal, _Out_opt_ LPINT lpiColRegion,
                                                   _Out_opt_ LPINT lpiRowRegion);
GUITERMINAL_CONTROL_API
// Translate zero-based region coordinates to zero-based terminal coordinates for a specific region.
// Returns FALSE when the region handle is invalid or the translated result cannot fit in INT.
BOOL GuiTerminalControl_ConvertFromRegionCoordinates(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                     _In_ INT iColRegion, _In_ INT iRowRegion, _Out_opt_ LPINT lpiColTerminal,
                                                     _Out_opt_ LPINT lpiRowTerminal);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ShowCursor(_In_ GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_HideCursor(_In_ GuiTerminalControl *lpControl);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_SetCursorStyle(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalCursorStyle cursorStyle);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_ResizeTerminal(_In_ GuiTerminalControl* lpControl, _In_ INT iCols, _In_ INT iRows);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetCellSize(_In_ const GuiTerminalControl* lpControl, _Out_ LPSIZE lpSize);
GUITERMINAL_CONTROL_API
BOOL GuiTerminalControl_GetCellPosition(_In_ const GuiTerminalControl* lpControl, _In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell);
GUITERMINAL_CONTROL_API
BOOL GuiTerminalControl_GetCellFromPosition(_In_ const GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol,
                                            _Out_opt_ LPINT lpiRow);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetPreferredClientSize(_In_ const GuiTerminalControl* lpControl, _Out_ LPSIZE lpSize);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetPreferredWindowSize(_In_ const GuiTerminalControl* lpControl, _Out_ LPSIZE lpSize, _In_opt_ BOOL bHasMenu);

#ifdef __cplusplus
} // extern "C"
#endif
