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
VOID GuiTerminalControl_Write(_In_ GuiTerminalControl* lpControl, _In_z_ LPCWSTR szTextW);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_Print(_In_ GuiTerminalControl* lpControl, _In_z_ LPCWSTR szFormatW, ...);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintV(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szFormatW, _In_ va_list argList);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_CreateRegion(_In_ GuiTerminalControl* lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                        _Out_ GuiTerminalRegion* lphRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_DestroyRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ClearRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_ScrollRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iLineCount);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_FillRegionArea(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                       _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                                       _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_WriteRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szTextW);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintRegion(_In_ GuiTerminalControl* lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW, ...);
GUITERMINAL_CONTROL_API
VOID GuiTerminalControl_PrintRegionV(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW,
                                     _In_ va_list argList);

GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_ResizeTerminal(_In_ GuiTerminalControl* lpControl, _In_ INT iCols, _In_ INT iRows);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetCellSize(_In_ const GuiTerminalControl* lpControl, _Out_ LPSIZE lpSize);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetPreferredClientSize(_In_ const GuiTerminalControl* lpControl, _Out_ LPSIZE lpSize);
GUITERMINAL_CONTROL_API
HRESULT GuiTerminalControl_GetPreferredWindowSize(_In_ const GuiTerminalControl* lpControl, _Out_ LPSIZE lpSize);

#ifdef __cplusplus
} // extern "C"
#endif
