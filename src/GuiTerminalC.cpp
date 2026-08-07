#include "..\include\GuiTerminalC.h"
#include "..\include\GuiTerminalControl.h"

// -----------------------------------------------------------------------------

static GuiTerminal::Control *ToCppControl(GuiTerminalControl *lpControl) noexcept;
static const GuiTerminal::Control *ToCppControl(const GuiTerminalControl *lpControl) noexcept;
static GuiTerminal::RegionHandle ToCppRegion(GuiTerminalRegion hRegion) noexcept;
static GuiTerminalRegion ToCRegion(GuiTerminal::RegionHandle hRegion) noexcept;
static GuiTerminal::Control::Config ToCppConfig(const GuiTerminalControlConfig &sConfig) noexcept;

// -----------------------------------------------------------------------------

extern "C"
{

HRESULT GuiTerminalControl_Create(_In_ HWND hWnd, _In_ const GuiTerminalControlConfig *lpConfig, _Out_ GuiTerminalControl **lplpControl)
{
    GuiTerminal::Control *lpCreatedControl;
    HRESULT hr;

    if (!lplpControl)
    {
        return E_POINTER;
    }
    *lplpControl = nullptr;
    if (!lpConfig)
    {
        return E_POINTER;
    }

    lpCreatedControl = nullptr;
    const GuiTerminal::Control::Config sCppConfig = ToCppConfig(*lpConfig);
    hr = GuiTerminal::Control::Create(hWnd, sCppConfig, &lpCreatedControl);
    if (SUCCEEDED(hr))
    {
        *lplpControl = reinterpret_cast<GuiTerminalControl *>(lpCreatedControl);
    }
    return hr;
}

BOOL GuiTerminalControl_WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT *lplResult)
{
    if (!lplResult)
    {
        return FALSE;
    }

    return GuiTerminal::Control::WndProc(hWnd, uMessage, wParam, lParam, lplResult);
}

GuiTerminalControl *GuiTerminalControl_GetFromWindow(_In_ HWND hWnd)
{
    return reinterpret_cast<GuiTerminalControl *>(GuiTerminal::Control::GetControl(hWnd));
}

VOID GuiTerminalControl_Clear(_In_ GuiTerminalControl *lpControl)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->Clear();
    }
}

VOID GuiTerminalControl_Scroll(_In_ GuiTerminalControl *lpControl, _In_ INT iLineCount)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->Scroll(iLineCount);
    }
}

VOID GuiTerminalControl_Move(_In_ GuiTerminalControl *lpControl, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                             _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground,
                             _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->Move(iSourceX, iSourceY, iWidth, iHeight, iTargetX, iTargetY, chFillW, crForeground, crBackground,
                                      dwStyleFlags);
    }
}

VOID GuiTerminalControl_Fill(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                             _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->Fill(iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_DrawHorizontalLine(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth,
                                           _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                           _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DrawHorizontalLine(iX, iY, iWidth, static_cast<GuiTerminal::Control::StrokeType>(strokeType), crForeground,
                                                    crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_DrawVerticalLine(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iHeight,
                                         _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                         _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DrawVerticalLine(iX, iY, iHeight, static_cast<GuiTerminal::Control::StrokeType>(strokeType), crForeground,
                                                  crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_DrawBox(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DrawBox(iX, iY, iWidth, iHeight, dwBoxSideFlags, crForeground, crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_Write(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szTextW)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->Write(szTextW);
    }
}

VOID GuiTerminalControl_Print(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szFormatW, ...)
{
    va_list argList;

    if (lpControl && szFormatW)
    {
        va_start(argList, szFormatW);
        ToCppControl(lpControl)->PrintV(szFormatW, argList);
        va_end(argList);
    }
}

VOID GuiTerminalControl_PrintV(_In_ GuiTerminalControl *lpControl, _In_z_ LPCWSTR szFormatW, _In_ va_list argList)
{
    if (lpControl && szFormatW)
    {
        ToCppControl(lpControl)->PrintV(szFormatW, argList);
    }
}

HRESULT GuiTerminalControl_SetContext(_In_ GuiTerminalControl *lpControl, _In_opt_ PVOID lpContext)
{
    if (!lpControl)
    {
        return E_POINTER;
    }
    return ToCppControl(lpControl)->SetContext(lpContext);
}

PVOID GuiTerminalControl_GetContext(_In_ const GuiTerminalControl *lpControl)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCppControl(lpControl)->GetContext();
}

HRESULT GuiTerminalControl_CreateRegion(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                        _Out_ GuiTerminalRegion *lphRegion, _In_opt_ GuiTerminalRegion hRegionParent)
{
    GuiTerminal::RegionHandle hRegion;
    HRESULT hr;

    if (!lphRegion)
    {
        return E_POINTER;
    }
    *lphRegion = nullptr;
    if (!lpControl)
    {
        return E_POINTER;
    }

    hRegion = nullptr;
    hr = ToCppControl(lpControl)->CreateRegion(iX, iY, iWidth, iHeight, &hRegion, ToCppRegion(hRegionParent));
    if (SUCCEEDED(hr))
    {
        *lphRegion = ToCRegion(hRegion);
    }
    return hr;
}

HRESULT GuiTerminalControl_CreateCustomDrawRegion(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth,
                                                  _In_ INT iHeight, _Out_ GuiTerminalRegion *lphRegion,
                                                  _In_ GuiTerminalCustomDrawCallback lpDrawCallback, _In_opt_ PVOID lpDrawState,
                                                  _In_opt_ GuiTerminalRegion hRegionParent)
{
    GuiTerminal::CustomDrawCallback fnDrawCallback;
    GuiTerminal::RegionHandle hRegion;
    HRESULT hr;

    if (!lphRegion)
    {
        return E_POINTER;
    }
    *lphRegion = nullptr;
    if ((!lpControl) || (!lpDrawCallback))
    {
        return E_INVALIDARG;
    }
    try
    {
        fnDrawCallback = [lpDrawCallback, lpDrawState](_In_ GuiTerminal::DrawContext &drawContext,
                                                       _In_ GuiTerminal::RegionHandle hRegionCallback) noexcept {
            lpDrawCallback(reinterpret_cast<GuiTerminalDrawContext *>(&drawContext), ToCRegion(hRegionCallback), lpDrawState);
        };
    }
    catch (const std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }
    hRegion = nullptr;
    hr = ToCppControl(lpControl)->CreateCustomDrawRegion(iX, iY, iWidth, iHeight, &hRegion, fnDrawCallback, ToCppRegion(hRegionParent));
    if (SUCCEEDED(hr))
    {
        hRegion->lpCAdapterDrawState = lpDrawState;
        *lphRegion = ToCRegion(hRegion);
    }
    return hr;
}

VOID GuiTerminalControl_DestroyRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DestroyRegion(ToCppRegion(hRegion));
    }
}

VOID GuiTerminalControl_ClearRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->ClearRegion(ToCppRegion(hRegion));
    }
}

VOID GuiTerminalControl_InvalidateRegion(_In_ GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->InvalidateRegion(ToCppRegion(hRegion));
    }
}

VOID GuiTerminalControl_ScrollRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iLineCount)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->ScrollRegion(ToCppRegion(hRegion), iLineCount);
    }
}

VOID GuiTerminalControl_MoveRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iSourceX, _In_ INT iSourceY,
                                   _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW,
                                   _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->MoveRegion(ToCppRegion(hRegion), iSourceX, iSourceY, iWidth, iHeight, iTargetX, iTargetY, chFillW,
                                            crForeground, crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_FillRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                   _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                                   _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->FillRegion(ToCppRegion(hRegion), iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground,
                                            dwStyleFlags);
    }
}

VOID GuiTerminalControl_DrawRegionHorizontalLine(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX,
                                                 _In_ INT iY, _In_ INT iWidth, _In_ GuiTerminalStrokeType strokeType,
                                                 _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DrawRegionHorizontalLine(ToCppRegion(hRegion), iX, iY, iWidth,
                                                          static_cast<GuiTerminal::Control::StrokeType>(strokeType), crForeground,
                                                          crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_DrawRegionVerticalLine(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                               _In_ INT iHeight, _In_ GuiTerminalStrokeType strokeType, _In_ COLORREF crForeground,
                                               _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DrawRegionVerticalLine(ToCppRegion(hRegion), iX, iY, iHeight,
                                                        static_cast<GuiTerminal::Control::StrokeType>(strokeType), crForeground,
                                                        crBackground, dwStyleFlags);
    }
}

VOID GuiTerminalControl_DrawRegionBox(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                      _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                                      _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DrawRegionBox(ToCppRegion(hRegion), iX, iY, iWidth, iHeight, dwBoxSideFlags, crForeground, crBackground,
                                               dwStyleFlags);
    }
}

VOID GuiTerminalControl_WriteRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szTextW)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->WriteRegion(ToCppRegion(hRegion), szTextW);
    }
}

VOID GuiTerminalControl_PrintRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW, ...)
{
    va_list argList;

    if (lpControl && szFormatW)
    {
        va_start(argList, szFormatW);
        ToCppControl(lpControl)->PrintRegionV(ToCppRegion(hRegion), szFormatW, argList);
        va_end(argList);
    }
}

VOID GuiTerminalControl_PrintRegionV(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_z_ LPCWSTR szFormatW,
                                     _In_ va_list argList)
{
    if (lpControl && szFormatW)
    {
        ToCppControl(lpControl)->PrintRegionV(ToCppRegion(hRegion), szFormatW, argList);
    }
}

HRESULT GuiTerminalControl_RelocateRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                          _In_ INT iWidth, _In_ INT iHeight)
{
    if (!lpControl)
    {
        return E_POINTER;
    }
    return ToCppControl(lpControl)->RelocateRegion(ToCppRegion(hRegion), iX, iY, iWidth, iHeight);
}

HRESULT GuiTerminalControl_BringRegionToFront(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if (!lpControl)
    {
        return E_POINTER;
    }
    return ToCppControl(lpControl)->BringRegionToFront(ToCppRegion(hRegion));
}

HRESULT GuiTerminalControl_SendRegionToBack(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if (!lpControl)
    {
        return E_POINTER;
    }
    return ToCppControl(lpControl)->SendRegionToBack(ToCppRegion(hRegion));
}

HRESULT GuiTerminalControl_MoveRegionAfter(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                           _In_opt_ GuiTerminalRegion hRegionReference)
{
    if (!lpControl)
    {
        return E_POINTER;
    }
    return ToCppControl(lpControl)->MoveRegionAfter(ToCppRegion(hRegion), ToCppRegion(hRegionReference));
}

HRESULT GuiTerminalControl_SetRegionContext(_In_ GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion,
                                            _In_opt_ PVOID lpContext)
{
    if (!lpControl)
    {
        return E_POINTER;
    }
    return ToCppControl(lpControl)->SetRegionContext(ToCppRegion(hRegion), lpContext);
}

PVOID GuiTerminalControl_GetRegionContext(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCppControl(lpControl)->GetRegionContext(ToCppRegion(hRegion));
}

HRESULT GuiTerminalControl_SetRegionDestroyCallback(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                    _In_opt_ GuiTerminalRegionDestroyCallback lpCallback, _In_opt_ PVOID lpCallbackContext)
{
    GuiTerminal::RegionDestroyCallback fnCallback;

    if (!lpControl)
    {
        return E_POINTER;
    }
    if (lpCallback)
    {
        try
        {
            fnCallback = [lpCallback, lpCallbackContext](_In_ GuiTerminal::RegionHandle hRegionCallback) noexcept {
                lpCallback(ToCRegion(hRegionCallback), lpCallbackContext);
            };
        }
        catch (const std::bad_alloc &)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_UNEXPECTED;
        }
    }
    return ToCppControl(lpControl)->SetRegionDestroyCallback(ToCppRegion(hRegion), fnCallback);
}

HRESULT GuiTerminalControl_SetCustomDrawRegionResourceCleanup(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                              _In_opt_ GuiTerminalCustomDrawResourceCleanupCallback lpCallback)
{
    GuiTerminal::CustomDrawResourceCleanupCallback fnCallback;
    GuiTerminal::RegionHandle hRegionCpp;
    PVOID lpDrawState;

    if (!lpControl)
    {
        return E_POINTER;
    }
    hRegionCpp = ToCppRegion(hRegion);
    if (!hRegionCpp)
    {
        return E_INVALIDARG;
    }
    if (lpCallback)
    {
        lpDrawState = hRegionCpp->lpCAdapterDrawState;
        try
        {
            fnCallback = [lpCallback, lpDrawState](_In_ GuiTerminal::CustomDrawResourceCleanupReason cleanupReason) noexcept {
                lpCallback(lpDrawState, static_cast<GuiTerminalCustomDrawResourceCleanupReason>(cleanupReason));
            };
        }
        catch (const std::bad_alloc &)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_UNEXPECTED;
        }
    }
    return ToCppControl(lpControl)->SetCustomDrawRegionResourceCleanup(hRegionCpp, fnCallback);
}

GuiTerminalRegion GuiTerminalControl_GetFirstRegion(_In_ const GuiTerminalControl *lpControl)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetFirstRegion());
}

GuiTerminalRegion GuiTerminalControl_GetLastRegion(_In_ const GuiTerminalControl *lpControl)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetLastRegion());
}

GuiTerminalRegion GuiTerminalControl_GetNextRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetNextRegion(ToCppRegion(hRegion)));
}

GuiTerminalRegion GuiTerminalControl_GetPreviousRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetPreviousRegion(ToCppRegion(hRegion)));
}

GuiTerminalRegion GuiTerminalControl_GetChildFirstRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegionParent)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetChildFirstRegion(ToCppRegion(hRegionParent)));
}

GuiTerminalRegion GuiTerminalControl_GetChildLastRegion(_In_ const GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegionParent)
{
    if (!lpControl)
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetChildLastRegion(ToCppRegion(hRegionParent)));
}

GuiTerminalRegion GuiTerminalControl_GetParentRegion(_In_ const GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if ((!lpControl) || (!hRegion))
    {
        return nullptr;
    }
    return ToCRegion(ToCppControl(lpControl)->GetParentRegion(ToCppRegion(hRegion)));
}

VOID GuiTerminalControl_GetRegionLocation(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _Out_opt_ LPINT lpiX,
                                          _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth, _Out_opt_ LPINT lpiHeight)
{
    if (lpControl && hRegion)
    {
        ToCppControl(lpControl)->GetRegionLocation(ToCppRegion(hRegion), lpiX, lpiY, lpiWidth, lpiHeight);
    }
    else
    {
        if (lpiX)
        {
            *lpiX = 0;
        }
        if (lpiY)
        {
            *lpiY = 0;
        }
        if (lpiWidth)
        {
            *lpiWidth = 0;
        }
        if (lpiHeight)
        {
            *lpiHeight = 0;
        }
    }
}

BOOL GuiTerminalControl_ConvertToRegionCoordinates(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                   _In_ INT iColTerminal, _In_ INT iRowTerminal, _Out_opt_ LPINT lpiColRegion,
                                                   _Out_opt_ LPINT lpiRowRegion)
{
    if ((!lpControl) || (!hRegion))
    {
        if (lpiColRegion)
        {
            *lpiColRegion = 0;
        }
        if (lpiRowRegion)
        {
            *lpiRowRegion = 0;
        }
        return FALSE;
    }

    return ToCppControl(lpControl)->ConvertToRegionCoordinates(ToCppRegion(hRegion), iColTerminal, iRowTerminal, lpiColRegion,
                                                               lpiRowRegion);
}

BOOL GuiTerminalControl_ConvertFromRegionCoordinates(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion,
                                                     _In_ INT iColRegion, _In_ INT iRowRegion, _Out_opt_ LPINT lpiColTerminal,
                                                     _Out_opt_ LPINT lpiRowTerminal)
{
    if ((!lpControl) || (!hRegion))
    {
        if (lpiColTerminal)
        {
            *lpiColTerminal = 0;
        }
        if (lpiRowTerminal)
        {
            *lpiRowTerminal = 0;
        }
        return FALSE;
    }

    return ToCppControl(lpControl)->ConvertFromRegionCoordinates(ToCppRegion(hRegion), iColRegion, iRowRegion, lpiColTerminal,
                                                                 lpiRowTerminal);
}

VOID GuiTerminalControl_ShowCursor(_In_ GuiTerminalControl *lpControl, _In_opt_ GuiTerminalRegion hRegion)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->ShowCursor(ToCppRegion(hRegion));
    }
}

VOID GuiTerminalControl_HideCursor(_In_ GuiTerminalControl *lpControl)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->HideCursor();
    }
}

VOID GuiTerminalControl_SetCursorStyle(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalCursorStyle cursorStyle)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->SetCursorStyle(static_cast<GuiTerminal::Control::CursorStyle>(cursorStyle));
    }
}

HRESULT GuiTerminalControl_ResizeTerminal(_In_ GuiTerminalControl *lpControl, _In_ INT iCols, _In_ INT iRows)
{
    if (!lpControl)
    {
        return E_POINTER;
    }

    return ToCppControl(lpControl)->ResizeTerminal(iCols, iRows);
}

VOID GuiTerminalControl_GetTerminalSize(_In_ const GuiTerminalControl *lpControl, _Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->GetTerminalSize(lpiCols, lpiRows);
    }
    else
    {
        if (lpiCols)
        {
            *lpiCols = 0;
        }
        if (lpiRows)
        {
            *lpiRows = 0;
        }
    }
}

HRESULT GuiTerminalControl_GetPreferredClientSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize)
{
    if (!lpSize)
    {
        return E_POINTER;
    }
    lpSize->cx = lpSize->cy = 0;
    if (!lpControl)
    {
        return E_POINTER;
    }

    return ToCppControl(lpControl)->GetPreferredClientSize(lpSize);
}

HRESULT GuiTerminalControl_GetPreferredWindowSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize, _In_opt_ BOOL bHasMenu)
{
    if (!lpSize)
    {
        return E_POINTER;
    }
    lpSize->cx = lpSize->cy = 0;
    if (!lpControl)
    {
        return E_POINTER;
    }

    return ToCppControl(lpControl)->GetPreferredWindowSize(lpSize, bHasMenu);
}

HRESULT GuiTerminalControl_GetCellSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize)
{
    if (!lpSize)
    {
        return E_POINTER;
    }
    lpSize->cx = lpSize->cy = 0;
    if (!lpControl)
    {
        return E_POINTER;
    }

    return ToCppControl(lpControl)->GetCellSize(lpSize);
}

BOOL GuiTerminalControl_GetCellPosition(_In_ const GuiTerminalControl *lpControl, _In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell)
{
    if (!lprcCell)
    {
        return FALSE;
    }
    lprcCell->left = 0;
    lprcCell->top = 0;
    lprcCell->right = 0;
    lprcCell->bottom = 0;
    if (!lpControl)
    {
        return FALSE;
    }

    return ToCppControl(lpControl)->GetCellPosition(iCol, iRow, lprcCell);
}

BOOL GuiTerminalControl_GetCellFromPosition(_In_ const GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol,
                                            _Out_opt_ LPINT lpiRow)
{
    if (lpiCol)
    {
        *lpiCol = 0;
    }
    if (lpiRow)
    {
        *lpiRow = 0;
    }
    if (!lpControl)
    {
        return FALSE;
    }

    return ToCppControl(lpControl)->GetCellFromPosition(iX, iY, lpiCol, lpiRow);
}

VOID GuiTerminalDrawContext_Clear(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ COLORREF crColor)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->Clear(crColor);
    }
}

VOID GuiTerminalDrawContext_DrawLine(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2,
                                     _In_ FLOAT fY2, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->DrawLine(fX1, fY1, fX2, fY2, crColor, fStrokeWidth);
    }
}

VOID GuiTerminalDrawContext_DrawPoint(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor,
                                      _In_ FLOAT fRadius)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->DrawPoint(fX, fY, crColor, fRadius);
    }
}

VOID GuiTerminalDrawContext_FillRectangle(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fLeft, _In_ FLOAT fTop, _In_ FLOAT fRight,
                                          _In_ FLOAT fBottom, _In_ COLORREF crColor)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->FillRectangle(D2D1::RectF(fLeft, fTop, fRight, fBottom), crColor);
    }
}

VOID GuiTerminalDrawContext_DrawRectangle(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fLeft, _In_ FLOAT fTop, _In_ FLOAT fRight,
                                          _In_ FLOAT fBottom, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)
            ->DrawRectangle(D2D1::RectF(fLeft, fTop, fRight, fBottom), crColor, fStrokeWidth);
    }
}

VOID GuiTerminalDrawContext_Write(_In_ GuiTerminalDrawContext *lpDrawContext, _In_z_ LPCWSTR szTextW, _In_ FLOAT fX, _In_ FLOAT fY,
                                  _In_ COLORREF crColor, _In_opt_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize, _In_ DWORD dwStyleFlags,
                                  _In_ GuiTerminalTextAlignment textAlignment, _In_ FLOAT fRotationDegrees)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)
            ->Write(szTextW, D2D1::Point2F(fX, fY), crColor, szFontFamilyW, fFontSize, dwStyleFlags,
                    static_cast<GuiTerminal::DrawContext::TextAlignment>(textAlignment), fRotationDegrees);
    }
}

VOID GuiTerminalDrawContext_BeginPath(_In_ GuiTerminalDrawContext *lpDrawContext)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->BeginPath();
    }
}

VOID GuiTerminalDrawContext_MoveTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX, _In_ FLOAT fY)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->MoveTo(fX, fY);
    }
}

VOID GuiTerminalDrawContext_LineTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fX, _In_ FLOAT fY)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->LineTo(fX, fY);
    }
}

VOID GuiTerminalDrawContext_QuadraticBezierTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fControlX, _In_ FLOAT fControlY,
                                              _In_ FLOAT fEndX, _In_ FLOAT fEndY)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->QuadraticBezierTo(fControlX, fControlY, fEndX, fEndY);
    }
}

VOID GuiTerminalDrawContext_CubicBezierTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fControl1X, _In_ FLOAT fControl1Y,
                                          _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y, _In_ FLOAT fEndX, _In_ FLOAT fEndY)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)
            ->CubicBezierTo(fControl1X, fControl1Y, fControl2X, fControl2Y, fEndX, fEndY);
    }
}

VOID GuiTerminalDrawContext_ArcTo(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ FLOAT fEndX, _In_ FLOAT fEndY, _In_ FLOAT fRadiusX,
                                  _In_ FLOAT fRadiusY, _In_ FLOAT fRotationDegrees, _In_ GuiTerminalPathSweepDirection sweepDirection,
                                  _In_ GuiTerminalPathArcSize arcSize)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)
            ->ArcTo(fEndX, fEndY, fRadiusX, fRadiusY, fRotationDegrees, static_cast<D2D1_SWEEP_DIRECTION>(sweepDirection),
                    static_cast<D2D1_ARC_SIZE>(arcSize));
    }
}

VOID GuiTerminalDrawContext_ClosePath(_In_ GuiTerminalDrawContext *lpDrawContext)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->ClosePath();
    }
}

VOID GuiTerminalDrawContext_StrokePath(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->StrokePath(crColor, fStrokeWidth);
    }
}

VOID GuiTerminalDrawContext_FillPath(_In_ GuiTerminalDrawContext *lpDrawContext, _In_ COLORREF crColor)
{
    if (lpDrawContext)
    {
        reinterpret_cast<GuiTerminal::DrawContext *>(lpDrawContext)->FillPath(crColor);
    }
}

INT GuiTerminalDrawContext_GetWidth(_In_ const GuiTerminalDrawContext *lpDrawContext)
{
    return lpDrawContext ? reinterpret_cast<const GuiTerminal::DrawContext *>(lpDrawContext)->GetWidth() : 0;
}

INT GuiTerminalDrawContext_GetHeight(_In_ const GuiTerminalDrawContext *lpDrawContext)
{
    return lpDrawContext ? reinterpret_cast<const GuiTerminal::DrawContext *>(lpDrawContext)->GetHeight() : 0;
}

UINT GuiTerminalDrawContext_GetDeviceGeneration(_In_ const GuiTerminalDrawContext *lpDrawContext)
{
    return lpDrawContext ? reinterpret_cast<const GuiTerminal::DrawContext *>(lpDrawContext)->GetDeviceGeneration() : 0U;
}

ID2D1RenderTarget *GuiTerminalDrawContext_GetDirect2DRenderTarget(_In_ const GuiTerminalDrawContext *lpDrawContext)
{
    return lpDrawContext ? reinterpret_cast<const GuiTerminal::DrawContext *>(lpDrawContext)->GetDirect2DRenderTarget() : nullptr;
}

} // extern "C"

// -----------------------------------------------------------------------------

static GuiTerminal::Control *ToCppControl(GuiTerminalControl *lpControl) noexcept
{
    return reinterpret_cast<GuiTerminal::Control *>(lpControl);
}

static const GuiTerminal::Control *ToCppControl(const GuiTerminalControl *lpControl) noexcept
{
    return reinterpret_cast<const GuiTerminal::Control *>(lpControl);
}

static GuiTerminal::RegionHandle ToCppRegion(GuiTerminalRegion hRegion) noexcept
{
    return reinterpret_cast<GuiTerminal::RegionHandle>(hRegion);
}

static GuiTerminalRegion ToCRegion(GuiTerminal::RegionHandle hRegion) noexcept
{
    return reinterpret_cast<GuiTerminalRegion>(hRegion);
}

static GuiTerminal::Control::Config ToCppConfig(const GuiTerminalControlConfig &sConfig) noexcept
{
    GuiTerminal::Control::Config sCppConfig;

    sCppConfig.iRows = sConfig.iRows;
    sCppConfig.iCols = sConfig.iCols;
    sCppConfig.szFontFamilyW = sConfig.szFontFamilyW;
    sCppConfig.fFontSize = sConfig.fFontSize;
    sCppConfig.crDefaultForeground = sConfig.crDefaultForeground;
    sCppConfig.crDefaultBackground = sConfig.crDefaultBackground;
    return sCppConfig;
}
