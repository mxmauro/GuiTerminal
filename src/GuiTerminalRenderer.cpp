#include "..\include\GuiTerminalRenderer.h"
#include "..\include\GuiTerminalControl.h"
#include <algorithm>
#include <cmath>

// -----------------------------------------------------------------------------

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor) noexcept;
static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor, _In_ FLOAT fAlpha) noexcept;
static FLOAT GetColorLuminance(_In_ COLORREF crColor) noexcept;
static BOOL IsPointInRect(_In_ INT iX, _In_ INT iY, _In_ const D2D1_RECT_F& rcCurrent) noexcept;
static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal::Internals
{
    HRESULT Renderer::Initialize(_In_ HWND hWnd, _In_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize) noexcept
    {
        RECT rcClient;
        HRESULT hr;

        if ((!hWnd) || (!szFontFamilyW) || *szFontFamilyW == 0 || fFontSize <= 0.0f)
        {
            return E_INVALIDARG;
        }

        m_hWnd = hWnd;
        try
        {
            m_metricsFont.strFontFamilyW = szFontFamilyW;
        }
        catch (const std::bad_alloc&)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_UNEXPECTED;
        }
        m_metricsFont.fFontSize = fFontSize;
        hr = CreateDeviceIndependentResources();
        if (FAILED(hr))
        {
            return hr;
        }
        RefreshDpi();
        hr = CreateTextFormatAndMetrics();
        if (FAILED(hr))
        {
            return hr;
        }
        if (GetClientRect(hWnd, &rcClient) == FALSE)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        hr = Resize(static_cast<UINT>(rcClient.right - rcClient.left), static_cast<UINT>(rcClient.bottom - rcClient.top));
        if (FAILED(hr))
        {
            return hr;
        }
        return CreateDeviceResources();
    }

    HRESULT Renderer::Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept
    {
        HRESULT hr;

        m_iClientWidth = static_cast<INT>(uiWidth);
        m_iClientHeight = static_cast<INT>(uiHeight);
        UpdateViewportLayout();
        if (m_renderTarget)
        {
            hr = m_renderTarget->Resize(D2D1::SizeU(uiWidth, uiHeight));
            if (FAILED(hr))
            {
                return hr;
            }
        }
        return S_OK;
    }

    HRESULT Renderer::Render(_In_ const Buffer& bufferGuiTerminal) noexcept
    {
        Buffer::Snapshot sSnapshotBuffer;
        INT iAttempt;
        HRESULT hr;

        for (iAttempt = 0; iAttempt < 2; ++iAttempt)
        {
            hr = CreateDeviceResources();
            if (FAILED(hr))
            {
                return hr;
            }
            hr = bufferGuiTerminal.GetSnapshot(&sSnapshotBuffer);
            if (FAILED(hr))
            {
                return hr;
            }

            m_iCols = sSnapshotBuffer.iCols;
            m_iRows = sSnapshotBuffer.iRows;
            UpdateViewportLayout();

            m_renderTarget->BeginDraw();
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            m_renderTarget->Clear(ToD2DColor(sSnapshotBuffer.crDefaultBackground));
            m_renderTarget->PushAxisAlignedClip(m_rcViewport, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            DrawCells(sSnapshotBuffer);
            m_renderTarget->PopAxisAlignedClip();
            DrawScrollBars(sSnapshotBuffer.crDefaultBackground);
            hr = m_renderTarget->EndDraw();
            if (hr != D2DERR_RECREATE_TARGET)
            {
                break;
            }

            m_brush.Reset();
            m_renderTarget.Reset();
        }
        return hr;
    }

    HRESULT Renderer::GetCellSize(_Out_ LPSIZE lpSize) const noexcept
    {
        if (!lpSize)
        {
            return E_POINTER;
        }
        lpSize->cx = static_cast<LONG>(std::lround(m_metricsFont.fCellWidth));
        lpSize->cy = static_cast<LONG>(std::lround(m_metricsFont.fCellHeight));
        return S_OK;
    }

    HRESULT Renderer::GetPreferredClientSize(_In_ INT iCols, _In_ INT iRows, _Out_ LPSIZE lpSize) const noexcept
    {
        if (!lpSize)
        {
            return E_POINTER;
        }
        lpSize->cx = static_cast<LONG>(std::lround(m_metricsFont.fCellWidth * static_cast<FLOAT>(iCols)));
        lpSize->cy = static_cast<LONG>(std::lround(m_metricsFont.fCellHeight * static_cast<FLOAT>(iRows)));
        return S_OK;
    }

    VOID Renderer::SetContentSize(_In_ INT iCols, _In_ INT iRows) noexcept
    {
        m_iCols = iCols;
        m_iRows = iRows;
        UpdateViewportLayout();
    }

    VOID Renderer::UpdateScrollBars() noexcept
    {
        UpdateViewportLayout();
    }

    BOOL Renderer::HasVisibleScrollBars() const noexcept
    {
        return ((m_scrollBarHorizontal.bVisible != FALSE) || (m_scrollBarVertical.bVisible != FALSE)) ? TRUE : FALSE;
    }

    BOOL Renderer::HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept
    {
        BOOL bHotHorizontalOld;
        BOOL bHotVerticalOld;
        BOOL bHitVertical;
        BOOL bHitThumb;

        bHotHorizontalOld = m_scrollBarHorizontal.bHot;
        bHotVerticalOld = m_scrollBarVertical.bHot;
        bHitVertical = FALSE;
        bHitThumb = FALSE;

        m_scrollBarHorizontal.bHot = FALSE;
        m_scrollBarVertical.bHot = FALSE;
        if (HitTestScrollBars(iX, iY, &bHitVertical, &bHitThumb) != FALSE)
        {
            if (bHitVertical != FALSE)
            {
                m_scrollBarVertical.bHot = TRUE;
            }
            else
            {
                m_scrollBarHorizontal.bHot = TRUE;
            }
        }
        return ((bHotHorizontalOld != m_scrollBarHorizontal.bHot) || (bHotVerticalOld != m_scrollBarVertical.bHot)) ? TRUE : FALSE;
    }

    BOOL Renderer::HandleMouseLeave() noexcept
    {
        BOOL bChanged;

        bChanged = ((m_scrollBarHorizontal.bHot != FALSE) || (m_scrollBarVertical.bHot != FALSE)) ? TRUE : FALSE;
        m_scrollBarHorizontal.bHot = FALSE;
        m_scrollBarVertical.bHot = FALSE;
        return bChanged;
    }

    BOOL Renderer::HitTestScrollBars(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbVertical, _Out_opt_ PBOOL lpbThumb) const noexcept
    {
        if (lpbVertical)
        {
            *lpbVertical = FALSE;
        }
        if (lpbThumb)
        {
            *lpbThumb = FALSE;
        }

        if ((m_scrollBarVertical.bVisible != FALSE) && IsPointInRect(iX, iY, m_scrollBarVertical.rcTrack) != FALSE)
        {
            if (lpbVertical)
            {
                *lpbVertical = TRUE;
            }
            if (lpbThumb)
            {
                *lpbThumb = IsPointInRect(iX, iY, m_scrollBarVertical.rcThumb);
            }
            return TRUE;
        }
        if ((m_scrollBarHorizontal.bVisible != FALSE) && IsPointInRect(iX, iY, m_scrollBarHorizontal.rcTrack) != FALSE)
        {
            if (lpbVertical)
            {
                *lpbVertical = FALSE;
            }
            if (lpbThumb)
            {
                *lpbThumb = IsPointInRect(iX, iY, m_scrollBarHorizontal.rcThumb);
            }
            return TRUE;
        }
        return FALSE;
    }

    BOOL Renderer::ScrollByTrackClick(_In_ BOOL bVertical, _In_ INT iPointerCoordinate) noexcept
    {
        if (bVertical != FALSE)
        {
            if (m_scrollBarVertical.bVisible == FALSE)
            {
                return FALSE;
            }
            if (static_cast<FLOAT>(iPointerCoordinate) < m_scrollBarVertical.rcThumb.top)
            {
                return ScrollByPage(TRUE, FALSE);
            }
            if (static_cast<FLOAT>(iPointerCoordinate) >= m_scrollBarVertical.rcThumb.bottom)
            {
                return ScrollByPage(TRUE, TRUE);
            }
            return FALSE;
        }
        if (m_scrollBarHorizontal.bVisible == FALSE)
        {
            return FALSE;
        }
        if (static_cast<FLOAT>(iPointerCoordinate) < m_scrollBarHorizontal.rcThumb.left)
        {
            return ScrollByPage(FALSE, FALSE);
        }
        if (static_cast<FLOAT>(iPointerCoordinate) >= m_scrollBarHorizontal.rcThumb.right)
        {
            return ScrollByPage(FALSE, TRUE);
        }
        return FALSE;
    }

    BOOL Renderer::ScrollByWheelDelta(_In_ SHORT iDelta) noexcept
    {
        INT iStep;
        INT iNewOffsetY;

        if (m_scrollBarVertical.bVisible == FALSE)
        {
            return FALSE;
        }
        iStep = (std::max)(static_cast<INT>(std::lround(m_metricsFont.fCellHeight * 3.0f)), 24);
        iNewOffsetY = m_iScrollOffsetY - static_cast<INT>(std::lround((static_cast<FLOAT>(iDelta) / static_cast<FLOAT>(WHEEL_DELTA)) *
                                                                       static_cast<FLOAT>(iStep)));
        return SetScrollOffset(m_iScrollOffsetX, iNewOffsetY);
    }

    BOOL Renderer::ScrollByPage(_In_ BOOL bVertical, _In_ BOOL bForward) noexcept
    {
        INT iPageSize;
        INT iDirection;

        iDirection = (bForward != FALSE) ? 1 : -1;
        if (bVertical != FALSE)
        {
            if (m_scrollBarVertical.bVisible == FALSE)
            {
                return FALSE;
            }
            iPageSize = (std::max)(m_scrollBarVertical.iViewportSize - static_cast<INT>(std::lround(m_metricsFont.fCellHeight * 2.0f)), 1);
            return SetScrollOffset(m_iScrollOffsetX, m_iScrollOffsetY + (iPageSize * iDirection));
        }
        if (m_scrollBarHorizontal.bVisible == FALSE)
        {
            return FALSE;
        }
        iPageSize = (std::max)(m_scrollBarHorizontal.iViewportSize - static_cast<INT>(std::lround(m_metricsFont.fCellWidth * 2.0f)), 1);
        return SetScrollOffset(m_iScrollOffsetX + (iPageSize * iDirection), m_iScrollOffsetY);
    }

    BOOL Renderer::SetScrollOffset(_In_ INT iOffsetX, _In_ INT iOffsetY) noexcept
    {
        INT iOffsetXClamped;
        INT iOffsetYClamped;
        BOOL bChanged;

        UpdateViewportLayout();
        iOffsetXClamped = (m_scrollBarHorizontal.bVisible != FALSE) ? ClampInt(iOffsetX, 0, m_scrollBarHorizontal.iMaxOffset) : 0;
        iOffsetYClamped = (m_scrollBarVertical.bVisible != FALSE) ? ClampInt(iOffsetY, 0, m_scrollBarVertical.iMaxOffset) : 0;
        bChanged = ((m_iScrollOffsetX != iOffsetXClamped) || (m_iScrollOffsetY != iOffsetYClamped)) ? TRUE : FALSE;
        m_iScrollOffsetX = iOffsetXClamped;
        m_iScrollOffsetY = iOffsetYClamped;
        UpdateViewportLayout();
        return bChanged;
    }

    INT Renderer::GetScrollOffsetX() const noexcept
    {
        return m_iScrollOffsetX;
    }

    INT Renderer::GetScrollOffsetY() const noexcept
    {
        return m_iScrollOffsetY;
    }

    BOOL Renderer::ScrollFromThumbDrag(_In_ BOOL bVertical, _In_ INT iPointerCoordinate, _In_ INT iPointerOrigin,
                                       _In_ INT iOffsetOrigin) noexcept
    {
        const ScrollBarMetrics& scrollBarMetrics = (bVertical != FALSE) ? m_scrollBarVertical : m_scrollBarHorizontal;
        INT iPointerDelta;
        INT iOffsetCurrent;

        if ((scrollBarMetrics.bVisible == FALSE) || (scrollBarMetrics.fThumbTravel <= 0.0f) || (scrollBarMetrics.iMaxOffset <= 0))
        {
            return FALSE;
        }
        iPointerDelta = iPointerCoordinate - iPointerOrigin;
        iOffsetCurrent = iOffsetOrigin +
                         static_cast<INT>(std::lround((static_cast<FLOAT>(iPointerDelta) / scrollBarMetrics.fThumbTravel) *
                                                      static_cast<FLOAT>(scrollBarMetrics.iMaxOffset)));
        if (bVertical != FALSE)
        {
            return SetScrollOffset(m_iScrollOffsetX, iOffsetCurrent);
        }
        return SetScrollOffset(iOffsetCurrent, m_iScrollOffsetY);
    }

    VOID Renderer::RefreshDpi() noexcept
    {
        UINT uiDpi;

        uiDpi = GetDpiForWindow(m_hWnd);
        m_fDpiX = static_cast<FLOAT>(uiDpi);
        m_fDpiY = static_cast<FLOAT>(uiDpi);
        m_iScrollBarThickness = (std::max)(static_cast<INT>(std::lround((12.0f * m_fDpiX) / 96.0f)), 10);
        UpdateViewportLayout();
        if (m_renderTarget)
        {
            m_renderTarget->SetDpi(m_fDpiX, m_fDpiY);
        }
    }

    HRESULT Renderer::CreateDeviceIndependentResources() noexcept
    {
        HRESULT hr;

        if (!m_d2dFactory)
        {
            hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }
        }
        if (!m_dwriteFactory)
        {
            hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                     reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
            if (FAILED(hr))
            {
                return hr;
            }
        }
        return S_OK;
    }

    HRESULT Renderer::CreateDeviceResources() noexcept
    {
        D2D1_SIZE_U sSizeRenderTarget;
        HRESULT hr;

        if (!m_renderTarget)
        {
            sSizeRenderTarget = D2D1::SizeU(static_cast<UINT>((std::max)(m_iClientWidth, 0)),
                                            static_cast<UINT>((std::max)(m_iClientHeight, 0)));

            hr = m_d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                                      D2D1::HwndRenderTargetProperties(m_hWnd, sSizeRenderTarget),
                                                      m_renderTarget.GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }

            m_renderTarget->SetDpi(m_fDpiX, m_fDpiY);
            hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), m_brush.GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }
        }
        return S_OK;
    }

    HRESULT Renderer::CreateTextFormatAndMetrics() noexcept
    {
        Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
        DWRITE_TEXT_METRICS textMetrics;
        DWRITE_LINE_METRICS lineMetrics;
        UINT32 uiActualLineCount;
        DWRITE_FONT_METRICS fontMetrics;
        Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection;
        UINT32 uiFamilyIndex;
        BOOL bExists;
        Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
        Microsoft::WRL::ComPtr<IDWriteFont> font;
        HRESULT hr;

        hr = m_dwriteFactory->GetSystemFontCollection(fontCollection.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        hr = fontCollection->FindFamilyName(m_metricsFont.strFontFamilyW.c_str(), &uiFamilyIndex, &bExists);
        if (FAILED(hr))
        {
            return hr;
        }
        if (bExists == FALSE)
        {
            return DWRITE_E_NOFONT;
        }
        hr = fontCollection->GetFontFamily(uiFamilyIndex, fontFamily.GetAddressOf());
        if (FAILED(hr))
        {
            return DWRITE_E_NOFONT;
        }
        hr = fontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                              font.GetAddressOf());
        if (FAILED(hr))
        {
            return DWRITE_E_NOFONT;
        }

        for (INT iStyle = 0; iStyle < 4; ++iStyle)
        {
            hr = m_dwriteFactory->CreateTextFormat(m_metricsFont.strFontFamilyW.c_str(), nullptr,
                                                   ((iStyle & 1) == 0) ? DWRITE_FONT_WEIGHT_NORMAL : DWRITE_FONT_WEIGHT_BOLD,
                                                   ((iStyle & 2) == 0) ? DWRITE_FONT_STYLE_NORMAL : DWRITE_FONT_STYLE_ITALIC,
                                                   DWRITE_FONT_STRETCH_NORMAL, m_metricsFont.fFontSize, L"",
                                                   m_textFormat[iStyle].GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }
            hr = m_textFormat[iStyle]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            if (FAILED(hr))
            {
                return hr;
            }
            hr = m_textFormat[iStyle]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            if (FAILED(hr))
            {
                return hr;
            }
            hr = m_textFormat[iStyle]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            if (FAILED(hr))
            {
                return hr;
            }
        }

        hr = m_dwriteFactory->CreateTextLayout(L"W", 1U, m_textFormat[0].Get(), 1024.0f, 1024.0f, textLayout.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        hr = textLayout->GetMetrics(&textMetrics);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = textLayout->GetLineMetrics(&lineMetrics, 1U, &uiActualLineCount);
        if (FAILED(hr))
        {
            return hr;
        }
        font->GetMetrics(&fontMetrics);
        m_metricsFont.fCellWidth = std::ceil(textMetrics.widthIncludingTrailingWhitespace);
        m_metricsFont.fCellHeight = std::ceil(lineMetrics.height);
        m_metricsFont.fBaseline =
            (static_cast<FLOAT>(fontMetrics.ascent) / static_cast<FLOAT>(fontMetrics.designUnitsPerEm)) * m_metricsFont.fFontSize;
        m_metricsFont.fUnderlineOffset =
            (static_cast<FLOAT>(fontMetrics.underlinePosition) / static_cast<FLOAT>(fontMetrics.designUnitsPerEm)) *
            m_metricsFont.fFontSize;
        m_metricsFont.fUnderlineThickness =
            (static_cast<FLOAT>(fontMetrics.underlineThickness) / static_cast<FLOAT>(fontMetrics.designUnitsPerEm)) *
            m_metricsFont.fFontSize;
        if (m_metricsFont.fUnderlineThickness < 1.0f)
        {
            m_metricsFont.fUnderlineThickness = 1.0f;
        }
        UpdateViewportLayout();
        return S_OK;
    }

    VOID Renderer::UpdateViewportLayout() noexcept
    {
        INT iContentWidth;
        INT iContentHeight;
        INT iViewportWidth;
        INT iViewportHeight;
        BOOL bVisibleHorizontal;
        BOOL bVisibleVertical;
        BOOL bVisibleHorizontalOld;
        BOOL bVisibleVerticalOld;

        iContentWidth = static_cast<INT>(std::lround(m_metricsFont.fCellWidth * static_cast<FLOAT>(m_iCols)));
        iContentHeight = static_cast<INT>(std::lround(m_metricsFont.fCellHeight * static_cast<FLOAT>(m_iRows)));
        bVisibleHorizontal = FALSE;
        bVisibleVertical = FALSE;
        do
        {
            bVisibleHorizontalOld = bVisibleHorizontal;
            bVisibleVerticalOld = bVisibleVertical;
            iViewportWidth = m_iClientWidth - ((bVisibleVertical != FALSE) ? m_iScrollBarThickness : 0);
            iViewportHeight = m_iClientHeight - ((bVisibleHorizontal != FALSE) ? m_iScrollBarThickness : 0);
            if (iViewportWidth < 0)
            {
                iViewportWidth = 0;
            }
            if (iViewportHeight < 0)
            {
                iViewportHeight = 0;
            }
            bVisibleHorizontal = (iContentWidth > iViewportWidth) ? TRUE : FALSE;
            bVisibleVertical = (iContentHeight > iViewportHeight) ? TRUE : FALSE;
        }
        while (bVisibleHorizontalOld != bVisibleHorizontal || bVisibleVerticalOld != bVisibleVertical);

        iViewportWidth = m_iClientWidth - ((bVisibleVertical != FALSE) ? m_iScrollBarThickness : 0);
        iViewportHeight = m_iClientHeight - ((bVisibleHorizontal != FALSE) ? m_iScrollBarThickness : 0);
        if (iViewportWidth < 0)
        {
            iViewportWidth = 0;
        }
        if (iViewportHeight < 0)
        {
            iViewportHeight = 0;
        }

        m_rcViewport.left = 0.0f;
        m_rcViewport.top = 0.0f;
        m_rcViewport.right = static_cast<FLOAT>(iViewportWidth);
        m_rcViewport.bottom = static_cast<FLOAT>(iViewportHeight);

        m_scrollBarHorizontal.bVisible = bVisibleHorizontal;
        m_scrollBarVertical.bVisible = bVisibleVertical;
        if (m_scrollBarHorizontal.bVisible == FALSE)
        {
            m_iScrollOffsetX = 0;
            m_scrollBarHorizontal.iOffset = 0;
            m_scrollBarHorizontal.bHot = FALSE;
        }
        if (m_scrollBarVertical.bVisible == FALSE)
        {
            m_iScrollOffsetY = 0;
            m_scrollBarVertical.iOffset = 0;
            m_scrollBarVertical.bHot = FALSE;
        }

        UpdateScrollBarMetrics(m_scrollBarHorizontal, FALSE);
        UpdateScrollBarMetrics(m_scrollBarVertical, TRUE);
        m_fGridOffsetX = m_rcViewport.left + (std::max)(0.0f, ((m_rcViewport.right - m_rcViewport.left) -
                                                               static_cast<FLOAT>(iContentWidth)) * 0.5f) -
                         static_cast<FLOAT>(m_iScrollOffsetX);
        m_fGridOffsetY = m_rcViewport.top + (std::max)(0.0f, ((m_rcViewport.bottom - m_rcViewport.top) -
                                                              static_cast<FLOAT>(iContentHeight)) * 0.5f) -
                         static_cast<FLOAT>(m_iScrollOffsetY);
    }

    VOID Renderer::UpdateScrollBarMetrics(_Inout_ ScrollBarMetrics& scrollBarMetrics, _In_ BOOL bVertical) noexcept
    {
        FLOAT fTrackLength;
        FLOAT fThumbLength;
        FLOAT fThumbPosition;
        FLOAT fInset;

        scrollBarMetrics.fThumbTravel = 0.0f;
        scrollBarMetrics.rcTrack = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        scrollBarMetrics.rcThumb = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        if (bVertical != FALSE)
        {
            scrollBarMetrics.iViewportSize = static_cast<INT>(std::lround(m_rcViewport.bottom - m_rcViewport.top));
            scrollBarMetrics.iContentSize = static_cast<INT>(std::lround(m_metricsFont.fCellHeight * static_cast<FLOAT>(m_iRows)));
            scrollBarMetrics.iMaxOffset = (std::max)(scrollBarMetrics.iContentSize - scrollBarMetrics.iViewportSize, 0);
            m_iScrollOffsetY = ClampInt(m_iScrollOffsetY, 0, scrollBarMetrics.iMaxOffset);
            scrollBarMetrics.iOffset = m_iScrollOffsetY;
            if (scrollBarMetrics.bVisible == FALSE)
            {
                return;
            }
            scrollBarMetrics.rcTrack = D2D1::RectF(m_rcViewport.right, 0.0f, m_rcViewport.right + static_cast<FLOAT>(m_iScrollBarThickness), m_rcViewport.bottom);
        }
        else
        {
            scrollBarMetrics.iViewportSize = static_cast<INT>(std::lround(m_rcViewport.right - m_rcViewport.left));
            scrollBarMetrics.iContentSize = static_cast<INT>(std::lround(m_metricsFont.fCellWidth * static_cast<FLOAT>(m_iCols)));
            scrollBarMetrics.iMaxOffset = (std::max)(scrollBarMetrics.iContentSize - scrollBarMetrics.iViewportSize, 0);
            m_iScrollOffsetX = ClampInt(m_iScrollOffsetX, 0, scrollBarMetrics.iMaxOffset);
            scrollBarMetrics.iOffset = m_iScrollOffsetX;
            if (scrollBarMetrics.bVisible == FALSE)
            {
                return;
            }
            scrollBarMetrics.rcTrack = D2D1::RectF(0.0f, m_rcViewport.bottom, m_rcViewport.right, m_rcViewport.bottom + static_cast<FLOAT>(m_iScrollBarThickness));
        }

        fTrackLength = (bVertical != FALSE) ? (scrollBarMetrics.rcTrack.bottom - scrollBarMetrics.rcTrack.top) :
                                              (scrollBarMetrics.rcTrack.right - scrollBarMetrics.rcTrack.left);
        if (scrollBarMetrics.iContentSize <= 0 || scrollBarMetrics.iViewportSize <= 0 || fTrackLength <= 0.0f)
        {
            return;
        }
        fThumbLength = (std::max)((fTrackLength * static_cast<FLOAT>(scrollBarMetrics.iViewportSize)) /
                                  static_cast<FLOAT>(scrollBarMetrics.iContentSize),
                                  static_cast<FLOAT>((std::max)(m_iScrollBarThickness * 2, 24)));
        if (fThumbLength > fTrackLength)
        {
            fThumbLength = fTrackLength;
        }
        scrollBarMetrics.fThumbTravel = fTrackLength - fThumbLength;
        fThumbPosition = (scrollBarMetrics.iMaxOffset > 0) ?
                         (scrollBarMetrics.fThumbTravel * static_cast<FLOAT>(scrollBarMetrics.iOffset)) /
                             static_cast<FLOAT>(scrollBarMetrics.iMaxOffset) :
                         0.0f;
        fInset = (std::max)(static_cast<FLOAT>(m_iScrollBarThickness) * 0.2f, 2.0f);
        if (bVertical != FALSE)
        {
            scrollBarMetrics.rcThumb = D2D1::RectF(scrollBarMetrics.rcTrack.left + fInset,
                                                   scrollBarMetrics.rcTrack.top + fThumbPosition + fInset,
                                                   scrollBarMetrics.rcTrack.right - fInset,
                                                   scrollBarMetrics.rcTrack.top + fThumbPosition + fThumbLength - fInset);
        }
        else
        {
            scrollBarMetrics.rcThumb = D2D1::RectF(scrollBarMetrics.rcTrack.left + fThumbPosition + fInset,
                                                   scrollBarMetrics.rcTrack.top + fInset,
                                                   scrollBarMetrics.rcTrack.left + fThumbPosition + fThumbLength - fInset,
                                                   scrollBarMetrics.rcTrack.bottom - fInset);
        }
    }

    VOID Renderer::DrawScrollBars(_In_ COLORREF crDefaultBackground) noexcept
    {
        D2D1_ROUNDED_RECT rcThumbRounded;
        D2D1_RECT_F rcCorner;
        D2D1_COLOR_F colorTrack;
        D2D1_COLOR_F colorThumb;
        D2D1_COLOR_F colorThumbHot;
        FLOAT fThumbRadius;
        BOOL bDarkMode;

        bDarkMode = (GetColorLuminance(crDefaultBackground) < 0.5f) ? TRUE : FALSE;
        colorTrack = (bDarkMode != FALSE) ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f) : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.08f);
        colorThumb = (bDarkMode != FALSE) ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.34f) : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.28f);
        colorThumbHot = (bDarkMode != FALSE) ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.52f) : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.42f);

        if (m_scrollBarVertical.bVisible != FALSE)
        {
            m_brush->SetColor(colorTrack);
            m_renderTarget->FillRectangle(m_scrollBarVertical.rcTrack, m_brush.Get());
            fThumbRadius = (std::min)((m_scrollBarVertical.rcThumb.right - m_scrollBarVertical.rcThumb.left) * 0.5f,
                                      (m_scrollBarVertical.rcThumb.bottom - m_scrollBarVertical.rcThumb.top) * 0.5f);
            rcThumbRounded = D2D1::RoundedRect(m_scrollBarVertical.rcThumb, fThumbRadius, fThumbRadius);
            m_brush->SetColor((m_scrollBarVertical.bHot != FALSE) ? colorThumbHot : colorThumb);
            m_renderTarget->FillRoundedRectangle(rcThumbRounded, m_brush.Get());
        }

        if (m_scrollBarHorizontal.bVisible != FALSE)
        {
            m_brush->SetColor(colorTrack);
            m_renderTarget->FillRectangle(m_scrollBarHorizontal.rcTrack, m_brush.Get());
            fThumbRadius = (std::min)((m_scrollBarHorizontal.rcThumb.right - m_scrollBarHorizontal.rcThumb.left) * 0.5f,
                                      (m_scrollBarHorizontal.rcThumb.bottom - m_scrollBarHorizontal.rcThumb.top) * 0.5f);
            rcThumbRounded = D2D1::RoundedRect(m_scrollBarHorizontal.rcThumb, fThumbRadius, fThumbRadius);
            m_brush->SetColor((m_scrollBarHorizontal.bHot != FALSE) ? colorThumbHot : colorThumb);
            m_renderTarget->FillRoundedRectangle(rcThumbRounded, m_brush.Get());
        }

        if ((m_scrollBarHorizontal.bVisible != FALSE) && (m_scrollBarVertical.bVisible != FALSE))
        {
            rcCorner = D2D1::RectF(m_rcViewport.right, m_rcViewport.bottom, static_cast<FLOAT>(m_iClientWidth),
                                   static_cast<FLOAT>(m_iClientHeight));
            m_brush->SetColor(colorTrack);
            m_renderTarget->FillRectangle(rcCorner, m_brush.Get());
        }
    }

    VOID Renderer::DrawCells(_In_ const Buffer::Snapshot& sSnapshotBuffer) noexcept
    {
        INT iRow;
        INT iCol;
        size_t iIndex;

        for (iRow = 0; iRow < sSnapshotBuffer.iRows; ++iRow)
        {
            for (iCol = 0; iCol < sSnapshotBuffer.iCols; ++iCol)
            {
                iIndex = static_cast<size_t>(iRow * sSnapshotBuffer.iCols + iCol);
                DrawCell(sSnapshotBuffer.lpCells[iIndex], iCol, iRow, sSnapshotBuffer);
            }
        }
    }

    VOID Renderer::DrawCell(_In_ const Buffer::Cell& sCellCurrent, _In_ INT iCol, _In_ INT iRow,
                            _In_ const Buffer::Snapshot& sSnapshotBuffer) noexcept
    {
        D2D1_RECT_F rcBackground;
        D2D1_RECT_F rcText;
        D2D1_RECT_F rcUnderline;
        COLORREF crForeground;
        COLORREF crBackground;
        FLOAT flOriginX;
        FLOAT flOriginY;
        BOOL bTextVisible;

        if ((sCellCurrent.dwStyleFlags & Control::StyleInverse) == 0)
        {
            crForeground = sCellCurrent.crForeground;
            crBackground = sCellCurrent.crBackground;
        }
        else
        {
            crForeground = sCellCurrent.crBackground;
            crBackground = sCellCurrent.crForeground;
        }
        flOriginX = m_fGridOffsetX + (m_metricsFont.fCellWidth * static_cast<FLOAT>(iCol));
        flOriginY = m_fGridOffsetY + (m_metricsFont.fCellHeight * static_cast<FLOAT>(iRow));
        rcBackground = D2D1::RectF(flOriginX, flOriginY, flOriginX + m_metricsFont.fCellWidth, flOriginY + m_metricsFont.fCellHeight);
        m_brush->SetColor(ToD2DColor(crBackground));
        m_renderTarget->FillRectangle(rcBackground, m_brush.Get());

        bTextVisible = (((sCellCurrent.dwStyleFlags & Control::StyleBlink) == 0U) ||
                        (sSnapshotBuffer.bBlinkVisible != FALSE)) ? TRUE : FALSE;
        if ((bTextVisible != FALSE) && (sCellCurrent.chCodepointW != L' '))
        {
            INT iStyle;

            iStyle = 0;
            if ((sCellCurrent.dwStyleFlags & Control::StyleBold) != 0U)
            {
                iStyle |= 1;
            }
            if ((sCellCurrent.dwStyleFlags & Control::StyleItalic) != 0U)
            {
                iStyle |= 2;
            }

            rcText = D2D1::RectF(flOriginX, flOriginY, flOriginX + m_metricsFont.fCellWidth, flOriginY + m_metricsFont.fCellHeight);
            m_brush->SetColor(ToD2DColor(crForeground));
            m_renderTarget->DrawTextW(&sCellCurrent.chCodepointW, 1, m_textFormat[iStyle].Get(), rcText, m_brush.Get(),
                                      D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_GDI_CLASSIC);
        }

        if ((sCellCurrent.dwStyleFlags & Control::StyleUnderline) != 0U)
        {
            rcUnderline = D2D1::RectF(flOriginX, flOriginY + m_metricsFont.fBaseline - m_metricsFont.fUnderlineOffset,
                                      flOriginX + m_metricsFont.fCellWidth,
                                      flOriginY + m_metricsFont.fBaseline - m_metricsFont.fUnderlineOffset +
                                      m_metricsFont.fUnderlineThickness);
            m_brush->SetColor(ToD2DColor(crForeground));
            m_renderTarget->FillRectangle(rcUnderline, m_brush.Get());
        }
    }
}

// -----------------------------------------------------------------------------

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor) noexcept
{
    return D2D1::ColorF(static_cast<FLOAT>(GetRValue(crColor)) / 255.0f,
                        static_cast<FLOAT>(GetGValue(crColor)) / 255.0f,
                        static_cast<FLOAT>(GetBValue(crColor)) / 255.0f,
                        1.0f);
}

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor, _In_ FLOAT fAlpha) noexcept
{
    return D2D1::ColorF(static_cast<FLOAT>(GetRValue(crColor)) / 255.0f,
                        static_cast<FLOAT>(GetGValue(crColor)) / 255.0f,
                        static_cast<FLOAT>(GetBValue(crColor)) / 255.0f,
                        fAlpha);
}

static FLOAT GetColorLuminance(_In_ COLORREF crColor) noexcept
{
    return ((0.2126f * static_cast<FLOAT>(GetRValue(crColor))) +
            (0.7152f * static_cast<FLOAT>(GetGValue(crColor))) +
            (0.0722f * static_cast<FLOAT>(GetBValue(crColor)))) / 255.0f;
}

static BOOL IsPointInRect(_In_ INT iX, _In_ INT iY, _In_ const D2D1_RECT_F& rcCurrent) noexcept
{
    return ((static_cast<FLOAT>(iX) >= rcCurrent.left) && (static_cast<FLOAT>(iX) < rcCurrent.right) &&
            (static_cast<FLOAT>(iY) >= rcCurrent.top) && (static_cast<FLOAT>(iY) < rcCurrent.bottom)) ? TRUE : FALSE;
}

static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept
{
    return (std::max)(iMinimum, (std::min)(iValue, iMaximumValue));
}
