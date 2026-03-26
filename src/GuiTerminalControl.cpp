#include "..\include\GuiTerminalControl.h"
#include "..\include\GuiTerminalParser.h"
#include <cstdio>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// -----------------------------------------------------------------------------

static HRESULT FormatWideStringV(_In_z_ LPCWSTR pszFormatW, _In_ va_list argList, _Out_ std::wstring& strTextW) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    HRESULT Control::Create(_In_ HWND hWnd, _In_ INT iRows, _In_ INT iCols, _Out_ Control** lplpControl) noexcept
    {
        Control* lpControl;
        HRESULT hr;

        if (lplpControl == nullptr)
        {
            return E_POINTER;
        }
        *lplpControl = nullptr;

        if ((hWnd == nullptr) || (iRows <= 0) || (iCols <= 0))
        {
            return E_INVALIDARG;
        }

        lpControl = new (std::nothrow) Control();
        if (lpControl == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        hr = lpControl->Initialize(hWnd, iCols, iRows);
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
        if ((lpControl == nullptr) || (hWnd != lpControl->m_hWnd))
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
                    }
                }
                *lplResult = 0;
                return TRUE;

            case WM_DPICHANGED:
                {
                    LPRECT lprcSuggested = reinterpret_cast<LPRECT>(lParam);
                    if (lprcSuggested != nullptr)
                    {
                        SetWindowPos(hWnd, nullptr, lprcSuggested->left, lprcSuggested->top,
                                     lprcSuggested->right - lprcSuggested->left, lprcSuggested->bottom - lprcSuggested->top,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    lpControl->RefreshDpi();
                }
                *lplResult = 0;
                return TRUE;

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

                SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
                delete lpControl;

                *lplResult = 0;
                return TRUE;
        }

        return FALSE;
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

        if ((iWidth <= 0) || (iHeight <= 0))
        {
            return;
        }
        m_sBuffer.FillArea(nullptr, iX, iY, iWidth, iHeight, chCodepointW, crForeground, crBackground, dwStyleFlags);
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

        if ((lphRegion == nullptr) || (iWidth <= 0) || (iHeight <= 0))
        {
            return E_INVALIDARG;
        }
        return m_sBuffer.CreateRegion(iX, iY, iWidth, iHeight, lphRegion);
    }

    VOID Control::DestroyRegion(_In_ RegionHandle hRegion) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);

        if (hRegion != nullptr)
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

        if ((iWidth > 0) && (iHeight > 0))
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

    HRESULT Control::ResizeGuiTerminal(_In_ INT iCols, _In_ INT iRows) noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);
        HRESULT hr;

        if ((iCols <= 0) || (iRows <= 0))
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
        return S_OK;
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

    HRESULT Control::Initialize(_In_ HWND hWnd, _In_ INT iCols, _In_ INT iRows) noexcept
    {
        HRESULT hr;

        hr = m_sBuffer.Initialize(iCols, iRows);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = m_sRenderer.Initialize(hWnd);
        if (FAILED(hr))
        {
            return hr;
        }

        m_iCols = iCols;
        m_iRows = iRows;
        m_hWnd = hWnd;
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

        if ((uiWidth == 0U) || (uiHeight == 0U))
        {
            return E_INVALIDARG;
        }
        return m_sRenderer.Resize(uiWidth, uiHeight);
    }

    VOID Control::RefreshDpi() noexcept
    {
        std::lock_guard<std::mutex> lockGuard(m_mutex);
        m_sRenderer.RefreshDpi();
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

