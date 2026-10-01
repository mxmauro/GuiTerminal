#include "..\include\GuiTerminalControl.h"
#include "..\include\GuiTerminalParser.h"
#include <cstdio>
#include <string>
#include <windowsx.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// -----------------------------------------------------------------------------

namespace GuiTerminal {

std::mutex Control::m_mapMutex;
std::unordered_map<HWND, Control *> Control::m_mapControls;

} // namespace GuiTerminal

// -----------------------------------------------------------------------------

static HRESULT FormatWideStringV(_In_z_ LPCWSTR pszFormatW, _In_ va_list argList, _Out_ std::wstring &strTextW) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal {

HRESULT Control::Create(_In_ HWND hWnd, _In_ const Config &configControl, _Out_ Control **lplpControl) noexcept
{
    Control *lpControl;
    HRESULT hr;

    if (!lplpControl)
    {
        return E_POINTER;
    }
    *lplpControl = nullptr;

    if (!configControl.szFontFamilyW)
    {
        return E_POINTER;
    }
    if ((!hWnd) || configControl.iRows <= 0 || configControl.iCols <= 0 || *configControl.szFontFamilyW == 0 ||
        configControl.fFontSize <= 0.0f)
    {
        return E_INVALIDARG;
    }

    lpControl = new (std::nothrow) Control();
    if (!lpControl)
    {
        return E_OUTOFMEMORY;
    }

    hr = lpControl->Initialize(hWnd, configControl);
    if (FAILED(hr))
    {
        delete lpControl;
        return hr;
    }

    {
        std::lock_guard<std::mutex> lockGuard(m_mapMutex);
        m_mapControls[hWnd] = lpControl;
    }

    // Publish the HWND mapping before workers start so window messages can safely find this control.
    hr = lpControl->StartBlinkThread();
    if (FAILED(hr))
    {
        std::lock_guard<std::mutex> lockGuard(m_mapMutex);

        m_mapControls.erase(hWnd);
        delete lpControl;
        return hr;
    }

    hr = lpControl->StartRenderThread();
    if (FAILED(hr))
    {
        lpControl->StopBlinkThread();
        std::lock_guard<std::mutex> lockGuard(m_mapMutex);

        m_mapControls.erase(hWnd);
        delete lpControl;
        return hr;
    }
    lpControl->RequestRender();

    *lplpControl = lpControl;
    return S_OK;
}

BOOL Control::WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT *lplResult) noexcept
{
    Control *lpControl;

    if (!lplResult)
    {
        return FALSE;
    }
    *lplResult = 0L;

    lpControl = GetControl(hWnd);
    if ((!lpControl) || hWnd != lpControl->m_hWnd)
    {
        return FALSE;
    }

    if (lpControl->m_bShuttingDown != FALSE && uMessage != WM_CLOSE && uMessage != WM_DESTROY && uMessage != WM_NCDESTROY)
    {
        if (uMessage == WM_PAINT)
        {
            PAINTSTRUCT sPs;

            if (BeginPaint(hWnd, &sPs))
            {
                EndPaint(hWnd, &sPs);
            }
            return TRUE;
        }
        return FALSE;
    }

    switch (uMessage)
    {
        case WM_ERASEBKGND:
            *lplResult = 1;
            return TRUE;

        case WM_PAINT:
            {
                PAINTSTRUCT sPs;

                // Presentation is owned by the render worker; WM_PAINT only validates this window region.
                if (BeginPaint(hWnd, &sPs))
                {
                    EndPaint(hWnd, &sPs);
                }
            }
            return TRUE;

        case WM_SIZE:
            {
                RECT rcClient;

                if (GetClientRect(hWnd, &rcClient) != FALSE)
                {
                    lpControl->ResizeRenderTarget(static_cast<UINT>(rcClient.right - rcClient.left),
                                                  static_cast<UINT>(rcClient.bottom - rcClient.top));
                    lpControl->RequestRender();
                }
            }
            break;

        case WM_DPICHANGED:
            {
                LPRECT lprcSuggested;

                lprcSuggested = reinterpret_cast<LPRECT>(lParam);
                if (lprcSuggested)
                {
                    SetWindowPos(hWnd, nullptr, lprcSuggested->left, lprcSuggested->top, lprcSuggested->right - lprcSuggested->left,
                                 lprcSuggested->bottom - lprcSuggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
                }
                lpControl->RefreshDpi();
                lpControl->RequestRender();
            }
            break;

        case WM_MOUSEMOVE:
            if (lpControl->HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) != FALSE)
            {
                lpControl->RequestRender();
                return TRUE;
            }
            break;

        case WM_MOUSELEAVE:
            if (lpControl->HandleMouseLeave() != FALSE)
            {
                lpControl->RequestRender();
                return TRUE;
            }
            break;

        case WM_LBUTTONDOWN:
            {
                BOOL bBeginCapture;
                INT iX;
                INT iY;

                bBeginCapture = FALSE;
                iX = GET_X_LPARAM(lParam);
                iY = GET_Y_LPARAM(lParam);
                if (lpControl->HandleLeftButtonDown(iX, iY, &bBeginCapture) != FALSE)
                {
                    if (bBeginCapture != FALSE)
                    {
                        SetCapture(hWnd);
                    }
                    lpControl->RequestRender();
                    return TRUE;
                }
            }
            break;

        case WM_LBUTTONUP:
            if (lpControl->HandleLeftButtonUp() != FALSE)
            {
                if (GetCapture() == hWnd)
                {
                    ReleaseCapture();
                }
                lpControl->RequestRender();
                return TRUE;
            }
            break;

        case WM_MOUSEWHEEL:
            if (lpControl->HandleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)) != FALSE)
            {
                lpControl->RequestRender();
                return TRUE;
            }
            break;

        case WM_CLOSE:
            lpControl->StopRenderThread(TRUE);
            break;

        case WM_DESTROY:
            lpControl->StopRenderThread(TRUE);
            break;

        case WM_NCDESTROY:
            {
                {
                    std::lock_guard<std::mutex> lockGuard(m_mapMutex);

                    m_mapControls.erase(hWnd);
                }

                lpControl->StopRenderThread(FALSE);
                lpControl->StopBlinkThread();
                if (GetCapture() == hWnd)
                {
                    ReleaseCapture();
                }

                delete lpControl;
            }
            break;
    }

    return FALSE;
}

Control *Control::GetControl(_In_ HWND hWnd)
{
    std::lock_guard<std::mutex> lockGuard(m_mapMutex);
    std::unordered_map<HWND, Control *>::const_iterator itControl;

    itControl = m_mapControls.find(hWnd);
    if (itControl == m_mapControls.end())
    {
        return nullptr;
    }
    return itControl->second;
}

VOID Control::Clear() noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Clear(nullptr);
    RequestRender();
}

VOID Control::Scroll(_In_ INT iLineCount) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Scroll(nullptr, iLineCount);
    m_sRenderer.InvalidateComposition();
    RequestRender();
}

VOID Control::Move(_In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY,
                   _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Move(nullptr, iSourceX, iSourceY, iWidth, iHeight, iTargetX, iTargetY, chFillW, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::Fill(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW, _In_ COLORREF crForeground,
                   _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Fill(nullptr, iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::DrawHorizontalLine(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                                 _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.DrawHorizontalLine(nullptr, iX, iY, iWidth, strokeType, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::DrawVerticalLine(_In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                               _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.DrawVerticalLine(nullptr, iX, iY, iHeight, strokeType, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::DrawBox(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground,
                      _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.DrawBox(nullptr, iX, iY, iWidth, iHeight, dwBoxSideFlags, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::Write(_In_z_ LPCWSTR szTextW) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (szTextW)
    {
        Internals::Parser m_sParser(m_sBuffer, nullptr);

        m_sParser.Feed(szTextW);
        RequestRender();
    }
}

VOID Control::Print(_In_z_ LPCWSTR szFormatW, ...) noexcept
{
    va_list argList;

    if (szFormatW)
    {
        va_start(argList, szFormatW);
        PrintV(szFormatW, argList);
        va_end(argList);
    }
}

VOID Control::PrintV(_In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    std::wstring strTextW;
    HRESULT hr;

    if (szFormatW)
    {
        hr = FormatWideStringV(szFormatW, argList, strTextW);
        if (SUCCEEDED(hr))
        {
            Internals::Parser m_sParser(m_sBuffer, nullptr);

            m_sParser.Feed(strTextW.c_str());
            RequestRender();
        }
    }
}

HRESULT Control::SetContext(_In_opt_ PVOID lpContext) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.SetRegionContext(nullptr, lpContext);
}

PVOID Control::GetContext() const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetRegionContext(nullptr);
}

HRESULT Control::CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                              _In_opt_ RegionHandle hRegionParent) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.CreateRegion(iX, iY, iWidth, iHeight, lphRegion, hRegionParent);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

HRESULT Control::CreateCustomDrawRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                                        _In_ const CustomDrawCallback &fnDrawCallback, _In_opt_ RegionHandle hRegionParent) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.CreateCustomDrawRegion(iX, iY, iWidth, iHeight, lphRegion, fnDrawCallback, hRegionParent);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

VOID Control::DestroyRegion(_In_ RegionHandle hRegion) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (hRegion)
    {
        m_sBuffer.DestroyRegion(hRegion);
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
}

VOID Control::ClearRegion(_In_opt_ RegionHandle hRegion) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Clear(hRegion);
    RequestRender();
}

VOID Control::InvalidateRegion(_In_opt_ RegionHandle hRegion) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (!hRegion || m_sBuffer.IsCustomDrawRegion(hRegion) != FALSE)
    {
        m_sRenderer.InvalidateCustomDrawRegion(hRegion);
    }
    else
    {
        m_sRenderer.InvalidateComposition();
    }
    RequestRender();
}

VOID Control::ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Scroll(hRegion, iLineCount);
    m_sRenderer.InvalidateComposition();
    RequestRender();
}

VOID Control::MoveRegion(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                         _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                         _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Move(hRegion, iSourceX, iSourceY, iWidth, iHeight, iTargetX, iTargetY, chFillW, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::FillRegion(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                         _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.Fill(hRegion, iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::DrawRegionHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType,
                                       _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.DrawHorizontalLine(hRegion, iX, iY, iWidth, strokeType, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::DrawRegionVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType,
                                     _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.DrawVerticalLine(hRegion, iX, iY, iHeight, strokeType, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::DrawRegionBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                            _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                            _In_ DWORD dwStyleFlags) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.DrawBox(hRegion, iX, iY, iWidth, iHeight, dwBoxSideFlags, crForeground, crBackground, dwStyleFlags);
    RequestRender();
}

VOID Control::WriteRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szTextW) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (szTextW)
    {
        Internals::Parser m_sParser(m_sBuffer, hRegion);

        m_sParser.Feed(szTextW);
        RequestRender();
    }
}

VOID Control::PrintRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, ...) noexcept
{
    va_list argList;

    if (szFormatW)
    {
        va_start(argList, szFormatW);
        PrintRegionV(hRegion, szFormatW, argList);
        va_end(argList);
    }
}

VOID Control::PrintRegionV(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, _In_ va_list argList) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    std::wstring strTextW;
    HRESULT hr;

    if (szFormatW)
    {
        hr = FormatWideStringV(szFormatW, argList, strTextW);
        if (SUCCEEDED(hr))
        {
            Internals::Parser m_sParser(m_sBuffer, hRegion);

            m_sParser.Feed(strTextW.c_str());
            RequestRender();
        }
    }
}

HRESULT Control::RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.RelocateRegion(hRegion, iX, iY, iWidth, iHeight);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

HRESULT Control::SetRegionVisible(_In_ RegionHandle hRegion, _In_ BOOL bVisible) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.SetRegionVisible(hRegion, bVisible);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

HRESULT Control::BringRegionToFront(_In_ RegionHandle hRegion) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.BringRegionToFront(hRegion);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

HRESULT Control::SendRegionToBack(_In_ RegionHandle hRegion) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.SendRegionToBack(hRegion);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

HRESULT Control::MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    HRESULT hr;

    hr = m_sBuffer.MoveRegionAfter(hRegion, hRegionReference);
    if (SUCCEEDED(hr))
    {
        m_sRenderer.InvalidateComposition();
        RequestRender();
    }
    return hr;
}

HRESULT Control::SetRegionContext(_In_opt_ RegionHandle hRegion, _In_opt_ PVOID lpContext) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.SetRegionContext(hRegion, lpContext);
}

PVOID Control::GetRegionContext(_In_opt_ RegionHandle hRegion) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetRegionContext(hRegion);
}

HRESULT Control::SetRegionDestroyCallback(_In_ RegionHandle hRegion, _In_ const RegionDestroyCallback &fnCallback) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.SetRegionDestroyCallback(hRegion, fnCallback);
}

HRESULT Control::SetCustomDrawRegionResourceCleanup(_In_ RegionHandle hRegion,
                                                    _In_ const CustomDrawResourceCleanupCallback &fnCallback) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.SetCustomDrawRegionResourceCleanup(hRegion, fnCallback);
}

RegionHandle Control::GetFirstRegion() const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetFirstRegion();
}

RegionHandle Control::GetLastRegion() const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetLastRegion();
}

RegionHandle Control::GetNextRegion(_In_opt_ RegionHandle hRegion) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetNextRegion(hRegion);
}

RegionHandle Control::GetPreviousRegion(_In_opt_ RegionHandle hRegion) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetPreviousRegion(hRegion);
}

RegionHandle Control::GetChildFirstRegion(_In_opt_ RegionHandle hRegionParent) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetChildFirstRegion(hRegionParent);
}

RegionHandle Control::GetChildLastRegion(_In_opt_ RegionHandle hRegionParent) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sBuffer.GetChildLastRegion(hRegionParent);
}

RegionHandle Control::GetParentRegion(_In_ RegionHandle hRegion) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (!hRegion)
    {
        return nullptr;
    }
    return m_sBuffer.GetParentRegion(hRegion);
}

VOID Control::GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                                _Out_opt_ LPINT lpiHeight) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (!hRegion)
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
            *lpiWidth = m_iCols;
        }
        if (lpiHeight)
        {
            *lpiHeight = 0;
        }
        return;
    }

    m_sBuffer.GetRegionLocation(hRegion, lpiX, lpiY, lpiWidth, lpiHeight);
}

BOOL Control::ConvertToRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal,
                                         _Out_opt_ LPINT lpiColRegion, _Out_opt_ LPINT lpiRowRegion) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (!hRegion)
    {
        return FALSE;
    }
    return m_sBuffer.ConvertToRegionCoordinates(hRegion, iColTerminal, iRowTerminal, lpiColRegion, lpiRowRegion);
}

BOOL Control::ConvertFromRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion,
                                           _Out_opt_ LPINT lpiColTerminal, _Out_opt_ LPINT lpiRowTerminal) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (!hRegion)
    {
        return FALSE;
    }
    return m_sBuffer.ConvertFromRegionCoordinates(hRegion, iColRegion, iRowRegion, lpiColTerminal, lpiRowTerminal);
}

VOID Control::ShowCursor(_In_opt_ RegionHandle hRegion) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.ShowCursor(hRegion);
    RequestRender();
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

VOID Control::HideCursor() noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.HideCursor();
    RequestRender();
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

VOID Control::SetCursorStyle(_In_ CursorStyle style) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.SetCursorStyle(style);
    RequestRender();
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

HRESULT Control::ResizeTerminal(_In_ INT iCols, _In_ INT iRows) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    HRESULT hr;

    if (iCols <= 0 || iRows <= 0)
    {
        return E_INVALIDARG;
    }

    hr = m_sBuffer.Resize(iCols, iRows);
    if (FAILED(hr))
    {
        return hr;
    }

    m_iCols = iCols;
    m_iRows = iRows;
    m_sRenderer.InvalidateComposition();
    UpdateScrollBars();
    RequestRender();
    return S_OK;
}

VOID Control::GetTerminalSize(_Out_opt_ LPINT lpiCols, _Out_opt_ LPINT lpiRows) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    if (lpiCols)
    {
        *lpiCols = m_iCols;
    }
    if (lpiRows)
    {
        *lpiRows = m_iRows;
    }
}

HRESULT Control::GetPreferredClientSize(_Out_ LPSIZE lpSize) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sRenderer.GetPreferredClientSize(m_iCols, m_iRows, lpSize);
}

HRESULT Control::GetPreferredWindowSize(_Out_ LPSIZE lpSize, _In_opt_ BOOL bHasMenu) const noexcept
{
    SIZE sizeClient;
    DWORD dwStyle;
    DWORD dwExStyle;
    RECT rcWindow;
    HRESULT hr;

    if (!lpSize)
    {
        return E_POINTER;
    }
    lpSize->cx = 0;
    lpSize->cy = 0;

    hr = m_sRenderer.GetPreferredClientSize(m_iCols, m_iRows, &sizeClient);
    if (FAILED(hr))
    {
        return hr;
    }
    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = sizeClient.cx;
    rcWindow.bottom = sizeClient.cy;
    dwStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hWnd, GWL_STYLE));
    dwExStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE));
    if (AdjustWindowRectExForDpi(&rcWindow, dwStyle, bHasMenu, dwExStyle, GetDpiForWindow(m_hWnd)) == FALSE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    lpSize->cx = rcWindow.right - rcWindow.left;
    lpSize->cy = rcWindow.bottom - rcWindow.top;
    return S_OK;
}

HRESULT Control::GetCellSize(_Out_ LPSIZE lpSize) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sRenderer.GetCellSize(lpSize);
}

BOOL Control::GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sRenderer.GetCellPosition(iCol, iRow, lprcCell);
}

BOOL Control::GetCellFromPosition(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sRenderer.HitTestCell(iX, iY, lpiCol, lpiRow);
}

HRESULT Control::Initialize(_In_ HWND hWnd, _In_ const Config &configControl) noexcept
{
    HRESULT hr;

    hr = m_sBuffer.Initialize(configControl.iCols, configControl.iRows, configControl.crDefaultForeground,
                              configControl.crDefaultBackground);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_sRenderer.Initialize(hWnd, configControl.szFontFamilyW, configControl.fFontSize);
    if (FAILED(hr))
    {
        return hr;
    }

    m_iCols = configControl.iCols;
    m_iRows = configControl.iRows;
    m_hWnd = hWnd;
    m_sRenderer.SetContentSize(m_iCols, m_iRows);
    m_sRenderer.UpdateScrollBars();
    return S_OK;
}

HRESULT Control::Present() noexcept
{
    HRESULT hr;

    {
        // Keep buffer snapshots and renderer cache updates synchronized with API mutations.
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        hr = m_sRenderer.Render(m_sBuffer);
    }
    if (FAILED(hr))
    {
        return hr;
    }
    hr = m_sRenderer.Present();
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.NotifyCustomDrawResourceCleanup(CustomDrawResourceCleanupReason::TargetLost);
    }
    return hr;
}

VOID Control::RequestRender() noexcept
{
    if (m_bShuttingDown != FALSE || (!m_hRenderRequestEvent))
    {
        return;
    }
    // Coalesce repeated mutations until the render worker consumes this request.
    if (m_bRenderRequested.exchange(TRUE) == FALSE)
    {
        SetEvent(m_hRenderRequestEvent);
    }
}

HRESULT Control::ResizeRenderTarget(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    HRESULT hr;

    if (uiWidth == 0 || uiHeight == 0)
    {
        return E_INVALIDARG;
    }

    hr = m_sRenderer.Resize(uiWidth, uiHeight);
    if (FAILED(hr))
    {
        return hr;
    }
    UpdateScrollBars();
    RequestRender();
    return S_OK;
}

VOID Control::UpdateScrollBars() noexcept
{
    m_sRenderer.SetContentSize(m_iCols, m_iRows);
    m_sRenderer.UpdateScrollBars();
    if (m_sRenderer.GetScrollOffsetX() == 0)
    {
        m_iScrollOffsetOriginX = 0;
    }
    if (m_sRenderer.GetScrollOffsetY() == 0)
    {
        m_iScrollOffsetOriginY = 0;
    }
}

BOOL Control::HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    TRACKMOUSEEVENT trackMouseEvent;
    BOOL bConsumed;

    if (m_bTrackingMouse == FALSE)
    {
        trackMouseEvent = TRACKMOUSEEVENT{};
        trackMouseEvent.cbSize = sizeof(trackMouseEvent);
        trackMouseEvent.dwFlags = TME_LEAVE;
        trackMouseEvent.hwndTrack = m_hWnd;
        if (TrackMouseEvent(&trackMouseEvent) != FALSE)
        {
            m_bTrackingMouse = TRUE;
        }
    }

    if (m_bDraggingScrollBar != FALSE)
    {
        if (m_scrollBarPartDragging == ScrollBarPartVerticalThumb)
        {
            return m_sRenderer.ScrollFromThumbDrag(TRUE, iY, m_iScrollDragOriginY, m_iScrollOffsetOriginY);
        }
        if (m_scrollBarPartDragging == ScrollBarPartHorizontalThumb)
        {
            return m_sRenderer.ScrollFromThumbDrag(FALSE, iX, m_iScrollDragOriginX, m_iScrollOffsetOriginX);
        }
        return TRUE;
    }

    bConsumed = (m_sRenderer.HitTestScrollBars(iX, iY, nullptr, nullptr) != FALSE) ? TRUE : FALSE;
    if (m_sRenderer.HandleMouseMove(iX, iY) != FALSE)
    {
        return TRUE;
    }
    return bConsumed;
}

BOOL Control::HandleMouseLeave() noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_bTrackingMouse = FALSE;
    return m_sRenderer.HandleMouseLeave();
}

BOOL Control::HandleLeftButtonDown(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbBeginCapture) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    BOOL bVertical;
    BOOL bThumb;

    if (lpbBeginCapture)
    {
        *lpbBeginCapture = FALSE;
    }

    if (m_sRenderer.HitTestScrollBars(iX, iY, &bVertical, &bThumb) == FALSE)
    {
        return FALSE;
    }
    if (bThumb != FALSE)
    {
        // Retain the pointer and scroll origins so each mouse move can calculate an absolute drag offset.
        m_bDraggingScrollBar = TRUE;
        m_scrollBarPartDragging = (bVertical != FALSE) ? ScrollBarPartVerticalThumb : ScrollBarPartHorizontalThumb;
        m_iScrollDragOriginX = iX;
        m_iScrollDragOriginY = iY;
        m_iScrollOffsetOriginX = m_sRenderer.GetScrollOffsetX();
        m_iScrollOffsetOriginY = m_sRenderer.GetScrollOffsetY();
        if (lpbBeginCapture)
        {
            *lpbBeginCapture = TRUE;
        }
        return TRUE;
    }
    return m_sRenderer.ScrollByTrackClick(bVertical, (bVertical != FALSE) ? iY : iX);
}

BOOL Control::HandleLeftButtonUp() noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    BOOL bWasDragging;

    bWasDragging = m_bDraggingScrollBar;
    m_bDraggingScrollBar = FALSE;
    m_scrollBarPartDragging = ScrollBarPartNone;
    return bWasDragging;
}

BOOL Control::HandleMouseWheel(_In_ SHORT iDelta) noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    return m_sRenderer.ScrollByWheelDelta(iDelta);
}

VOID Control::RefreshDpi() noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sRenderer.RefreshDpi();
    UpdateScrollBars();
    RequestRender();
}

VOID Control::ToggleBlink() noexcept
{
    std::lock_guard<std::mutex> lockGuard(m_mutex);

    m_sBuffer.ToggleBlinkVisibility();
    RequestRender();
}

HRESULT Control::StartBlinkThread() noexcept
{
    HRESULT hr;

    m_hBlinkStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_hBlinkStopEvent)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hr = S_OK;
    try
    {
        m_threadBlink = std::thread(&Control::BlinkThreadEntry, this);
    }
    catch (const std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }
    catch (...)
    {
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr))
    {
        CloseHandle(m_hBlinkStopEvent);
        m_hBlinkStopEvent = nullptr;
    }
    return hr;
}

VOID Control::StopBlinkThread() noexcept
{
    if (m_hBlinkStopEvent)
    {
        SetEvent(m_hBlinkStopEvent);
    }
    if (m_threadBlink.joinable())
    {
        m_threadBlink.join();
    }
    if (m_hBlinkStopEvent)
    {
        CloseHandle(m_hBlinkStopEvent);
        m_hBlinkStopEvent = nullptr;
    }
}

VOID Control::BlinkThreadEntry(_In_ Control *lpControl) noexcept
{
    DWORD dwWaitResult;

    if ((!lpControl) || (!lpControl->m_hBlinkStopEvent))
    {
        return;
    }

    for (;;)
    {
        // Blink changes renderer state without relying on a window paint message.
        dwWaitResult = WaitForSingleObject(lpControl->m_hBlinkStopEvent, 500U);
        if (dwWaitResult != WAIT_TIMEOUT)
        {
            break;
        }

        lpControl->ToggleBlink();
        lpControl->RequestRender();
    }
}

HRESULT Control::StartRenderThread() noexcept
{
    HRESULT hr;

    m_hRenderStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_hRenderStopEvent)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    m_hRenderRequestEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_hRenderRequestEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle(m_hRenderStopEvent);
        m_hRenderStopEvent = nullptr;
        return hr;
    }
    hr = S_OK;
    try
    {
        m_threadRender = std::thread(&Control::StaticRenderThread, this);
    }
    catch (const std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }
    catch (...)
    {
        hr = E_UNEXPECTED;
    }
    if (FAILED(hr))
    {
        CloseHandle(m_hRenderRequestEvent);
        CloseHandle(m_hRenderStopEvent);
        m_hRenderRequestEvent = nullptr;
        m_hRenderStopEvent = nullptr;
    }
    return hr;
}

VOID Control::StopRenderThread(_In_ BOOL bPumpMessages) noexcept
{
    BOOL bQuitReceived;
    INT iQuitCode;

    if (m_bShuttingDown.exchange(TRUE) != FALSE)
    {
        return;
    }
    if (m_hRenderStopEvent)
    {
        SetEvent(m_hRenderStopEvent);
    }
    bQuitReceived = FALSE;
    iQuitCode = 0;
    if (m_threadRender.joinable())
    {
        if (bPumpMessages != FALSE)
        {
            HANDLE hThread;

            // Closing on the UI thread must continue dispatching messages while the render worker drains.
            hThread = m_threadRender.native_handle();
            for (;;)
            {
                DWORD dwWaitResult;

                dwWaitResult = MsgWaitForMultipleObjectsEx(1U, &hThread, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                if (dwWaitResult == WAIT_OBJECT_0)
                {
                    break;
                }
                if (dwWaitResult == WAIT_OBJECT_0 + 1U)
                {
                    MSG msg;

                    while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE) != FALSE)
                    {
                        if (msg.message == WM_QUIT)
                        {
                            bQuitReceived = TRUE;
                            iQuitCode = static_cast<INT>(msg.wParam);
                            continue;
                        }
                        TranslateMessage(&msg);
                        DispatchMessageW(&msg);
                    }
                    continue;
                }
                break;
            }
        }
        m_threadRender.join();
    }
    if (m_hRenderRequestEvent)
    {
        CloseHandle(m_hRenderRequestEvent);
        m_hRenderRequestEvent = nullptr;
    }
    if (m_hRenderStopEvent)
    {
        CloseHandle(m_hRenderStopEvent);
        m_hRenderStopEvent = nullptr;
    }
    if (bQuitReceived != FALSE)
    {
        PostQuitMessage(iQuitCode);
    }
}

VOID Control::StaticRenderThread(_In_ Control *lpControl) noexcept
{
    _ASSERT(lpControl);
    _ASSERT(lpControl->m_hRenderStopEvent);
    _ASSERT(lpControl->m_hRenderRequestEvent);
    lpControl->RenderThread();
}

VOID Control::RenderThread() noexcept
{
    HANDLE aWaitHandles[2];
    DWORD dwWaitResult;
    HRESULT hr;

    for (;;)
    {
        aWaitHandles[0] = m_hRenderStopEvent;
        aWaitHandles[1] = m_hRenderRequestEvent;
        dwWaitResult = WaitForMultipleObjects(2U, aWaitHandles, FALSE, INFINITE);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            break;
        }
        if (dwWaitResult != WAIT_OBJECT_0 + 1U)
        {
            break;
        }
        m_bRenderRequested.exchange(FALSE);
        for (;;)
        {
            HANDLE hFrameLatency;

            {
                // Avoid issuing a new frame until DXGI can accept it without queueing latency.
                std::lock_guard<std::mutex> lockGuard(m_mutex);

                hFrameLatency = m_sRenderer.ConsumeFrameLatencyWaitableObject();
            }
            if (hFrameLatency)
            {
                aWaitHandles[0] = m_hRenderStopEvent;
                aWaitHandles[1] = hFrameLatency;
                dwWaitResult = WaitForMultipleObjects(2U, aWaitHandles, FALSE, INFINITE);
                if (dwWaitResult == WAIT_OBJECT_0)
                {
                    return;
                }
                if (dwWaitResult != WAIT_OBJECT_0 + 1U)
                {
                    return;
                }
            }
            if (m_bShuttingDown != FALSE)
            {
                return;
            }

            hr = Present();
            if (FAILED(hr))
            {
                RequestRender();
            }
            if (!m_bRenderRequested.exchange(FALSE))
            {
                break;
            }
        }
    }
}

} // namespace GuiTerminal

// -----------------------------------------------------------------------------

static HRESULT FormatWideStringV(_In_z_ LPCWSTR pszFormatW, _In_ va_list argList, _Out_ std::wstring &strTextW) noexcept
{
    va_list argListCopy;
    INT iCharCount;

    strTextW.clear();
    try
    {
        va_copy(argListCopy, argList);
        iCharCount = _vscwprintf(pszFormatW, argListCopy);
        va_end(argListCopy);

        if (iCharCount > 0)
        {
            strTextW.assign(static_cast<size_t>(iCharCount) + 1U, L'\0');

            va_copy(argListCopy, argList);
            _vsnwprintf_s(&strTextW[0], strTextW.size(), static_cast<size_t>(iCharCount), pszFormatW, argListCopy);
            va_end(argListCopy);

            strTextW.resize(static_cast<size_t>(iCharCount));
        }
    }
    catch (const std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }
    return S_OK;
}
