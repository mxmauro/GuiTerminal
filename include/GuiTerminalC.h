#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// -----------------------------------------------------------------------------

#if defined(GUITERMINAL_SHARED)
#if defined(GUITERMINAL_BUILD_DLL)
#define GUITERMINAL_CONTROL_API __declspec(dllexport)
#else
#define GUITERMINAL_CONTROL_API __declspec(dllimport)
#endif
#else
#define GUITERMINAL_CONTROL_API
#endif

// -----------------------------------------------------------------------------

typedef enum GuiTerminalStyleFlags_e {
    GuiTerminalStyleNone = 0U,
    GuiTerminalStyleBold = 1U << 0,
    GuiTerminalStyleUnderline = 1U << 1,
    GuiTerminalStyleBlink = 1U << 2,
    GuiTerminalStyleInverse = 1U << 3,
    GuiTerminalStyleItalic = 1U << 4
} GuiTerminalStyleFlags;

typedef enum GuiTerminalTextAlignment_e {
    GuiTerminalAlignLeft = 0U,
    GuiTerminalAlignCenter = 1U << 0,
    GuiTerminalAlignRight = 2U << 0,
    GuiTerminalAlignTop = 0U,
    GuiTerminalAlignMiddle = 1U << 2,
    GuiTerminalAlignBottom = 2U << 2,
    GuiTerminalAlignBaseline = 3U << 2
} GuiTerminalTextAlignment;

typedef enum GuiTerminalStrokeType_e {
    GuiTerminalStrokeSingleLine = 0U,
    GuiTerminalStrokeDoubleLine,
    GuiTerminalStrokeShadeLight,
    GuiTerminalStrokeShadeMedium,
    GuiTerminalStrokeShadeDark,
    GuiTerminalStrokeSolidBlock
} GuiTerminalStrokeType;

typedef enum GuiTerminalBoxSideFlags_e {
    GuiTerminalBoxSideNone = 0U,
    GuiTerminalBoxSideTopDouble = 1U << 0,
    GuiTerminalBoxSideRightDouble = 1U << 1,
    GuiTerminalBoxSideBottomDouble = 1U << 2,
    GuiTerminalBoxSideLeftDouble = 1U << 3
} GuiTerminalBoxSideFlags;

typedef enum GuiTerminalCursorStyle_e {
    GuiTerminalCursorBlock = 0U,
    GuiTerminalCursorUnderscore,
    GuiTerminalCursorBarLeft
} GuiTerminalCursorStyle;

typedef struct GuiTerminalControlConfig_s {
    INT iRows;
    INT iCols;
    LPCWSTR szFontFamilyW;
    FLOAT fFontSize;
    COLORREF crDefaultForeground;
    COLORREF crDefaultBackground;
} GuiTerminalControlConfig;

typedef struct GuiTerminalControl_s GuiTerminalControl;
typedef struct GuiTerminalRegion_s *GuiTerminalRegion;
typedef struct GuiTerminalDrawContext_s GuiTerminalDrawContext;
typedef struct ID2D1RenderTarget ID2D1RenderTarget;

typedef enum GuiTerminalCustomDrawResourceCleanupReason_e {
    GuiTerminalCustomDrawResourceCleanupTargetLost = 0U,
    GuiTerminalCustomDrawResourceCleanupRegionDestroyed
} GuiTerminalCustomDrawResourceCleanupReason;

typedef enum GuiTerminalPathSweepDirection_e {
    GuiTerminalPathSweepClockwise = 0,
    GuiTerminalPathSweepCounterClockwise = 1
} GuiTerminalPathSweepDirection;

typedef enum GuiTerminalPathArcSize_e {
    GuiTerminalPathArcSmall = 0,
    GuiTerminalPathArcLarge = 1
} GuiTerminalPathArcSize;

typedef VOID (*GuiTerminalCustomDrawCallback)(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ GuiTerminalRegion hRegion,
                                              _In_opt_ PVOID lpDrawState);
typedef VOID (*GuiTerminalRegionDestroyCallback)(_In_ GuiTerminalRegion hRegion, _In_opt_ PVOID lpCallbackContext);
typedef VOID (*GuiTerminalCustomDrawResourceCleanupCallback)(_In_opt_ PVOID lpDrawState,
                                                             _In_ GuiTerminalCustomDrawResourceCleanupReason cleanupReason);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_Create(_In_ HWND hWnd, _In_ const GuiTerminalControlConfig *lpConfig, _Out_ GuiTerminalControl **lplpControl);

GUITERMINAL_CONTROL_API
BOOL GuiTerminalControl_WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT *lplResult);

GUITERMINAL_CONTROL_API
GuiTerminalControl *GuiTerminalControl_GetFromWindow(_In_ HWND hWnd);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Clear(_In_ GuiTerminalControl *lpControl);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Scroll(_In_ GuiTerminalControl *lpControl, _In_ INT iLineCount);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Move(_In_ GuiTerminalControl *lpControl, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                             _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground,
                             _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Fill(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                             _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawHorizontalLine(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth,
                                           _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                           _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawVerticalLine(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iHeight,
                                         _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                         _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawBox(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Write(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szTextW);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Print(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szFormatW, ...);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintV(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szFormatW, _In_ va_list argList);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_SetContext(_In_ GuiTerminalControl *lpControl, _In_opt_ PVOID lpContext);

GUITERMINAL_CONTROL_API
PVOID GuiTerminalControl_GetContext(_In_ const GuiTerminalControl *lpControl);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_CreateRegion(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                        _Out_ GuiTerminalRegion *lphRegion, _In_opt_ GuiTerminalRegion hRegionParent);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_CreateCustomDrawRegion(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth,
                                                  _In_ INT iHeight, _Out_ GuiTerminalRegion *lphRegion,
                                                  _In_ GuiTerminalCustomDrawCallback lpDrawCallback, _In_opt_ PVOID lpDrawState,
                                                  _In_opt_ GuiTerminalRegion hRegionParent);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DestroyRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ClearRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_InvalidateRegion(_In_ GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ScrollRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iLineCount);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_MoveRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iSourceX, _In_ INT iSourceY,
                                   _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW,
                                   _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_FillRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                   _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                                   _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawRegionHorizontalLine(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX,
                                                 _In_ INT iY, _In_ INT iWidth, _In_ GuiTerminalStrokeType strokeType,
                                                 _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawRegionVerticalLine(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                               _In_ INT iHeight, _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground,
                                               _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DrawRegionBox(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                      _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                                      _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_WriteRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szTextW);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW, ...);

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
HRESULT GuiTerminalControl_SetRegionDestroyCallback(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                    _In_opt_ GuiTerminalRegionDestroyCallback lpCallback, _In_opt_ PVOID lpCallbackContext);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_SetCustomDrawRegionResourceCleanup(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                              _In_opt_ GuiTerminalCustomDrawResourceCleanupCallback lpCallback);

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
GuiTerminalRegion GuiTerminalControl_GetChildFirstRegion(_In_ const GuiTerminalControl *lpControl,
                                                         _In_opt_ GuiTerminalRegion hRegionParent);

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
HRESULT GuiTerminalControl_ResizeTerminal(_In_ GuiTerminalControl *lpControl, _In_ INT iCols, _In_ INT iRows);

GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_GetTerminalSize(_In_ const GuiTerminalControl *lpControl, _Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetPreferredClientSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetPreferredWindowSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize, _In_opt_ BOOL bHasMenu);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetCellSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize);

GUITERMINAL_CONTROL_API
BOOL GuiTerminalControl_GetCellPosition(_In_ const GuiTerminalControl *lpControl, _In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell);

GUITERMINAL_CONTROL_API
BOOL GuiTerminalControl_GetCellFromPosition(_In_ const GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol,
                                            _Out_opt_ LPINT lpiRow);

GUITERMINAL_CONTROL_API
VOID GuiTerminalDrawContext_Clear(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ COLORREF crColor);

GUITERMINAL_CONTROL_API
VOID GuiTerminalDrawContext_DrawLine(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2,
                                     _In_ FLOAT fY2, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth);

GUITERMINAL_CONTROL_API
VOID GuiTerminalDrawContext_DrawPoint(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor,
                                      _In_ FLOAT fRadius);

GUITERMINAL_CONTROL_API
VOID GuiTerminalDrawContext_FillRectangle(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fLeft, _In_ FLOAT fTop, _In_ FLOAT fRight,
                                          _In_ FLOAT fBottom, _In_ COLORREF crColor);

GUITERMINAL_CONTROL_API
VOID GuiTerminalDrawContext_DrawRectangle(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fLeft, _In_ FLOAT fTop, _In_ FLOAT fRight,
                                          _In_ FLOAT fBottom, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth);

GUITERMINAL_CONTROL_API
VOID GuiTerminalDrawContext_Write(_In_ GuiTerminalDrawContext *lpDrawContext, _In_z_ LPCWSTR szTextW, _In_ FLOAT fX, _In_ FLOAT fY,
                                  _In_ COLORREF crColor, _In_opt_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize, _In_ DWORD dwStyleFlags,
                                  _In_ GuiTerminalTextAlignment textAlignment, _In_ FLOAT fRotationDegrees);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_BeginPath(_In_ GuiTerminalDrawContext *lpDrawContext);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_MoveTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX, _In_ FLOAT fY);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_LineTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX, _In_ FLOAT fY);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_QuadraticBezierTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fControlX,
                                                                      _In_ FLOAT fControlY, _In_ FLOAT fEndX, _In_ FLOAT fEndY);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_CubicBezierTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fControl1X,
                                                                  _In_ FLOAT fControl1Y, _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y,
                                                                  _In_ FLOAT fEndX, _In_ FLOAT fEndY);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_ArcTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fEndX, _In_ FLOAT fEndY,
                                                          _In_ FLOAT fRadiusX, _In_ FLOAT fRadiusY, _In_ FLOAT fRotationDegrees,
                                                          _In_ GuiTerminalPathSweepDirection sweepDirection,
                                                          _In_ GuiTerminalPathArcSize arcSize);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_ClosePath(_In_ GuiTerminalDrawContext *lpDrawContext);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_StrokePath(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ COLORREF crColor,
                                                               _In_ FLOAT fStrokeWidth);

GUITERMINAL_CONTROL_API VOID GuiTerminalDrawContext_FillPath(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ COLORREF crColor);

GUITERMINAL_CONTROL_API
INT GuiTerminalDrawContext_GetWidth(_In_ const GuiTerminalDrawContext *lpDrawContext);

GUITERMINAL_CONTROL_API
INT GuiTerminalDrawContext_GetHeight(_In_ const GuiTerminalDrawContext *lpDrawContext);

GUITERMINAL_CONTROL_API
UINT GuiTerminalDrawContext_GetDeviceGeneration(_In_ const GuiTerminalDrawContext *lpDrawContext);

GUITERMINAL_CONTROL_API
ID2D1RenderTarget *GuiTerminalDrawContext_GetDirect2DRenderTarget(_In_ const GuiTerminalDrawContext *lpDrawContext);

#ifdef __cplusplus
} // extern "C"
#endif
