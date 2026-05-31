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

        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lpControl));
        hr = lpControl->StartBlinkThread();
        if (FAILED(hr))
        {
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
            delete lpControl;
            return hr;
        }

        *lplpControl = lpControl;
        return S_OK;
    }

    BOOL Control::WndProc(_In_ HWND hWnd, _In_ UINT uMessage, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ LRESULT* lplResult) noexcept
    {
        Control* lpControl;

        if (!lplResult)
        {
            return FALSE;
        }
        *lplResult = 0L;

        lpControl = reinterpret_cast<Control*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if ((!lpControl) || hWnd != lpControl->m_hWnd)
        {
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

                    if (BeginPaint(hWnd, &sPs))
                    {
                        lpControl->Present();
                        EndPaint(hWnd, &sPs);
                    }


                    return TRUE;
                }

            case WindowMessageBlinkRedraw:
                InvalidateRect(hWnd, nullptr, FALSE);
                return TRUE;

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
                break;

            case WM_DPICHANGED:
                {
                    LPRECT lprcSuggested;

                    lprcSuggested = reinterpret_cast<LPRECT>(lParam);
                    if (lprcSuggested)
                    {
                        SetWindowPos(hWnd, nullptr, lprcSuggested->left, lprcSuggested->top,
                                     lprcSuggested->right - lprcSuggested->left, lprcSuggested->bottom - lprcSuggested->top,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    lpControl->RefreshDpi();
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                break;

            case WM_MOUSEMOVE:
                if (lpControl->HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) != FALSE)
                {
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return TRUE;
                }
                break;

            case WM_MOUSELEAVE:
                if (lpControl->HandleMouseLeave() != FALSE)
                {
                    InvalidateRect(hWnd, nullptr, FALSE);
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
                        InvalidateRect(hWnd, nullptr, FALSE);
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
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return TRUE;
                }
                break;

            case WM_MOUSEWHEEL:
                if (lpControl->HandleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)) != FALSE)
                {
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return TRUE;
                }
                break;

            case WM_NCDESTROY:
                lpControl->StopBlinkThread();
                if (GetCapture() == hWnd)
                {
                    ReleaseCapture();
                }

                SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
                delete lpControl;
                break;
        }

        // Done
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

    VOID Control::Move(_In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX, _In_ INT iTargetY,
                       _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (iWidth > 0 && iHeight > 0)
        {
            m_sBuffer.MoveArea(nullptr, iSourceX, iSourceY, iWidth, iHeight, iTargetX, iTargetY, chFillW, crForeground, crBackground,
                               dwStyleFlags);
        }
    }

    VOID Control::DrawHorizontalLine(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                                     _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.DrawHorizontalLine(nullptr, iX, iY, iWidth, strokeType, crForeground, crBackground, dwStyleFlags);
    }

    VOID Control::DrawVerticalLine(_In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ StrokeType strokeType, _In_ COLORREF crForeground,
                                   _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.DrawVerticalLine(nullptr, iX, iY, iHeight, strokeType, crForeground, crBackground, dwStyleFlags);
    }

    VOID Control::DrawBox(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags,
                          _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                          _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.DrawBox(nullptr, iX, iY, iWidth, iHeight, dwBoxSideFlags, crForeground, crBackground, dwStyleFlags);
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

    HRESULT Control::CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion,
                                  _In_opt_ RegionHandle hRegionParent) noexcept
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
        return m_sBuffer.CreateRegion(iX, iY, iWidth, iHeight, lphRegion, hRegionParent);
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

    VOID Control::MoveRegion(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight,
                             _In_ INT iTargetX, _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground,
                             _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (iWidth > 0 && iHeight > 0)
        {
            m_sBuffer.MoveArea(hRegion, iSourceX, iSourceY, iWidth, iHeight, iTargetX, iTargetY, chFillW, crForeground, crBackground,
                               dwStyleFlags);
        }
    }

    VOID Control::DrawRegionHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth,
                                           _In_ StrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                           _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.DrawHorizontalLine(hRegion, iX, iY, iWidth, strokeType, crForeground, crBackground, dwStyleFlags);
    }

    VOID Control::DrawRegionVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight,
                                         _In_ StrokeType strokeType, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                         _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.DrawVerticalLine(hRegion, iX, iY, iHeight, strokeType, crForeground, crBackground, dwStyleFlags);
    }

    VOID Control::DrawRegionBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                                _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                _In_ DWORD dwStyleFlags) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.DrawBox(hRegion, iX, iY, iWidth, iHeight, dwBoxSideFlags, crForeground, crBackground, dwStyleFlags);
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
            }
        }
    }

    HRESULT Control::RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (!hRegion)
        {
            return E_POINTER;
        }
        if (iWidth <= 0 || iHeight <= 0)
        {
            return E_INVALIDARG;
        }
        return m_sBuffer.RelocateRegion(hRegion, iX, iY, iWidth, iHeight);
    }

    HRESULT Control::BringRegionToFront(_In_ RegionHandle hRegion) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (!hRegion)
        {
            return E_POINTER;
        }
        return m_sBuffer.BringRegionToFront(hRegion);
    }

    HRESULT Control::SendRegionToBack(_In_ RegionHandle hRegion) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (!hRegion)
        {
            return E_POINTER;
        }
        return m_sBuffer.SendRegionToBack(hRegion);
    }

    HRESULT Control::MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (!hRegion)
        {
            return E_POINTER;
        }
        return m_sBuffer.MoveRegionAfter(hRegion, hRegionReference);
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
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }

    VOID Control::HideCursor() noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.HideCursor();
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }

    VOID Control::SetCursorStyle(_In_ CursorStyle style) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.SetCursorStyle(style);
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
        UpdateScrollBars();
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
    }

    VOID Control::ToggleBlink() noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        m_sBuffer.ToggleBlinkVisibility();
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
        catch (const std::bad_alloc&)
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

    VOID Control::BlinkThreadEntry(_In_ Control* lpControl) noexcept
    {
        DWORD dwWaitResult;

        if ((!lpControl) || (!lpControl->m_hBlinkStopEvent))
        {
            return;
        }

        for (;;)
        {
            dwWaitResult = WaitForSingleObject(lpControl->m_hBlinkStopEvent, 500U);
            if (dwWaitResult != WAIT_TIMEOUT)
            {
                break;
            }

            lpControl->ToggleBlink();
            if (lpControl->m_hWnd)
            {
                PostMessageW(lpControl->m_hWnd, WindowMessageBlinkRedraw, 0U, 0L);
            }
        }
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
