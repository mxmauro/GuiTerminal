#include "..\include\GuiTerminalC.h"
#include "..\include\GuiTerminalControl.h"

// -----------------------------------------------------------------------------

static GuiTerminal::Control *ToCppControl(GuiTerminalControl *lpControl) noexcept;
static const GuiTerminal::Control *ToCppControl(const GuiTerminalControl *lpControl) noexcept;
static GuiTerminal::RegionHandle ToCppRegion(GuiTerminalRegion hRegion) noexcept;
static GuiTerminalRegion ToCRegion(GuiTerminal::RegionHandle hRegion) noexcept;
static GuiTerminal::Control::Config ToCppConfig(const GuiTerminalControlConfig &sConfig) noexcept;

// -----------------------------------------------------------------------------

extern "C" {

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

GuiTerminalControl* GuiTerminalControl_GetFromWindow(_In_ HWND hWnd)
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

VOID GuiTerminalControl_FillArea(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                 _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->FillArea(iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
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

HRESULT GuiTerminalControl_CreateRegion(_In_ GuiTerminalControl *lpControl, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                        _Out_ GuiTerminalRegion *lphRegion)
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
    hr = ToCppControl(lpControl)->CreateRegion(iX, iY, iWidth, iHeight, &hRegion);
    if (SUCCEEDED(hr))
    {
        *lphRegion = ToCRegion(hRegion);
    }
    return hr;
}

VOID GuiTerminalControl_ClearRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->ClearRegion(ToCppRegion(hRegion));
    }
}

VOID GuiTerminalControl_DestroyRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->DestroyRegion(ToCppRegion(hRegion));
    }
}

VOID GuiTerminalControl_ScrollRegion(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iLineCount)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->ScrollRegion(ToCppRegion(hRegion), iLineCount);
    }
}

VOID GuiTerminalControl_FillRegionArea(_In_ GuiTerminalControl *lpControl, _In_ GuiTerminalRegion hRegion, _In_ INT iX, _In_ INT iY,
                                       _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                                       _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags)
{
    if (lpControl)
    {
        ToCppControl(lpControl)->FillRegionArea(ToCppRegion(hRegion), iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground,
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

VOID GuiTerminalControl_GetRegionLocation(_In_ GuiTerminalControl *lpControl, _In_opt_  GuiTerminalRegion hRegion, _Out_opt_ LPINT lpiX,
                                          _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth, _Out_opt_ LPINT lpiHeight)
{
    if (lpControl)
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


HRESULT GuiTerminalControl_ResizeTerminal(_In_ GuiTerminalControl *lpControl, _In_ INT iCols, _In_ INT iRows)
{
    if (!lpControl)
    {
        return E_POINTER;
    }

    return ToCppControl(lpControl)->ResizeTerminal(iCols, iRows);
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

HRESULT GuiTerminalControl_GetPreferredWindowSize(_In_ const GuiTerminalControl *lpControl, _Out_ LPSIZE lpSize)
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

    return ToCppControl(lpControl)->GetPreferredWindowSize(lpSize);
}

} // extern "C"

// -----------------------------------------------------------------------------

static GuiTerminal::Control* ToCppControl(GuiTerminalControl* lpControl) noexcept
{
    return reinterpret_cast<GuiTerminal::Control*>(lpControl);
}

static const GuiTerminal::Control* ToCppControl(const GuiTerminalControl* lpControl) noexcept
{
    return reinterpret_cast<const GuiTerminal::Control*>(lpControl);
}

static GuiTerminal::RegionHandle ToCppRegion(GuiTerminalRegion hRegion) noexcept
{
    return reinterpret_cast<GuiTerminal::RegionHandle>(hRegion);
}

static GuiTerminalRegion ToCRegion(GuiTerminal::RegionHandle hRegion) noexcept
{
    return reinterpret_cast<GuiTerminalRegion>(hRegion);
}

static GuiTerminal::Control::Config ToCppConfig(const GuiTerminalControlConfig& sConfig) noexcept
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
