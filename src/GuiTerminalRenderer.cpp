#include "..\include\GuiTerminalRenderer.h"
#include "..\include\GuiTerminalControl.h"
#include <cmath>

// -----------------------------------------------------------------------------

static LPCWSTR szDefaultCandidatesW[4] = { L"Consolas", L"Cascadia Mono", L"Segoe UI Mono", L"Segoe UI" };

// -----------------------------------------------------------------------------

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal::Internals
{
    HRESULT Renderer::Initialize(_In_ HWND hWnd) noexcept
    {
        RECT rcClient;
        HRESULT hr;

        m_hWnd = hWnd;
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
        hr = CreateDeviceResources();
        if (FAILED(hr))
        {
            return hr;
        }
        return Resize(static_cast<UINT>(rcClient.right - rcClient.left), static_cast<UINT>(rcClient.bottom - rcClient.top));
    }

    HRESULT Renderer::Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept
    {
        HRESULT hr;

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
        HRESULT hr;

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

        m_renderTarget->BeginDraw();
        m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_renderTarget->Clear(ToD2DColor(sSnapshotBuffer.crDefaultBackground));
        DrawCells(sSnapshotBuffer);
        hr = m_renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET)
        {
            m_brush.Reset();
            m_renderTarget.Reset();
            hr = S_OK;
        }
        return hr;
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

    VOID Renderer::RefreshDpi() noexcept
    {
        UINT uiDpi;

        uiDpi = GetDpiForWindow(m_hWnd);
        m_fDpiX = static_cast<FLOAT>(uiDpi);
        m_fDpiY = static_cast<FLOAT>(uiDpi);
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
        RECT rcClient;
        D2D1_SIZE_U sSizeRenderTarget;
        HRESULT hr;

        if (!m_renderTarget)
        {
            if (GetClientRect(m_hWnd, &rcClient) == FALSE)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            sSizeRenderTarget = D2D1::SizeU(static_cast<UINT>(rcClient.right - rcClient.left),
                                            static_cast<UINT>(rcClient.bottom - rcClient.top));

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
        LPCWSTR szFontFamilyW;
        Microsoft::WRL::ComPtr<IDWriteTextLayout> lpTextLayout;
        DWRITE_TEXT_METRICS textMetrics;
        DWRITE_LINE_METRICS lineMetrics;
        UINT32 uiActualLineCount;
        DWRITE_FONT_METRICS fontMetrics;
        Microsoft::WRL::ComPtr<IDWriteFontCollection> lpFontCollection;
        UINT32 uiFamilyIndex;
        BOOL bExists;
        Microsoft::WRL::ComPtr<IDWriteFontFamily> lpFontFamily;
        Microsoft::WRL::ComPtr<IDWriteFont> lpFont;
        HRESULT hr;

        hr = ChooseFontFamily(&szFontFamilyW);
        if (FAILED(hr))
        {
            return hr;
        }
        for (INT iStyle = 0; iStyle < 4; ++iStyle)
        {
            hr = m_dwriteFactory->CreateTextFormat(szFontFamilyW, nullptr,
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

        hr = m_dwriteFactory->CreateTextLayout(L"W", 1U, m_textFormat[0].Get(), 1024.0f, 1024.0f, lpTextLayout.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        hr = lpTextLayout->GetMetrics(&textMetrics);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = lpTextLayout->GetLineMetrics(&lineMetrics, 1U, &uiActualLineCount);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = m_dwriteFactory->GetSystemFontCollection(lpFontCollection.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        hr = lpFontCollection->FindFamilyName(szFontFamilyW, &uiFamilyIndex, &bExists);
        if (FAILED(hr) || (bExists == FALSE))
        {
            return FAILED(hr) ? hr : E_FAIL;
        }
        hr = lpFontCollection->GetFontFamily(uiFamilyIndex, lpFontFamily.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        hr = lpFontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                                lpFont.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        lpFont->GetMetrics(&fontMetrics);
        m_metricsFont.szFontFamilyW = szFontFamilyW;
        m_metricsFont.fCellWidth = std::ceil(textMetrics.widthIncludingTrailingWhitespace);
        m_metricsFont.fCellHeight = std::ceil(lineMetrics.height);
        m_metricsFont.fBaseline = (static_cast<FLOAT>(fontMetrics.ascent) / static_cast<FLOAT>(fontMetrics.designUnitsPerEm)) * m_metricsFont.fFontSize;
        m_metricsFont.fUnderlineOffset =
            (static_cast<FLOAT>(fontMetrics.underlinePosition) / static_cast<FLOAT>(fontMetrics.designUnitsPerEm)) * m_metricsFont.fFontSize;
        m_metricsFont.fUnderlineThickness =
            (static_cast<FLOAT>(fontMetrics.underlineThickness) / static_cast<FLOAT>(fontMetrics.designUnitsPerEm)) * m_metricsFont.fFontSize;
        if (m_metricsFont.fUnderlineThickness < 1.0f)
        {
            m_metricsFont.fUnderlineThickness = 1.0f;
        }
        return S_OK;
    }

    HRESULT Renderer::ChooseFontFamily(_Out_ LPCWSTR *lpszFontFamilyW) noexcept
    {
        Microsoft::WRL::ComPtr<IDWriteFontCollection> lpFontCollection;
        UINT32 uiFamilyIndex;
        BOOL bExists;
        INT iIndex;
        HRESULT hr;

        *lpszFontFamilyW = nullptr;

        hr = m_dwriteFactory->GetSystemFontCollection(lpFontCollection.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
        for (iIndex = 0; iIndex < 4; ++iIndex)
        {
            bExists = FALSE;
            hr = lpFontCollection->FindFamilyName(szDefaultCandidatesW[iIndex], &uiFamilyIndex, &bExists);
            if (FAILED(hr))
            {
                return hr;
            }
            if (bExists != FALSE)
            {
                *lpszFontFamilyW = szDefaultCandidatesW[iIndex];
                return S_OK;
            }
        }
        *lpszFontFamilyW = L"Segoe UI";
        return S_OK;
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

        if ((sCellCurrent.dwStyleFlags & Control::StyleInverse) == 0U)
        {
            crForeground = sCellCurrent.crForeground;
            crBackground = sCellCurrent.crBackground;
        }
        else
        {
            crForeground = sCellCurrent.crBackground;
            crBackground = sCellCurrent.crForeground;
        }
        flOriginX = m_metricsFont.fCellWidth * static_cast<FLOAT>(iCol);
        flOriginY = m_metricsFont.fCellHeight * static_cast<FLOAT>(iRow);
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

