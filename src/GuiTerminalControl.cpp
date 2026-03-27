#include "..\include\GuiTerminalControl.h"
#include "..\include\GuiTerminalParser.h"
#include <cstdio>
#include <string>
#include <windowsx.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// -----------------------------------------------------------------------------

static HRESULT FormatWideStringV(_In_z_ LPCWSTR pszFormatW, _In_ va_list argList, _Out_ std::wstring& strTextW) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    HRESULT Control::Create(_In_ HWND hWnd, _In_ const Config& configControl, _Out_ Control** lplpControl) noexcept
    {
        Control* lpControl;
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
        if (lpControl == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        hr = lpControl->Initialize(hWnd, configControl);
        if (FAILED(hr))
        {
            delete lpControl;
            return hr;
        }

        if (SetTimer(hWnd, 1U, 500U, nullptr) == 0U)
        {
            delete lpControl;
            return HRESULT_FROM_WIN32(GetLastError());
        }

        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lpControl));

        *lplpControl = lpControl;
        return S_OK;
    }

    BOOL Control::WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT* lplResult) noexcept
    {
        Control* lpControl;

        lpControl = reinterpret_cast<Control*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

        *lplResult = 0L;
        if ((!lpControl) || hWnd != lpControl->m_hWnd)
        {
            return FALSE;
        }

        switch (uMessage)
        {
            case WM_PAINT:
                {
                    PAINTSTRUCT ps;

                    if (BeginPaint(hWnd, &ps) != nullptr)
                    {
                        lpControl->Present();
                        EndPaint(hWnd, &ps);
                    }

                    *lplResult = 0;
                    return TRUE;
                }

            case WM_SIZE:
                {
                    RECT rcClient;

                    if (GetClientRect(hWnd, &rcClient) != FALSE)
                    {
                        lpControl->ResizeRenderTarget(static_cast<UINT>(rcClient.right - rcClient.left),
                                                      static_cast<UINT>(rcClient.bottom - rcClient.top));
                        InvalidateRect(hWnd, nullptr, FALSE);
                    }
                }
                *lplResult = 0;
                return TRUE;

            case WM_DPICHANGED:
                {
                    LPRECT lprcSuggested = reinterpret_cast<LPRECT>(lParam);
                    if (lprcSuggested)
                    {
                        SetWindowPos(hWnd, nullptr, lprcSuggested->left, lprcSuggested->top,
                                     lprcSuggested->right - lprcSuggested->left, lprcSuggested->bottom - lprcSuggested->top,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    lpControl->RefreshDpi();
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                *lplResult = 0;
                return TRUE;

            case WM_MOUSEMOVE:
                if (lpControl->HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) != FALSE)
                {
                    InvalidateRect(hWnd, nullptr, FALSE);
                    *lplResult = 0;
                    return TRUE;
                }
                break;

            case WM_MOUSELEAVE:
                if (lpControl->HandleMouseLeave() != FALSE)
                {
                    InvalidateRect(hWnd, nullptr, FALSE);
                    *lplResult = 0;
                    return TRUE;
                }
                break;

            case WM_LBUTTONDOWN:
                if (lpControl->HandleLeftButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) != FALSE)
                {
                    if (lpControl->m_bDraggingScrollBar != FALSE)
                    {
                        SetCapture(hWnd);
                    }
                    InvalidateRect(hWnd, nullptr, FALSE);
                    *lplResult = 0;
                    return TRUE;
                }
                break;

            case WM_LBUTTONUP:
                if (lpControl->HandleLeftButtonUp() != FALSE)
                {
                    if (GetCapture() == hWnd)
                    {
                        ReleaseCapture();
                    }
                    InvalidateRect(hWnd, nullptr, FALSE);
                    *lplResult = 0;
                    return TRUE;
                }
                break;

            case WM_MOUSEWHEEL:
                if (lpControl->HandleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)) != FALSE)
                {
                    InvalidateRect(hWnd, nullptr, FALSE);
                    *lplResult = 0;
                    return TRUE;
                }
                break;

            case WM_TIMER:
                if (static_cast<UINT_PTR>(wParam) == lpControl->m_uiBlinkTimerId)
                {
                    lpControl->ToggleBlink();
                    InvalidateRect(hWnd, nullptr, FALSE);
                    *lplResult = 0;
                    return TRUE;
                }
                break;

            case WM_NCDESTROY:
                if (lpControl->m_uiBlinkTimerId != 0)
                {
                    KillTimer(hWnd, lpControl->m_uiBlinkTimerId);
                }
                if (GetCapture() == hWnd)
                {
                    ReleaseCapture();
                }

                SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
                delete lpControl;

                *lplResult = 0;
                return TRUE;
        }

        return FALSE;
    }

    Control* Control::GetControl(_In_ HWND hWnd)
    {
        return reinterpret_cast<Control *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    VOID Control::Clear() noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.ClearRegion(nullptr);
    }

    VOID Control::Scroll(_In_ INT iLineCount) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.ScrollRegion(nullptr, iLineCount);
    }

    VOID Control::FillArea(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW,
                           _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (iWidth > 0 && iHeight > 0)
        {
            m_sBuffer.FillArea(nullptr, iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
        }
    }

    VOID Control::Write(_In_z_ LPCWSTR szTextW) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (szTextW)
        {
            Internals::Parser m_sParser(m_sBuffer, nullptr);

            m_sParser.Feed(szTextW);
        }
    }

    VOID Control::Print(_In_z_ LPCWSTR szFormatW, ...) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);
        std::wstring strTextW;
        va_list argList;
        HRESULT hr;

        if (szFormatW)
        {
            va_start(argList, szFormatW);
            hr = FormatWideStringV(szFormatW, argList, strTextW);
            va_end(argList);

            if (SUCCEEDED(hr))
            {
                Internals::Parser m_sParser(m_sBuffer, nullptr);

                m_sParser.Feed(strTextW.c_str());
            }
        }
    }

    HRESULT Control::CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (!lphRegion)
        {
            return E_POINTER;
        }
        if (iWidth <= 0 || iHeight <= 0)
        {
            return E_INVALIDARG;
        }
        return m_sBuffer.CreateRegion(iX, iY, iWidth, iHeight, lphRegion);
    }

    VOID Control::DestroyRegion(_In_ RegionHandle hRegion) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (hRegion)
        {
            m_sBuffer.DestroyRegion(hRegion);
        }
    }

    VOID Control::ClearRegion(_In_opt_ RegionHandle hRegion) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.ClearRegion(hRegion);
    }

    VOID Control::ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.ScrollRegion(hRegion, iLineCount);
    }

    VOID Control::FillRegionArea(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                 _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                 _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (iWidth > 0 && iHeight > 0)
        {
            m_sBuffer.FillArea(hRegion, iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
        }
    }

    VOID Control::WriteRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szTextW) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (szTextW)
        {
            Internals::Parser m_sParser(m_sBuffer, hRegion);

            m_sParser.Feed(szTextW);
        }
    }

    VOID Control::PrintRegion(_In_opt_ RegionHandle hRegion, _In_z_ LPCWSTR szFormatW, ...) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);
        std::wstring strTextW;
        va_list argList;
        HRESULT hr;

        if (szFormatW)
        {
            va_start(argList, szFormatW);
            hr = FormatWideStringV(szFormatW, argList, strTextW);
            va_end(argList);

            if (SUCCEEDED(hr))
            {
                Internals::Parser m_sParser(m_sBuffer, hRegion);

                m_sParser.Feed(strTextW.c_str());
            }
        }
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
        UpdateScrollBars();
        return S_OK;
    }

    HRESULT Control::GetCellSize(_Out_ LPSIZE lpSize) const noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        return m_sRenderer.GetCellSize(lpSize);
    }

    HRESULT Control::GetPreferredClientSize(_Out_ LPSIZE lpSize) const noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        return m_sRenderer.GetPreferredClientSize(m_iCols, m_iRows, lpSize);
    }

    HRESULT Control::GetPreferredWindowSize(_Out_ LPSIZE lpSize) const noexcept
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
        if (AdjustWindowRectExForDpi(&rcWindow, dwStyle, TRUE, dwExStyle, GetDpiForWindow(m_hWnd)) == FALSE)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        lpSize->cx = rcWindow.right - rcWindow.left;
        lpSize->cy = rcWindow.bottom - rcWindow.top;
        return S_OK;
    }

    HRESULT Control::Initialize(_In_ HWND hWnd, _In_ const Config& configControl) noexcept
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
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        return m_sRenderer.Render(m_sBuffer);
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

    BOOL Control::HandleLeftButtonDown(_In_ INT iX, _In_ INT iY) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);
        BOOL bVertical;
        BOOL bThumb;

        if (m_sRenderer.HitTestScrollBars(iX, iY, &bVertical, &bThumb) == FALSE)
        {
            return FALSE;
        }
        if (bThumb != FALSE)
        {
            m_bDraggingScrollBar = TRUE;
            m_scrollBarPartDragging = (bVertical != FALSE) ? ScrollBarPartVerticalThumb : ScrollBarPartHorizontalThumb;
            m_iScrollDragOriginX = iX;
            m_iScrollDragOriginY = iY;
            m_iScrollOffsetOriginX = m_sRenderer.GetScrollOffsetX();
            m_iScrollOffsetOriginY = m_sRenderer.GetScrollOffsetY();
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
    }

    VOID Control::ToggleBlink() noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.ToggleBlinkVisibility();
    }
}

// -----------------------------------------------------------------------------

static HRESULT FormatWideStringV(_In_z_ LPCWSTR pszFormatW, _In_ va_list argList, _Out_ std::wstring& strTextW) noexcept
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
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }
    return S_OK;
}
