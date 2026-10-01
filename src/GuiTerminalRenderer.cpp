#include "..\include\GuiTerminalRenderer.h"
#include "..\include\GuiTerminalControl.h"
#include <algorithm>
#include <cmath>

// #define DEBUG_SHOW_PERF_INFO

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// -----------------------------------------------------------------------------

static LARGE_INTEGER liPerfFreq = [] {
    LARGE_INTEGER li;

    ::QueryPerformanceFrequency(&li);
    return li;
}();

// -----------------------------------------------------------------------------

#if defined(DEBUG_SHOW_PERF_INFO)

namespace GuiTerminal {

namespace Internals {

class CPerformanceTimer
{
  public:
    CPerformanceTimer() noexcept = default;
    CPerformanceTimer(_In_ const CPerformanceTimer &) = delete;
    CPerformanceTimer(_Inout_ CPerformanceTimer &&) = delete;
    ~CPerformanceTimer() noexcept = default;

    CPerformanceTimer &operator=(_In_ const CPerformanceTimer &) = delete;
    CPerformanceTimer &operator=(_Inout_ CPerformanceTimer &&) = delete;

    VOID Start()
    {
        ::QueryPerformanceCounter(&liStart);
    }

    ULONGLONG Mark()
    {
        ULONGLONG ullMark;

        ::QueryPerformanceCounter(&liEnd);
        ullMark = ((liEnd.QuadPart - liStart.QuadPart) * 1000000ULL) / liPerfFreq.QuadPart;
        liStart.QuadPart = liEnd.QuadPart;
        return ullMark;
    }

  private:
    LARGE_INTEGER liStart, liEnd;
};

} // namespace Internals

} // namespace GuiTerminal

#endif // DEBUG_SHOW_PERF_INFO

// -----------------------------------------------------------------------------

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor) noexcept;
static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor, _In_ FLOAT fAlpha) noexcept;
static FLOAT GetColorLuminance(_In_ COLORREF crColor) noexcept;
static BOOL IsPointInRect(_In_ INT iX, _In_ INT iY, _In_ const RECT &rcCurrent) noexcept;
static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept;
static BOOL AreCellsEqual(_In_ const GuiTerminal::Internals::Buffer::Cell &sCellFirst,
                          _In_ const GuiTerminal::Internals::Buffer::Cell &sCellSecond) noexcept;
static BOOL IntersectCellRects(_In_ const GuiTerminal::Internals::CellRect_t &sRectFirst,
                               _In_ const GuiTerminal::Internals::CellRect_t &sRectSecond) noexcept;
static VOID IncludeCellRect(_Inout_ GuiTerminal::Internals::CellRect_t &sRectTarget,
                            _In_ const GuiTerminal::Internals::CellRect_t &sRectSource) noexcept;

#if defined(DEBUG_SHOW_PERF_INFO)
static VOID OutputPerformanceInfo(_In_z_ LPCSTR szOperationA, _In_ ULONGLONG ullDurationUs) noexcept
{
    CHAR szBufA[128];

    sprintf_s(szBufA, "%s: Total=%I64u\n", szOperationA, ullDurationUs);
    OutputDebugStringA(szBufA);
}
#endif // DEBUG_SHOW_PERF_INFO

// -----------------------------------------------------------------------------

GuiTerminal::DrawContext::DrawContext(_In_ Internals::Renderer *lpRenderer, _In_ INT iWidth, _In_ INT iHeight) noexcept
    : m_lpRenderer(lpRenderer), m_iWidth(iWidth), m_iHeight(iHeight)
{
}

VOID GuiTerminal::DrawContext::Clear(_In_ COLORREF crColor) noexcept
{
    m_lpRenderer->DrawContextClear(crColor);
}

VOID GuiTerminal::DrawContext::DrawPoint(_In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor, _In_ FLOAT fRadius) noexcept
{
    m_lpRenderer->DrawContextPoint(fX, fY, crColor, fRadius);
}

VOID GuiTerminal::DrawContext::DrawLine(_In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2, _In_ FLOAT fY2, _In_ COLORREF crColor,
                                        _In_ FLOAT fStrokeWidth) noexcept
{
    m_lpRenderer->DrawContextLine(fX1, fY1, fX2, fY2, crColor, fStrokeWidth);
}

VOID GuiTerminal::DrawContext::DrawRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth) noexcept
{
    m_lpRenderer->DrawContextRectangle(rcRectangle, crColor, fStrokeWidth, FALSE);
}

VOID GuiTerminal::DrawContext::FillRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor) noexcept
{
    m_lpRenderer->DrawContextRectangle(rcRectangle, crColor, 0.0f, TRUE);
}

VOID GuiTerminal::DrawContext::DrawRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor,
                                                    _In_ FLOAT fStrokeWidth) noexcept
{
    m_lpRenderer->DrawContextRoundedRectangle(rcRectangle, crColor, fStrokeWidth, FALSE);
}

VOID GuiTerminal::DrawContext::FillRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor) noexcept
{
    m_lpRenderer->DrawContextRoundedRectangle(rcRectangle, crColor, 0.0f, TRUE);
}

VOID GuiTerminal::DrawContext::DrawEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth) noexcept
{
    m_lpRenderer->DrawContextEllipse(ellipse, crColor, fStrokeWidth, FALSE);
}

VOID GuiTerminal::DrawContext::FillEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor) noexcept
{
    m_lpRenderer->DrawContextEllipse(ellipse, crColor, 0.0f, TRUE);
}

VOID GuiTerminal::DrawContext::Write(_In_z_ LPCWSTR szTextW, _In_ const D2D1_POINT_2F &pointReference, _In_ COLORREF crColor,
                                     _In_opt_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize, _In_ DWORD dwStyleFlags,
                                     _In_ TextAlignment textAlignment, _In_ FLOAT fRotationDegrees) noexcept
{
    m_lpRenderer->DrawContextWrite(szTextW, pointReference, crColor, szFontFamilyW, fFontSize, dwStyleFlags, textAlignment,
                                   fRotationDegrees);
}

VOID GuiTerminal::DrawContext::BeginPath() noexcept
{
    m_lpRenderer->BeginPath();
}

VOID GuiTerminal::DrawContext::MoveTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept
{
    m_lpRenderer->MovePathTo(fX, fY);
}

VOID GuiTerminal::DrawContext::LineTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept
{
    m_lpRenderer->AddPathLine(fX, fY);
}

VOID GuiTerminal::DrawContext::QuadraticBezierTo(_In_ FLOAT fControlX, _In_ FLOAT fControlY, _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept
{
    m_lpRenderer->AddPathQuadraticBezier(fControlX, fControlY, fEndX, fEndY);
}

VOID GuiTerminal::DrawContext::CubicBezierTo(_In_ FLOAT fControl1X, _In_ FLOAT fControl1Y, _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y,
                                             _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept
{
    m_lpRenderer->AddPathCubicBezier(fControl1X, fControl1Y, fControl2X, fControl2Y, fEndX, fEndY);
}

VOID GuiTerminal::DrawContext::ArcTo(_In_ FLOAT fEndX, _In_ FLOAT fEndY, _In_ FLOAT fRadiusX, _In_ FLOAT fRadiusY,
                                     _In_ FLOAT fRotationDegrees, _In_ D2D1_SWEEP_DIRECTION sweepDirection,
                                     _In_ D2D1_ARC_SIZE arcSize) noexcept
{
    m_lpRenderer->AddPathArc(fEndX, fEndY, fRadiusX, fRadiusY, fRotationDegrees, sweepDirection, arcSize);
}

VOID GuiTerminal::DrawContext::ClosePath() noexcept
{
    m_lpRenderer->ClosePath();
}

VOID GuiTerminal::DrawContext::StrokePath(_In_ COLORREF crColor, _In_ FLOAT fStrokeWidth) noexcept
{
    m_lpRenderer->StrokePath(crColor, fStrokeWidth);
}

VOID GuiTerminal::DrawContext::FillPath(_In_ COLORREF crColor) noexcept
{
    m_lpRenderer->FillPath(crColor);
}

INT GuiTerminal::DrawContext::GetWidth() const noexcept
{
    return m_iWidth;
}

INT GuiTerminal::DrawContext::GetHeight() const noexcept
{
    return m_iHeight;
}

UINT GuiTerminal::DrawContext::GetDeviceGeneration() const noexcept
{
    return m_lpRenderer->GetDeviceGeneration();
}

ID2D1RenderTarget *GuiTerminal::DrawContext::GetDirect2DRenderTarget() const noexcept
{
    return m_lpRenderer->GetRenderTarget();
}

// -----------------------------------------------------------------------------

namespace GuiTerminal::Internals {

Renderer::~Renderer() noexcept
{
    DiscardDeviceResources();
}

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
    catch (const std::bad_alloc &)
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
    return S_OK;
}

HRESULT Renderer::Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept
{
    m_iClientWidth = static_cast<INT>(uiWidth);
    m_iClientHeight = static_cast<INT>(uiHeight);
    UpdateViewportLayout();
    InvalidateComposition();
    return S_OK;
}

HRESULT Renderer::Render(_In_ const Buffer &bufferGuiTerminal) noexcept
{
#if defined(DEBUG_SHOW_PERF_INFO)
    CPerformanceTimer cPerfTimer;
#endif // DEBUG_SHOW_PERF_INFO
    Buffer::Snapshot sSnapshotBuffer;
    std::vector<Buffer::RenderItem> vecRenderItems;
    std::vector<INT> vecCustomDrawRegionIds;
    CellRect_t sRectDirtyCells;
    CellRect_t sRectCurrent;
    CellRect_t sRectCustomCoverage;
    RECT rcDirtyPixels;
    BOOL bFullRedraw;
    BOOL bTerminalDirty;
    BOOL bBlinkChanged;
    BOOL bTargetLost;
    INT iAttempt;
    INT iCol;
    INT iRow;
    size_t uIndex;
    HRESULT hr;

    for (iAttempt = 0; iAttempt < 2; iAttempt++)
    {
        hr = CreateDeviceResources();
        if (hr == S_FALSE)
        {
            return S_OK;
        }
        if (FAILED(hr))
        {
            return hr;
        }
        hr = bufferGuiTerminal.GetSnapshot(&sSnapshotBuffer);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = bufferGuiTerminal.GetRenderPlan(&vecRenderItems);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = bufferGuiTerminal.GetCustomDrawRegionIds(&vecCustomDrawRegionIds);
        if (FAILED(hr))
        {
            return hr;
        }
        // Keep only bitmap caches whose custom regions still exist in the buffer.
        PruneCustomDrawCaches(vecCustomDrawRegionIds);

        // Check whether structural state invalidates the entire composed terminal.
        bFullRedraw = m_bCompositionInvalid;
        if (m_iCachedCols != sSnapshotBuffer.iCols || m_iCachedRows != sSnapshotBuffer.iRows)
        {
            bFullRedraw = TRUE;
        }
        m_iCols = sSnapshotBuffer.iCols;
        m_iRows = sSnapshotBuffer.iRows;
        UpdateViewportLayout();

        sRectDirtyCells = CellRect_t{};
        bBlinkChanged = (m_bCachedBlinkVisible != sSnapshotBuffer.bBlinkVisible) ? TRUE : FALSE;
        if (bFullRedraw == FALSE)
        {
            // Cell attributes already contain resolved colors; the default background only matters during full clears and scrollbar draws.
            for (iRow = 0; iRow < sSnapshotBuffer.iRows; ++iRow)
            {
                for (iCol = 0; iCol < sSnapshotBuffer.iCols; ++iCol)
                {
                    uIndex = static_cast<size_t>(iRow * sSnapshotBuffer.iCols + iCol);
                    if (m_vecCachedCells[uIndex].bDirty != FALSE ||
                        AreCellsEqual(m_vecCachedCells[uIndex].sCell, sSnapshotBuffer.lpCells[uIndex]) == FALSE ||
                        (bBlinkChanged != FALSE && m_vecCachedCells[uIndex].bHasBlink != FALSE))
                    {
                        m_vecCachedCells[uIndex].bDirty = TRUE;
                        sRectCurrent = CellRect_t{iCol, iRow, 1, 1};
                        IncludeCellRect(sRectDirtyCells, sRectCurrent);
                    }
                }
            }

            if (m_bCachedCursorVisible != sSnapshotBuffer.bCursorVisible || m_iCachedCursorCol != sSnapshotBuffer.iCursorCol ||
                m_iCachedCursorRow != sSnapshotBuffer.iCursorRow || m_dwCachedCursorStyle != sSnapshotBuffer.dwCursorStyle ||
                bBlinkChanged != FALSE)
            {
                if (m_bCachedCursorVisible != FALSE)
                {
                    sRectCurrent = CellRect_t{m_iCachedCursorCol, m_iCachedCursorRow, 1, 1};
                    IncludeCellRect(sRectDirtyCells, sRectCurrent);
                }
                if (sSnapshotBuffer.bCursorVisible != FALSE)
                {
                    sRectCurrent = CellRect_t{sSnapshotBuffer.iCursorCol, sSnapshotBuffer.iCursorRow, 1, 1};
                    IncludeCellRect(sRectDirtyCells, sRectCurrent);
                }
            }

            for (const Buffer::RenderItem &sRenderItem : vecRenderItems)
            {
                if (sRenderItem.lpsRegion->bCustomDraw != FALSE)
                {
                    const auto itCache = m_mapCustomDrawCaches.find(sRenderItem.lpsRegion->iId);

                    if (itCache == m_mapCustomDrawCaches.end() || itCache->second.bDirty != FALSE)
                    {
                        IncludeCellRect(sRectDirtyCells, sRenderItem.sVisibleRect);
                    }
                }
            }
        }
        if (bFullRedraw != FALSE)
        {
            sRectDirtyCells = CellRect_t{0, 0, sSnapshotBuffer.iCols, sSnapshotBuffer.iRows};
        }
        else if (sRectDirtyCells.iWidth > 0 && sRectDirtyCells.iHeight > 0)
        {
            const INT iLeft = (std::max)(sRectDirtyCells.iX - 1, 0);
            const INT iTop = (std::max)(sRectDirtyCells.iY - 1, 0);
            const INT iRight = (std::min)(sRectDirtyCells.iX + sRectDirtyCells.iWidth + 1, sSnapshotBuffer.iCols);
            const INT iBottom = (std::min)(sRectDirtyCells.iY + sRectDirtyCells.iHeight + 1, sSnapshotBuffer.iRows);

            sRectDirtyCells.iX = iLeft;
            sRectDirtyCells.iY = iTop;
            sRectDirtyCells.iWidth = iRight - iLeft;
            sRectDirtyCells.iHeight = iBottom - iTop;
        }
        bTerminalDirty = (sRectDirtyCells.iWidth > 0 && sRectDirtyCells.iHeight > 0) ? TRUE : FALSE;
        if (bTerminalDirty == FALSE && m_bScrollBarsDirty == FALSE)
        {
            return S_FALSE;
        }

        // Refresh custom bitmaps only when their own cache entry requires it.
        bTargetLost = FALSE;
        for (const Buffer::RenderItem &sRenderItem : vecRenderItems)
        {
            if (sRenderItem.lpsRegion->bCustomDraw != FALSE)
            {
                hr = UpdateCustomDrawCache(sRenderItem);
                if (FAILED(hr))
                {
                    if (hr == D2DERR_RECREATE_TARGET)
                    {
                        const_cast<Buffer &>(bufferGuiTerminal)
                            .NotifyCustomDrawResourceCleanup(CustomDrawResourceCleanupReason::TargetLost);
                        DiscardDeviceResources();
                        bTargetLost = TRUE;
                        break;
                    }
                    return hr;
                }
            }
        }
        if (bTargetLost != FALSE)
        {
            continue;
        }

        m_uDirtyCount = 0U;
        m_bFrameFull = bFullRedraw;
        m_renderTarget->SetTarget(m_compositionBitmap.Get());
        m_renderTarget->SetDpi(m_fDpiX, m_fDpiY);

#if defined(DEBUG_SHOW_PERF_INFO)
        cPerfTimer.Start();
#endif // DEBUG_SHOW_PERF_INFO
        m_renderTarget->BeginDraw();
#if defined(DEBUG_SHOW_PERF_INFO)
        OutputPerformanceInfo("BeginDraw (Composition)", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO

        m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        if (bFullRedraw != FALSE)
        {
            m_renderTarget->Clear(ToD2DColor(sSnapshotBuffer.crDefaultBackground));
        }
        if (bTerminalDirty != FALSE)
        {
            // Convert dirty terminal cells into client pixels using the grid origin and cell dimensions.
            // Clamp that area to the viewport so it never redraws behind the scroll bars.
            rcDirtyPixels.left = m_iGridOffsetX + (sRectDirtyCells.iX * m_metricsFont.iCellWidthPx);
            rcDirtyPixels.top = m_iGridOffsetY + (sRectDirtyCells.iY * m_metricsFont.iCellHeightPx);
            rcDirtyPixels.right = m_iGridOffsetX + ((sRectDirtyCells.iX + sRectDirtyCells.iWidth) * m_metricsFont.iCellWidthPx);
            rcDirtyPixels.bottom = m_iGridOffsetY + ((sRectDirtyCells.iY + sRectDirtyCells.iHeight) * m_metricsFont.iCellHeightPx);
            rcDirtyPixels.left = (std::max)(rcDirtyPixels.left, m_rcViewport.left);
            rcDirtyPixels.top = (std::max)(rcDirtyPixels.top, m_rcViewport.top);
            rcDirtyPixels.right = (std::min)(rcDirtyPixels.right, m_rcViewport.right);
            rcDirtyPixels.bottom = (std::min)(rcDirtyPixels.bottom, m_rcViewport.bottom);
            if (rcDirtyPixels.left < rcDirtyPixels.right && rcDirtyPixels.top < rcDirtyPixels.bottom)
            {
                m_renderTarget->PushAxisAlignedClip(D2D1::RectF(PixelsToDipsX(rcDirtyPixels.left), PixelsToDipsY(rcDirtyPixels.top),
                                                                PixelsToDipsX(rcDirtyPixels.right), PixelsToDipsY(rcDirtyPixels.bottom)),
                                                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                // The snapshot contains the final normal-cell composition for this dirty area.
                DrawCells(sSnapshotBuffer, sRectDirtyCells);
                sRectCustomCoverage = CellRect_t{};
                for (const Buffer::RenderItem &sRenderItem : vecRenderItems)
                {
                    if (sRenderItem.lpsRegion->bCustomDraw != FALSE)
                    {
                        if (IntersectCellRects(sRenderItem.sVisibleRect, sRectDirtyCells) != FALSE)
                        {
                            DrawCustomRegion(sRenderItem);
                            sRectCurrent.iX = (std::max)(sRenderItem.sVisibleRect.iX, sRectDirtyCells.iX);
                            sRectCurrent.iY = (std::max)(sRenderItem.sVisibleRect.iY, sRectDirtyCells.iY);
                            sRectCurrent.iWidth = (std::min)(sRenderItem.sVisibleRect.iX + sRenderItem.sVisibleRect.iWidth,
                                                             sRectDirtyCells.iX + sRectDirtyCells.iWidth) -
                                                  sRectCurrent.iX;
                            sRectCurrent.iHeight = (std::min)(sRenderItem.sVisibleRect.iY + sRenderItem.sVisibleRect.iHeight,
                                                              sRectDirtyCells.iY + sRectDirtyCells.iHeight) -
                                                   sRectCurrent.iY;
                            IncludeCellRect(sRectCustomCoverage, sRectCurrent);
                        }
                    }
                    // Restore later normal layers where a preceding custom bitmap may have covered them.
                    else if (IntersectCellRects(sRenderItem.sVisibleRect, sRectCustomCoverage) != FALSE)
                    {
                        DrawRegionCells(sRenderItem, sSnapshotBuffer, sRectCustomCoverage);
                    }
                }
                DrawCursor(sSnapshotBuffer);
                m_renderTarget->PopAxisAlignedClip();
                AddDirtyRect(rcDirtyPixels);
            }
        }
        if (m_bScrollBarsDirty != FALSE)
        {
            ClearScrollBarAreas(sSnapshotBuffer.crDefaultBackground);
            DrawScrollBars(sSnapshotBuffer.crDefaultBackground);
            if (m_scrollBarVertical.bVisible != FALSE)
            {
                AddDirtyRect(m_scrollBarVertical.rcTrack);
            }
            if (m_scrollBarHorizontal.bVisible != FALSE)
            {
                AddDirtyRect(m_scrollBarHorizontal.rcTrack);
            }
            if (m_scrollBarVertical.bVisible != FALSE && m_scrollBarHorizontal.bVisible != FALSE)
            {
                AddDirtyRect(RECT{m_rcViewport.right, m_rcViewport.bottom, m_iClientWidth, m_iClientHeight});
            }
        }

#if defined(DEBUG_SHOW_PERF_INFO)
        OutputPerformanceInfo("Draw", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO
        hr = m_renderTarget->EndDraw();
#if defined(DEBUG_SHOW_PERF_INFO)
        OutputPerformanceInfo("EndDraw (Composition)", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO

        if (FAILED(hr))
        {
            if (hr != D2DERR_RECREATE_TARGET)
            {
                return hr;
            }

            const_cast<Buffer &>(bufferGuiTerminal).NotifyCustomDrawResourceCleanup(CustomDrawResourceCleanupReason::TargetLost);
            DiscardDeviceResources();
            continue;
        }
        if (m_uDirtyCount != 0U)
        {
            m_renderTarget->SetTarget(m_targetBitmap.Get());
            m_renderTarget->SetDpi(m_fDpiX, m_fDpiY);

#if defined(DEBUG_SHOW_PERF_INFO)
            cPerfTimer.Start();
#endif // DEBUG_SHOW_PERF_INFO
            m_renderTarget->BeginDraw();
#if defined(DEBUG_SHOW_PERF_INFO)
            OutputPerformanceInfo("BeginDraw (Target)", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO

            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            m_renderTarget->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
            for (UINT uDirtyIndex = 0U; uDirtyIndex < m_uDirtyCount; ++uDirtyIndex)
            {
                const D2D1_RECT_F rcDestination =
                    D2D1::RectF(PixelsToDipsX(m_rcDirty[uDirtyIndex].left), PixelsToDipsY(m_rcDirty[uDirtyIndex].top),
                                PixelsToDipsX(m_rcDirty[uDirtyIndex].right), PixelsToDipsY(m_rcDirty[uDirtyIndex].bottom));
                const D2D1_RECT_F rcSource = rcDestination;

                m_renderTarget->DrawBitmap(m_compositionBitmap.Get(), rcDestination, 1.0f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                                           rcSource, nullptr);
            }
            m_renderTarget->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);

#if defined(DEBUG_SHOW_PERF_INFO)
            cPerfTimer.Start();
#endif // DEBUG_SHOW_PERF_INFO
            hr = m_renderTarget->EndDraw();
#if defined(DEBUG_SHOW_PERF_INFO)
            OutputPerformanceInfo("EndDraw (Target)", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO

            if (FAILED(hr))
            {
                if (hr != D2DERR_RECREATE_TARGET)
                {
                    return hr;
                }

                const_cast<Buffer &>(bufferGuiTerminal).NotifyCustomDrawResourceCleanup(CustomDrawResourceCleanupReason::TargetLost);
                DiscardDeviceResources();
                continue;
            }
        }
        try
        {
            m_vecCachedCells.resize(static_cast<size_t>(sSnapshotBuffer.iCols * sSnapshotBuffer.iRows));
            for (uIndex = 0U; uIndex < m_vecCachedCells.size(); ++uIndex)
            {
                m_vecCachedCells[uIndex].sCell = sSnapshotBuffer.lpCells[uIndex];
                m_vecCachedCells[uIndex].bDirty = FALSE;
                m_vecCachedCells[uIndex].bHasBlink =
                    ((sSnapshotBuffer.lpCells[uIndex].dwStyleFlags & Control::StyleBlink) != 0U) ? TRUE : FALSE;
            }
        }
        catch (const std::bad_alloc &)
        {
            InvalidateComposition();
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            InvalidateComposition();
            return E_UNEXPECTED;
        }
        m_iCachedCols = sSnapshotBuffer.iCols;
        m_iCachedRows = sSnapshotBuffer.iRows;
        m_bCachedBlinkVisible = sSnapshotBuffer.bBlinkVisible;
        m_bCachedCursorVisible = sSnapshotBuffer.bCursorVisible;
        m_iCachedCursorCol = sSnapshotBuffer.iCursorCol;
        m_iCachedCursorRow = sSnapshotBuffer.iCursorRow;
        m_dwCachedCursorStyle = sSnapshotBuffer.dwCursorStyle;
        m_bCompositionInvalid = FALSE;
        m_bScrollBarsDirty = FALSE;
        m_bFrameReady = (m_uDirtyCount != 0U) ? TRUE : FALSE;
        hr = S_OK;
        break;
    }
    return hr;
}

HRESULT Renderer::Present() noexcept
{
    DXGI_PRESENT_PARAMETERS sPresentParameters;
    HRESULT hr;
#if defined(DEBUG_SHOW_PERF_INFO)
    CPerformanceTimer cPerfTimer;
#endif

    if (!m_swapChain || m_bFrameReady == FALSE)
    {
        return S_FALSE;
    }

    sPresentParameters = DXGI_PRESENT_PARAMETERS{};
    if (m_bFrameFull == FALSE)
    {
        sPresentParameters.DirtyRectsCount = m_uDirtyCount;
        sPresentParameters.pDirtyRects = m_rcDirty;
    }
#if defined(DEBUG_SHOW_PERF_INFO)
    cPerfTimer.Start();
#endif
    hr = m_swapChain->Present1(1U, 0U, &sPresentParameters);
#if defined(DEBUG_SHOW_PERF_INFO)
    OutputPerformanceInfo("Present", cPerfTimer.Mark());
#endif
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        DiscardDeviceResources();
    }
    else if (FAILED(hr))
    {
        InvalidateComposition();
    }
    else
    {
        m_bFrameReady = FALSE;
        m_bWaitForFrameLatency = TRUE;
    }
    return hr;
}

VOID Renderer::DrawRegionCells(_In_ const Buffer::RenderItem &sRenderItem, _In_ const Buffer::Snapshot &sSnapshotBuffer,
                               _In_ const CellRect_t &sRectDirty) noexcept
{
    INT iTerminalX;
    INT iTerminalY;
    INT iLocalX;
    INT iLocalY;
    INT iStartX;
    INT iStartY;
    INT iEndX;
    INT iEndY;
    size_t uIndex;

#if defined(DEBUG_SHOW_PERF_INFO)
    CPerformanceTimer cPerfTimer;

    cPerfTimer.Start();
#endif

    iStartX = (std::max)(sRenderItem.sVisibleRect.iX, sRectDirty.iX);
    iStartY = (std::max)(sRenderItem.sVisibleRect.iY, sRectDirty.iY);
    iEndX = (std::min)(sRenderItem.sVisibleRect.iX + sRenderItem.sVisibleRect.iWidth, sRectDirty.iX + sRectDirty.iWidth);
    iEndY = (std::min)(sRenderItem.sVisibleRect.iY + sRenderItem.sVisibleRect.iHeight, sRectDirty.iY + sRectDirty.iHeight);
    if (iStartX < iEndX && iStartY < iEndY)
    {
        for (iTerminalY = iStartY; iTerminalY < iEndY; ++iTerminalY)
        {
            for (iTerminalX = iStartX; iTerminalX < iEndX; ++iTerminalX)
            {
                iLocalX = static_cast<INT>(static_cast<LONGLONG>(iTerminalX) - sRenderItem.llOriginX);
                iLocalY = static_cast<INT>(static_cast<LONGLONG>(iTerminalY) - sRenderItem.llOriginY);
                uIndex = static_cast<size_t>(iLocalY * sRenderItem.lpsRegion->iWidth + iLocalX);
                DrawCell(sRenderItem.lpsRegion->vecCells[uIndex], iTerminalX, iTerminalY, sSnapshotBuffer);
            }
        }
    }

#if defined(DEBUG_SHOW_PERF_INFO)
    CHAR szBufA[128];
    const ULONGLONG ullMark = cPerfTimer.Mark();
    const ULONGLONG ullCellCount = static_cast<ULONGLONG>(iEndX - iStartX) * static_cast<ULONGLONG>(iEndY - iStartY);

    sprintf_s(szBufA, "DrawRegion: Total=%I64u / PerCell=%I64u\n", ullMark, ullMark / ullCellCount);
    OutputDebugStringA(szBufA);
#endif
}

VOID Renderer::DrawCustomRegion(_In_ const Buffer::RenderItem &sRenderItem) noexcept
{
    const Region_t &sRegion = *sRenderItem.lpsRegion;
    const auto itCache = m_mapCustomDrawCaches.find(sRegion.iId);
    D2D1_RECT_F rcDestination;
    D2D1_RECT_F rcSource;
    INT iSourceX;
    INT iSourceY;

    if (itCache == m_mapCustomDrawCaches.end() || !itCache->second.bitmap)
    {
        return;
    }
    iSourceX = static_cast<INT>(static_cast<LONGLONG>(sRenderItem.sVisibleRect.iX) - sRenderItem.llOriginX);
    iSourceY = static_cast<INT>(static_cast<LONGLONG>(sRenderItem.sVisibleRect.iY) - sRenderItem.llOriginY);
    rcDestination = D2D1::RectF(
        PixelsToDipsX(m_iGridOffsetX + (m_metricsFont.iCellWidthPx * sRenderItem.sVisibleRect.iX)),
        PixelsToDipsY(m_iGridOffsetY + (m_metricsFont.iCellHeightPx * sRenderItem.sVisibleRect.iY)),
        PixelsToDipsX(m_iGridOffsetX + (m_metricsFont.iCellWidthPx * (sRenderItem.sVisibleRect.iX + sRenderItem.sVisibleRect.iWidth))),
        PixelsToDipsY(m_iGridOffsetY + (m_metricsFont.iCellHeightPx * (sRenderItem.sVisibleRect.iY + sRenderItem.sVisibleRect.iHeight))));
    rcSource =
        D2D1::RectF(static_cast<FLOAT>(iSourceX * m_metricsFont.iCellWidthPx), static_cast<FLOAT>(iSourceY * m_metricsFont.iCellHeightPx),
                    static_cast<FLOAT>((iSourceX + sRenderItem.sVisibleRect.iWidth) * m_metricsFont.iCellWidthPx),
                    static_cast<FLOAT>((iSourceY + sRenderItem.sVisibleRect.iHeight) * m_metricsFont.iCellHeightPx));
    m_renderTarget->DrawBitmap(itCache->second.bitmap.Get(), rcDestination, 1.0f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, rcSource);
}

HRESULT Renderer::UpdateCustomDrawCache(_In_ const Buffer::RenderItem &sRenderItem) noexcept
{
#if defined(DEBUG_SHOW_PERF_INFO)
    CPerformanceTimer cPerfTimer;
#endif // DEBUG_SHOW_PERF_INFO
    const Region_t &sRegion = *sRenderItem.lpsRegion;
    CustomDrawCache *lpsCache;
    D2D1_BITMAP_PROPERTIES1 sBitmapProperties;
    DrawContext drawContext(this, sRegion.iWidth * m_metricsFont.iCellWidthPx, sRegion.iHeight * m_metricsFont.iCellHeightPx);
    UINT uiWidth;
    UINT uiHeight;
    HRESULT hr;

    try
    {
        lpsCache = &m_mapCustomDrawCaches[sRegion.iId];
    }
    catch (const std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }
    uiWidth = static_cast<UINT>(sRegion.iWidth * m_metricsFont.iCellWidthPx);
    uiHeight = static_cast<UINT>(sRegion.iHeight * m_metricsFont.iCellHeightPx);
    if (lpsCache->bitmap && lpsCache->uiWidth == uiWidth && lpsCache->uiHeight == uiHeight && lpsCache->bDirty == FALSE)
    {
        return S_OK;
    }
    lpsCache->bitmap.Reset();
    lpsCache->uiWidth = uiWidth;
    lpsCache->uiHeight = uiHeight;
    lpsCache->bDirty = TRUE;
    sBitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
                                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f, 96.0f);
    hr = m_renderTarget->CreateBitmap(D2D1::SizeU(uiWidth, uiHeight), nullptr, 0U, &sBitmapProperties, lpsCache->bitmap.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    // Render the callback into its local bitmap so normal composition can reuse it unchanged.
    m_renderTarget->SetTarget(lpsCache->bitmap.Get());
    m_renderTarget->SetDpi(96.0f, 96.0f);

#if defined(DEBUG_SHOW_PERF_INFO)
    cPerfTimer.Start();
#endif // DEBUG_SHOW_PERF_INFO
    m_renderTarget->BeginDraw();
#if defined(DEBUG_SHOW_PERF_INFO)
    OutputPerformanceInfo("BeginDraw (Custom)", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO

    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    sRegion.fnCustomDrawCallback(drawContext, const_cast<Region_t *>(&sRegion));

#if defined(DEBUG_SHOW_PERF_INFO)
    cPerfTimer.Start();
#endif // DEBUG_SHOW_PERF_INFO
    hr = m_renderTarget->EndDraw();
#if defined(DEBUG_SHOW_PERF_INFO)
    OutputPerformanceInfo("EndDraw (Custom)", cPerfTimer.Mark());
#endif // DEBUG_SHOW_PERF_INFO

    m_renderTarget->SetTarget(nullptr);
    if (FAILED(hr))
    {
        return hr;
    }
    lpsCache->bDirty = FALSE;
    return S_OK;
}

VOID Renderer::PruneCustomDrawCaches(_In_ const std::vector<INT> &vecCustomDrawRegionIds) noexcept
{
    auto itCache = m_mapCustomDrawCaches.begin();

    while (itCache != m_mapCustomDrawCaches.end())
    {
        if (std::find(vecCustomDrawRegionIds.begin(), vecCustomDrawRegionIds.end(), itCache->first) == vecCustomDrawRegionIds.end())
        {
            itCache = m_mapCustomDrawCaches.erase(itCache);
        }
        else
        {
            ++itCache;
        }
    }
}

VOID Renderer::DrawContextClear(_In_ COLORREF crColor) noexcept
{
    m_renderTarget->Clear(ToD2DColor(crColor));
}

VOID Renderer::DrawContextPoint(_In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor, _In_ FLOAT fRadius) noexcept
{
    if (fRadius > 0.0f)
    {
        m_brush->SetColor(ToD2DColor(crColor));
        m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(fX, fY), fRadius, fRadius), m_brush.Get());
    }
}

VOID Renderer::DrawContextLine(_In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2, _In_ FLOAT fY2, _In_ COLORREF crColor,
                               _In_ FLOAT fStrokeWidth) noexcept
{
    m_brush->SetColor(ToD2DColor(crColor));
    m_renderTarget->DrawLine(D2D1::Point2F(fX1, fY1), D2D1::Point2F(fX2, fY2), m_brush.Get(), fStrokeWidth);
}

VOID Renderer::DrawContextRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                                    _In_ BOOL bFill) noexcept
{
    m_brush->SetColor(ToD2DColor(crColor));
    if (bFill)
    {
        m_renderTarget->FillRectangle(rcRectangle, m_brush.Get());
    }
    else
    {
        m_renderTarget->DrawRectangle(rcRectangle, m_brush.Get(), fStrokeWidth);
    }
}

VOID Renderer::DrawContextRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                                           _In_ BOOL bFill) noexcept
{
    m_brush->SetColor(ToD2DColor(crColor));
    if (bFill)
    {
        m_renderTarget->FillRoundedRectangle(rcRectangle, m_brush.Get());
    }
    else
    {
        m_renderTarget->DrawRoundedRectangle(rcRectangle, m_brush.Get(), fStrokeWidth);
    }
}

VOID Renderer::DrawContextEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                                  _In_ BOOL bFill) noexcept
{
    m_brush->SetColor(ToD2DColor(crColor));
    if (bFill)
    {
        m_renderTarget->FillEllipse(ellipse, m_brush.Get());
    }
    else
    {
        m_renderTarget->DrawEllipse(ellipse, m_brush.Get(), fStrokeWidth);
    }
}

VOID Renderer::DrawContextWrite(_In_z_ LPCWSTR szTextW, _In_ const D2D1_POINT_2F &pointReference, _In_ COLORREF crColor,
                                _In_opt_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize, _In_ DWORD dwStyleFlags,
                                _In_ DrawContext::TextAlignment textAlignment, _In_ FLOAT fRotationDegrees) noexcept
{
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
    DWRITE_TEXT_METRICS textMetrics;
    DWRITE_LINE_METRICS lineMetrics;
    DWRITE_FONT_WEIGHT fontWeight;
    DWRITE_FONT_STYLE fontStyle;
    D2D1_POINT_2F pointOrigin;
    D2D1_MATRIX_3X2_F transformPrevious;
    FLOAT fSizeDips;
    HRESULT hr;
    UINT32 uiLineCount;

    if ((!szTextW) || *szTextW == 0)
    {
        return;
    }
    fSizeDips = ((fFontSize > 0.0f) ? fFontSize : m_metricsFont.fFontSize) * 96.0f / 72.0f;
    fontWeight = ((dwStyleFlags & Control::StyleBold) != 0U) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    fontStyle = ((dwStyleFlags & Control::StyleItalic) != 0U) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    hr =
        m_dwriteFactory->CreateTextFormat((szFontFamilyW && *szFontFamilyW) ? szFontFamilyW : m_metricsFont.strFontFamilyW.c_str(), nullptr,
                                          fontWeight, fontStyle, DWRITE_FONT_STRETCH_NORMAL, fSizeDips, L"", textFormat.GetAddressOf());
    if (FAILED(hr))
    {
        return;
    }
    hr = m_dwriteFactory->CreateTextLayout(szTextW, static_cast<UINT32>(wcslen(szTextW)), textFormat.Get(), 1000000.0f, 1000000.0f,
                                           textLayout.GetAddressOf());
    uiLineCount = 0U;
    if (FAILED(hr) || FAILED(textLayout->GetMetrics(&textMetrics)) || FAILED(textLayout->GetLineMetrics(&lineMetrics, 1U, &uiLineCount)) ||
        (uiLineCount == 0U))
    {
        return;
    }
    pointOrigin = pointReference;
    switch (static_cast<DWORD>(textAlignment) & 3U)
    {
        case DrawContext::AlignCenter:
            pointOrigin.x -= textMetrics.widthIncludingTrailingWhitespace / 2.0f;
            break;
        case DrawContext::AlignRight:
            pointOrigin.x -= textMetrics.widthIncludingTrailingWhitespace;
            break;
    }
    switch (static_cast<DWORD>(textAlignment) & (3U << 2))
    {
        case DrawContext::AlignTop:
            pointOrigin.y -= textMetrics.top;
            break;
        case DrawContext::AlignMiddle:
            pointOrigin.y -= textMetrics.top + (textMetrics.height / 2.0f);
            break;
        case DrawContext::AlignBottom:
            pointOrigin.y -= textMetrics.top + textMetrics.height;
            break;
        case DrawContext::AlignBaseline:
            pointOrigin.y -= lineMetrics.baseline;
            break;
    }
    m_brush->SetColor(ToD2DColor(crColor));
    m_renderTarget->GetTransform(&transformPrevious);
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(fRotationDegrees, pointReference) * transformPrevious);
    m_renderTarget->DrawTextLayout(pointOrigin, textLayout.Get(), m_brush.Get());
    m_renderTarget->SetTransform(transformPrevious);
}

VOID Renderer::BeginPath() noexcept
{
    m_pathSink.Reset();
    m_pathGeometry.Reset();
    m_bPathFigureOpen = FALSE;
    m_bPathClosed = FALSE;
    if (FAILED(m_d2dFactory->CreatePathGeometry(m_pathGeometry.GetAddressOf())) || FAILED(m_pathGeometry->Open(m_pathSink.GetAddressOf())))
    {
        m_pathSink.Reset();
        m_pathGeometry.Reset();
    }
}

VOID Renderer::MovePathTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept
{
    if (!m_pathSink || m_bPathFigureOpen != FALSE)
    {
        return;
    }
    m_pathSink->BeginFigure(D2D1::Point2F(fX, fY), D2D1_FIGURE_BEGIN_FILLED);
    m_bPathFigureOpen = TRUE;
}

VOID Renderer::AddPathLine(_In_ FLOAT fX, _In_ FLOAT fY) noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE)
    {
        m_pathSink->AddLine(D2D1::Point2F(fX, fY));
    }
}

VOID Renderer::AddPathQuadraticBezier(_In_ FLOAT fControlX, _In_ FLOAT fControlY, _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE)
    {
        m_pathSink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(D2D1::Point2F(fControlX, fControlY), D2D1::Point2F(fEndX, fEndY)));
    }
}

VOID Renderer::AddPathCubicBezier(_In_ FLOAT fControl1X, _In_ FLOAT fControl1Y, _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y,
                                  _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE)
    {
        m_pathSink->AddBezier(
            D2D1::BezierSegment(D2D1::Point2F(fControl1X, fControl1Y), D2D1::Point2F(fControl2X, fControl2Y), D2D1::Point2F(fEndX, fEndY)));
    }
}

VOID Renderer::AddPathArc(_In_ FLOAT fEndX, _In_ FLOAT fEndY, _In_ FLOAT fRadiusX, _In_ FLOAT fRadiusY, _In_ FLOAT fRotationDegrees,
                          _In_ D2D1_SWEEP_DIRECTION sweepDirection, _In_ D2D1_ARC_SIZE arcSize) noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE && fRadiusX > 0.0f && fRadiusY > 0.0f)
    {
        m_pathSink->AddArc(
            D2D1::ArcSegment(D2D1::Point2F(fEndX, fEndY), D2D1::SizeF(fRadiusX, fRadiusY), fRotationDegrees, sweepDirection, arcSize));
    }
}

VOID Renderer::ClosePath() noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE)
    {
        m_pathSink->EndFigure(D2D1_FIGURE_END_CLOSED);
        m_bPathFigureOpen = FALSE;
        m_bPathClosed = TRUE;
    }
}

VOID Renderer::StrokePath(_In_ COLORREF crColor, _In_ FLOAT fStrokeWidth) noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE)
    {
        m_pathSink->EndFigure(D2D1_FIGURE_END_OPEN);
        m_bPathFigureOpen = FALSE;
    }
    if (m_pathSink && SUCCEEDED(m_pathSink->Close()))
    {
        m_pathSink.Reset();
    }
    if (m_pathGeometry && fStrokeWidth > 0.0f)
    {
        m_brush->SetColor(ToD2DColor(crColor));
        m_renderTarget->DrawGeometry(m_pathGeometry.Get(), m_brush.Get(), fStrokeWidth);
    }
}

VOID Renderer::FillPath(_In_ COLORREF crColor) noexcept
{
    if (m_pathSink && m_bPathFigureOpen != FALSE)
    {
        m_pathSink->EndFigure(D2D1_FIGURE_END_OPEN);
        m_bPathFigureOpen = FALSE;
    }
    if (m_pathSink && SUCCEEDED(m_pathSink->Close()))
    {
        m_pathSink.Reset();
    }
    if (m_pathGeometry)
    {
        m_brush->SetColor(ToD2DColor(crColor));
        m_renderTarget->FillGeometry(m_pathGeometry.Get(), m_brush.Get());
    }
}

UINT Renderer::GetDeviceGeneration() const noexcept
{
    return m_uiDeviceGeneration;
}

HANDLE Renderer::ConsumeFrameLatencyWaitableObject() noexcept
{
    HANDLE hFrameLatencyWaitableObject;

    if (m_bWaitForFrameLatency == FALSE)
    {
        return nullptr;
    }
    m_bWaitForFrameLatency = FALSE;
    hFrameLatencyWaitableObject = m_hFrameLatencyWaitableObject;
    return hFrameLatencyWaitableObject;
}

VOID Renderer::InvalidateCustomDrawRegion(_In_opt_ RegionHandle hRegion) noexcept
{
    if (!hRegion)
    {
        // A null handle means device-independent custom content changed everywhere.
        for (auto &sCachePair : m_mapCustomDrawCaches)
        {
            sCachePair.second.bDirty = TRUE;
        }
        return;
    }
    if (const auto itCache = m_mapCustomDrawCaches.find(hRegion->iId); itCache != m_mapCustomDrawCaches.end())
    {
        itCache->second.bDirty = TRUE;
    }
}

ID2D1RenderTarget *Renderer::GetRenderTarget() const noexcept
{
    return m_renderTarget.Get();
}

BOOL Renderer::GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept
{
    if (!lprcCell)
    {
        return FALSE;
    }
    lprcCell->left = 0;
    lprcCell->top = 0;
    lprcCell->right = 0;
    lprcCell->bottom = 0;

    if (iCol < 0 || iCol >= m_iCols || iRow < 0 || iRow >= m_iRows)
    {
        return FALSE;
    }

    lprcCell->left = m_iGridOffsetX + (m_metricsFont.iCellWidthPx * iCol);
    lprcCell->top = m_iGridOffsetY + (m_metricsFont.iCellHeightPx * iRow);
    lprcCell->right = lprcCell->left + m_metricsFont.iCellWidthPx;
    lprcCell->bottom = lprcCell->top + m_metricsFont.iCellHeightPx;
    return TRUE;
}

HRESULT Renderer::GetCellSize(_Out_ LPSIZE lpSize) const noexcept
{
    if (!lpSize)
    {
        return E_POINTER;
    }
    lpSize->cx = static_cast<LONG>(m_metricsFont.iCellWidthPx);
    lpSize->cy = static_cast<LONG>(m_metricsFont.iCellHeightPx);
    return S_OK;
}

HRESULT Renderer::GetPreferredClientSize(_In_ INT iCols, _In_ INT iRows, _Out_ LPSIZE lpSize) const noexcept
{
    if (!lpSize)
    {
        return E_POINTER;
    }
    lpSize->cx = static_cast<LONG>((std::max)(iCols, 0) * m_metricsFont.iCellWidthPx);
    lpSize->cy = static_cast<LONG>((std::max)(iRows, 0) * m_metricsFont.iCellHeightPx);
    return S_OK;
}

VOID Renderer::SetContentSize(_In_ INT iCols, _In_ INT iRows) noexcept
{
    m_iCols = iCols;
    m_iRows = iRows;
    UpdateViewportLayout();
    InvalidateComposition();
}

VOID Renderer::UpdateScrollBars() noexcept
{
    UpdateViewportLayout();
    InvalidateComposition();
}

BOOL Renderer::HasVisibleScrollBars() const noexcept
{
    return (m_scrollBarHorizontal.bVisible != FALSE || m_scrollBarVertical.bVisible != FALSE) ? TRUE : FALSE;
}

BOOL Renderer::HitTestCell(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept
{
    INT iContentX;
    INT iContentY;
    INT iCol;
    INT iRow;

    if (lpiCol)
    {
        *lpiCol = 0;
    }
    if (lpiRow)
    {
        *lpiRow = 0;
    }

    if (m_iCols <= 0 || m_iRows <= 0 || m_metricsFont.iCellWidthPx <= 0 || m_metricsFont.iCellHeightPx <= 0)
    {
        return FALSE;
    }
    if (iX < m_rcViewport.left || iX >= m_rcViewport.right || iY < m_rcViewport.top || iY >= m_rcViewport.bottom)
    {
        return FALSE;
    }

    iContentX = iX - m_iGridOffsetX;
    iContentY = iY - m_iGridOffsetY;
    if (iContentX < 0 || iContentY < 0 || iContentX >= m_metricsFont.iCellWidthPx * m_iCols ||
        iContentY >= m_metricsFont.iCellHeightPx * m_iRows)
    {
        return FALSE;
    }

    iCol = ClampInt(iContentX / m_metricsFont.iCellWidthPx, 0, m_iCols - 1);
    iRow = ClampInt(iContentY / m_metricsFont.iCellHeightPx, 0, m_iRows - 1);
    if (lpiCol)
    {
        *lpiCol = iCol;
    }
    if (lpiRow)
    {
        *lpiRow = iRow;
    }
    return TRUE;
}

BOOL Renderer::HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept
{
    BOOL bHotHorizontalOld;
    BOOL bHotVerticalOld;
    BOOL bHitVertical;
    BOOL bHitThumb;
    BOOL bChanged;

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
    bChanged = (bHotHorizontalOld != m_scrollBarHorizontal.bHot || bHotVerticalOld != m_scrollBarVertical.bHot) ? TRUE : FALSE;
    if (bChanged != FALSE)
    {
        m_bScrollBarsDirty = TRUE;
    }
    return bChanged;
}

BOOL Renderer::HandleMouseLeave() noexcept
{
    BOOL bChanged;

    bChanged = (m_scrollBarHorizontal.bHot != FALSE || m_scrollBarVertical.bHot != FALSE) ? TRUE : FALSE;
    m_scrollBarHorizontal.bHot = FALSE;
    m_scrollBarVertical.bHot = FALSE;
    if (bChanged != FALSE)
    {
        m_bScrollBarsDirty = TRUE;
    }
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

    if (m_scrollBarVertical.bVisible != FALSE && IsPointInRect(iX, iY, m_scrollBarVertical.rcTrack) != FALSE)
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
    if (m_scrollBarHorizontal.bVisible != FALSE && IsPointInRect(iX, iY, m_scrollBarHorizontal.rcTrack) != FALSE)
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
        if (iPointerCoordinate < m_scrollBarVertical.rcThumb.top)
        {
            return ScrollByPage(TRUE, FALSE);
        }
        if (iPointerCoordinate >= m_scrollBarVertical.rcThumb.bottom)
        {
            return ScrollByPage(TRUE, TRUE);
        }
        return FALSE;
    }
    if (m_scrollBarHorizontal.bVisible == FALSE)
    {
        return FALSE;
    }
    if (iPointerCoordinate < m_scrollBarHorizontal.rcThumb.left)
    {
        return ScrollByPage(FALSE, FALSE);
    }
    if (iPointerCoordinate >= m_scrollBarHorizontal.rcThumb.right)
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
    iStep = (std::max)(m_metricsFont.iCellHeightPx * 3, 24);
    iNewOffsetY = m_iScrollOffsetY -
                  static_cast<INT>(std::lround((static_cast<FLOAT>(iDelta) / static_cast<FLOAT>(WHEEL_DELTA)) * static_cast<FLOAT>(iStep)));
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
        iPageSize = (std::max)(m_scrollBarVertical.iViewportSize - (m_metricsFont.iCellHeightPx * 2), 1);
        return SetScrollOffset(m_iScrollOffsetX, m_iScrollOffsetY + (iPageSize * iDirection));
    }
    if (m_scrollBarHorizontal.bVisible == FALSE)
    {
        return FALSE;
    }
    iPageSize = (std::max)(m_scrollBarHorizontal.iViewportSize - (m_metricsFont.iCellWidthPx * 2), 1);
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
    bChanged = (m_iScrollOffsetX != iOffsetXClamped || m_iScrollOffsetY != iOffsetYClamped) ? TRUE : FALSE;
    m_iScrollOffsetX = iOffsetXClamped;
    m_iScrollOffsetY = iOffsetYClamped;
    UpdateViewportLayout();
    if (bChanged != FALSE)
    {
        InvalidateComposition();
    }
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
    const ScrollBarMetrics &scrollBarMetrics = (bVertical != FALSE) ? m_scrollBarVertical : m_scrollBarHorizontal;
    INT iPointerDelta;
    INT iOffsetCurrent;

    if (scrollBarMetrics.bVisible == FALSE || scrollBarMetrics.iThumbTravel <= 0 || scrollBarMetrics.iMaxOffset <= 0)
    {
        return FALSE;
    }
    // Map pointer travel along the thumb track back to the content scroll range.
    iPointerDelta = iPointerCoordinate - iPointerOrigin;
    iOffsetCurrent = iOffsetOrigin +
                     static_cast<INT>(std::lround((static_cast<FLOAT>(iPointerDelta) / static_cast<FLOAT>(scrollBarMetrics.iThumbTravel)) *
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
    if (m_dwriteFactory)
    {
        CreateTextFormatAndMetrics();
    }
    // Font pixel metrics and custom bitmap dimensions change with DPI, so cached composition cannot survive it.
    UpdateViewportLayout();
    m_uiDeviceGeneration += 1U;
    InvalidateCustomDrawRegion(nullptr);
    InvalidateComposition();
}

HRESULT Renderer::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr;

    if (!m_d2dFactory)
    {
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), nullptr,
                               reinterpret_cast<VOID **>(m_d2dFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            return hr;
        }
    }
    if (!m_dwriteFactory)
    {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                 reinterpret_cast<IUnknown **>(m_dwriteFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            return hr;
        }
    }
    return S_OK;
}

HRESULT Renderer::CreateDeviceResources() noexcept
{
    DXGI_SWAP_CHAIN_DESC1 sSwapChainDesc;
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr;

    if (m_iClientWidth <= 0 || m_iClientHeight <= 0)
    {
        return S_FALSE;
    }
    if (m_swapChain)
    {
        if (m_uiRenderTargetWidth != static_cast<UINT>(m_iClientWidth) || m_uiRenderTargetHeight != static_cast<UINT>(m_iClientHeight))
        {
            return ResizeDeviceResources();
        }
        return S_OK;
    }

    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0U, D3D11_SDK_VERSION,
                           m_d3dDevice.GetAddressOf(), &featureLevel, m_d3dContext.GetAddressOf());
    if (FAILED(hr))
    {
        // WARP keeps the control usable on systems without a suitable hardware D3D device.
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0U, D3D11_SDK_VERSION,
                               m_d3dDevice.GetAddressOf(), &featureLevel, m_d3dContext.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }
    }
    hr = m_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<VOID **>(dxgiFactory.GetAddressOf()));
    if (FAILED(hr))
    {
        return hr;
    }
    hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), m_d2dDevice.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_renderTarget.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    sSwapChainDesc = DXGI_SWAP_CHAIN_DESC1{};
    sSwapChainDesc.Width = static_cast<UINT>(m_iClientWidth);
    sSwapChainDesc.Height = static_cast<UINT>(m_iClientHeight);
    sSwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sSwapChainDesc.SampleDesc.Count = 1U;
    sSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sSwapChainDesc.BufferCount = 3U;
    sSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    sSwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    sSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    hr = dxgiFactory->CreateSwapChainForHwnd(m_d3dDevice.Get(), m_hWnd, &sSwapChainDesc, nullptr, nullptr, swapChain.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    hr = swapChain.As(&m_swapChain);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = m_swapChain->SetMaximumFrameLatency(1U);
    if (FAILED(hr))
    {
        return hr;
    }
    m_hFrameLatencyWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();
    if (!m_hFrameLatencyWaitableObject)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    return CreateTargetBitmap();
}

HRESULT Renderer::CreateTargetBitmap() noexcept
{
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
    D2D1_BITMAP_PROPERTIES1 sBitmapProperties;
    HRESULT hr;

    hr = m_swapChain->GetBuffer(0U, __uuidof(IDXGISurface), reinterpret_cast<VOID **>(dxgiSurface.GetAddressOf()));
    if (FAILED(hr))
    {
        return hr;
    }
    sBitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), m_fDpiX, m_fDpiY);
    hr = m_renderTarget->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &sBitmapProperties, m_targetBitmap.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    m_renderTarget->SetTarget(m_targetBitmap.Get());
    m_renderTarget->SetDpi(m_fDpiX, m_fDpiY);
    hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), m_brush.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    m_uiRenderTargetWidth = static_cast<UINT>(m_iClientWidth);
    m_uiRenderTargetHeight = static_cast<UINT>(m_iClientHeight);
    m_uiDeviceGeneration += 1U;
    return CreateCompositionBitmap();
}

HRESULT Renderer::CreateCompositionBitmap() noexcept
{
    D2D1_BITMAP_PROPERTIES1 sBitmapProperties;
    HRESULT hr;

    // Build terminal cells off-screen; only changed pixel rectangles are copied to the swap-chain target later.
    m_compositionBitmap.Reset();
    sBitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
                                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), m_fDpiX, m_fDpiY);
    hr = m_renderTarget->CreateBitmap(D2D1::SizeU(static_cast<UINT>(m_iClientWidth), static_cast<UINT>(m_iClientHeight)), nullptr, 0U,
                                      &sBitmapProperties, m_compositionBitmap.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    m_renderTarget->SetTarget(m_targetBitmap.Get());
    InvalidateComposition();
    return S_OK;
}

HRESULT Renderer::ResizeDeviceResources() noexcept
{
    HRESULT hr;

    // Direct2D must release its swap-chain target before DXGI can resize the underlying buffers.
    m_renderTarget->SetTarget(nullptr);
    m_targetBitmap.Reset();
    m_compositionBitmap.Reset();
    m_brush.Reset();
    hr = m_swapChain->ResizeBuffers(3U, static_cast<UINT>(m_iClientWidth), static_cast<UINT>(m_iClientHeight), DXGI_FORMAT_B8G8R8A8_UNORM,
                                    DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    if (FAILED(hr))
    {
        return hr;
    }
    return CreateTargetBitmap();
}

VOID Renderer::DiscardDeviceResources() noexcept
{
    m_pathSink.Reset();
    m_pathGeometry.Reset();
    m_brush.Reset();
    if (m_renderTarget)
    {
        m_renderTarget->SetTarget(nullptr);
    }
    m_targetBitmap.Reset();
    m_compositionBitmap.Reset();
    m_renderTarget.Reset();
    m_d2dDevice.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();
    if (m_hFrameLatencyWaitableObject)
    {
        CloseHandle(m_hFrameLatencyWaitableObject);
    }
    m_uiRenderTargetWidth = 0U;
    m_uiRenderTargetHeight = 0U;
    m_hFrameLatencyWaitableObject = nullptr;
    m_vecCachedCells.clear();
    m_mapCustomDrawCaches.clear();
    InvalidateComposition();
}

VOID Renderer::InvalidateComposition() noexcept
{
    m_bCompositionInvalid = TRUE;
    m_bScrollBarsDirty = TRUE;
    m_bFrameReady = FALSE;
    m_bFrameFull = TRUE;
    m_bWaitForFrameLatency = FALSE;
    m_uDirtyCount = 0U;
}

VOID Renderer::AddDirtyRect(_In_ const RECT &rcDirty) noexcept
{
    RECT &rcLast = m_rcDirty[2U];

    if (rcDirty.left >= rcDirty.right || rcDirty.top >= rcDirty.bottom)
    {
        return;
    }
    if (m_uDirtyCount < 3U)
    {
        m_rcDirty[m_uDirtyCount] = rcDirty;
        m_uDirtyCount += 1U;
        return;
    }
    // Present1 accepts only this small bounded list; merge further updates into the final rectangle.
    rcLast.left = (std::min)(rcLast.left, rcDirty.left);
    rcLast.top = (std::min)(rcLast.top, rcDirty.top);
    rcLast.right = (std::max)(rcLast.right, rcDirty.right);
    rcLast.bottom = (std::max)(rcLast.bottom, rcDirty.bottom);
}

HRESULT Renderer::CreateTextFormatAndMetrics() noexcept
{
    DWRITE_FONT_METRICS fontMetrics;
    Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection;
    UINT32 uiFamilyIndex;
    BOOL bExists;
    Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
    Microsoft::WRL::ComPtr<IDWriteFont> font;
    Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;
    UINT32 uiCodepoint;
    UINT16 uiGlyphIndex;
    DWRITE_GLYPH_METRICS glyphMetrics;
    FLOAT fDesignUnitsPerDip;
    FLOAT fAdvanceWidthDips;
    FLOAT fAdvanceHeightDips;
    FLOAT fAscentDips;
    FLOAT fDescentDips;
    FLOAT fLineGapDips;
    FLOAT fBaselineDips;
    FLOAT fUnderlineOffsetDips;
    FLOAT fUnderlineThicknessDips;
    FLOAT fFontSizeDips;
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
    hr = font->CreateFontFace(fontFace.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    fFontSizeDips = (m_metricsFont.fFontSize * 96.0f) / 72.0f;
    uiCodepoint = static_cast<UINT32>(L'0');

    for (INT iStyle = 0; iStyle < 4; ++iStyle)
    {
        m_textFormat[iStyle].Reset();
        hr = m_dwriteFactory->CreateTextFormat(m_metricsFont.strFontFamilyW.c_str(), nullptr,
                                               ((iStyle & 1) == 0) ? DWRITE_FONT_WEIGHT_NORMAL : DWRITE_FONT_WEIGHT_BOLD,
                                               ((iStyle & 2) == 0) ? DWRITE_FONT_STYLE_NORMAL : DWRITE_FONT_STYLE_ITALIC,
                                               DWRITE_FONT_STRETCH_NORMAL, fFontSizeDips, L"", m_textFormat[iStyle].GetAddressOf());
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

    fontFace->GetMetrics(&fontMetrics);
    fDesignUnitsPerDip = fFontSizeDips / static_cast<FLOAT>(fontMetrics.designUnitsPerEm);
    uiGlyphIndex = 0;
    hr = fontFace->GetGlyphIndicesW(&uiCodepoint, 1U, &uiGlyphIndex);
    if (FAILED(hr))
    {
        return hr;
    }
    glyphMetrics = DWRITE_GLYPH_METRICS{};
    if (uiGlyphIndex != 0)
    {
        hr = fontFace->GetDesignGlyphMetrics(&uiGlyphIndex, 1U, &glyphMetrics, FALSE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    fAdvanceWidthDips = static_cast<FLOAT>(glyphMetrics.advanceWidth) * fDesignUnitsPerDip;
    fAscentDips = static_cast<FLOAT>(fontMetrics.ascent) * fDesignUnitsPerDip;
    fDescentDips = static_cast<FLOAT>(fontMetrics.descent) * fDesignUnitsPerDip;
    fLineGapDips = static_cast<FLOAT>(fontMetrics.lineGap) * fDesignUnitsPerDip;
    fAdvanceHeightDips = fAscentDips + fDescentDips + fLineGapDips;
    fBaselineDips = fAscentDips + (fLineGapDips * 0.5f);
    fUnderlineOffsetDips = static_cast<FLOAT>(fontMetrics.underlinePosition) * fDesignUnitsPerDip;
    fUnderlineThicknessDips = static_cast<FLOAT>(fontMetrics.underlineThickness) * fDesignUnitsPerDip;

    m_metricsFont.iCellWidthPx = (std::max)(DipsToPixelsX(fAdvanceWidthDips), 1);
    m_metricsFont.iCellHeightPx = (std::max)(DipsToPixelsY(fAdvanceHeightDips), 1);
    m_metricsFont.iBaselinePx = (std::max)(DipsToPixelsY(fBaselineDips), 1);
    m_metricsFont.iUnderlineOffsetPx = DipsToPixelsY(fUnderlineOffsetDips);
    m_metricsFont.iUnderlineThicknessPx = (std::max)(DipsToPixelsY(fUnderlineThicknessDips), 1);
    UpdateViewportLayout();
    return S_OK;
}

FLOAT Renderer::PixelsToDipsX(_In_ INT iPixels) const noexcept
{
    return (static_cast<FLOAT>(iPixels) * 96.0f) / m_fDpiX;
}

FLOAT Renderer::PixelsToDipsY(_In_ INT iPixels) const noexcept
{
    return (static_cast<FLOAT>(iPixels) * 96.0f) / m_fDpiY;
}

INT Renderer::DipsToPixelsX(_In_ FLOAT fDips) const noexcept
{
    return static_cast<INT>(std::lround((fDips * m_fDpiX) / 96.0f));
}

INT Renderer::DipsToPixelsY(_In_ FLOAT fDips) const noexcept
{
    return static_cast<INT>(std::lround((fDips * m_fDpiY) / 96.0f));
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

    iContentWidth = m_metricsFont.iCellWidthPx * m_iCols;
    iContentHeight = m_metricsFont.iCellHeightPx * m_iRows;
    bVisibleHorizontal = FALSE;
    bVisibleVertical = FALSE;
    // One scrollbar reduces the opposite viewport dimension, which can require the other scrollbar too.
    do
    {
        bVisibleHorizontalOld = bVisibleHorizontal;
        bVisibleVerticalOld = bVisibleVertical;
        iViewportWidth = m_iClientWidth - ((bVisibleVertical != FALSE) ? m_iScrollBarThickness : 0);
        iViewportHeight = m_iClientHeight - ((bVisibleHorizontal != FALSE) ? m_iScrollBarThickness : 0);
        iViewportWidth = (std::max)(iViewportWidth, 0);
        iViewportHeight = (std::max)(iViewportHeight, 0);
        bVisibleHorizontal = (iContentWidth > iViewportWidth) ? TRUE : FALSE;
        bVisibleVertical = (iContentHeight > iViewportHeight) ? TRUE : FALSE;
    }
    while (bVisibleHorizontalOld != bVisibleHorizontal || bVisibleVerticalOld != bVisibleVertical);

    iViewportWidth = m_iClientWidth - ((bVisibleVertical != FALSE) ? m_iScrollBarThickness : 0);
    iViewportHeight = m_iClientHeight - ((bVisibleHorizontal != FALSE) ? m_iScrollBarThickness : 0);
    iViewportWidth = (std::max)(iViewportWidth, 0);
    iViewportHeight = (std::max)(iViewportHeight, 0);

    m_rcViewport.left = 0;
    m_rcViewport.top = 0;
    m_rcViewport.right = iViewportWidth;
    m_rcViewport.bottom = iViewportHeight;

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
    // Center undersized content, or translate oversized content by the current pixel scroll offset.
    m_iGridOffsetX = static_cast<INT>(m_rcViewport.left) +
                     (std::max)(0, (static_cast<INT>(m_rcViewport.right - m_rcViewport.left) - iContentWidth) / 2) - m_iScrollOffsetX;
    m_iGridOffsetY = static_cast<INT>(m_rcViewport.top) +
                     (std::max)(0, (static_cast<INT>(m_rcViewport.bottom - m_rcViewport.top) - iContentHeight) / 2) - m_iScrollOffsetY;
}

VOID Renderer::UpdateScrollBarMetrics(_Inout_ ScrollBarMetrics &scrollBarMetrics, _In_ BOOL bVertical) noexcept
{
    INT iTrackLength;
    INT iThumbLength;
    INT iThumbPosition;
    INT iInset;
    INT iThickness;

    scrollBarMetrics.iThumbTravel = 0;
    scrollBarMetrics.rcTrack = RECT{};
    scrollBarMetrics.rcThumb = RECT{};
    if (bVertical != FALSE)
    {
        scrollBarMetrics.iViewportSize = m_rcViewport.bottom - m_rcViewport.top;
        scrollBarMetrics.iContentSize = m_metricsFont.iCellHeightPx * m_iRows;
        scrollBarMetrics.iMaxOffset = (std::max)(scrollBarMetrics.iContentSize - scrollBarMetrics.iViewportSize, 0);
        m_iScrollOffsetY = ClampInt(m_iScrollOffsetY, 0, scrollBarMetrics.iMaxOffset);
        scrollBarMetrics.iOffset = m_iScrollOffsetY;
        if (scrollBarMetrics.bVisible == FALSE)
        {
            return;
        }
        scrollBarMetrics.rcTrack.left = m_rcViewport.right;
        scrollBarMetrics.rcTrack.top = 0;
        scrollBarMetrics.rcTrack.right = m_rcViewport.right + m_iScrollBarThickness;
        scrollBarMetrics.rcTrack.bottom = m_rcViewport.bottom;
    }
    else
    {
        scrollBarMetrics.iViewportSize = m_rcViewport.right - m_rcViewport.left;
        scrollBarMetrics.iContentSize = m_metricsFont.iCellWidthPx * m_iCols;
        scrollBarMetrics.iMaxOffset = (std::max)(scrollBarMetrics.iContentSize - scrollBarMetrics.iViewportSize, 0);
        m_iScrollOffsetX = ClampInt(m_iScrollOffsetX, 0, scrollBarMetrics.iMaxOffset);
        scrollBarMetrics.iOffset = m_iScrollOffsetX;
        if (scrollBarMetrics.bVisible == FALSE)
        {
            return;
        }
        scrollBarMetrics.rcTrack.left = 0;
        scrollBarMetrics.rcTrack.top = m_rcViewport.bottom;
        scrollBarMetrics.rcTrack.right = m_rcViewport.right;
        scrollBarMetrics.rcTrack.bottom = m_rcViewport.bottom + m_iScrollBarThickness;
    }

    iTrackLength = (bVertical != FALSE) ? (scrollBarMetrics.rcTrack.bottom - scrollBarMetrics.rcTrack.top)
                                        : (scrollBarMetrics.rcTrack.right - scrollBarMetrics.rcTrack.left);
    if (scrollBarMetrics.iContentSize <= 0 || scrollBarMetrics.iViewportSize <= 0 || iTrackLength <= 0)
    {
        return;
    }
    iThickness = m_iScrollBarThickness;
    iThumbLength =
        (std::max)(static_cast<INT>(std::lround((static_cast<FLOAT>(iTrackLength) * static_cast<FLOAT>(scrollBarMetrics.iViewportSize)) /
                                                static_cast<FLOAT>(scrollBarMetrics.iContentSize))),
                   (std::max)(iThickness * 2, 24));
    // Keep the thumb proportional to the visible fraction while preserving a usable minimum grab size.
    if (iThumbLength > iTrackLength)
    {
        iThumbLength = iTrackLength;
    }
    scrollBarMetrics.iThumbTravel = iTrackLength - iThumbLength;
    iThumbPosition = (scrollBarMetrics.iMaxOffset > 0) ? static_cast<INT>(std::lround((static_cast<FLOAT>(scrollBarMetrics.iThumbTravel) *
                                                                                       static_cast<FLOAT>(scrollBarMetrics.iOffset)) /
                                                                                      static_cast<FLOAT>(scrollBarMetrics.iMaxOffset)))
                                                       : 0;
    iInset = (std::max)(static_cast<INT>(std::lround(static_cast<FLOAT>(iThickness) * 0.2f)), 2);
    if (bVertical != FALSE)
    {
        scrollBarMetrics.rcThumb.left = scrollBarMetrics.rcTrack.left + iInset;
        scrollBarMetrics.rcThumb.top = scrollBarMetrics.rcTrack.top + iThumbPosition + iInset;
        scrollBarMetrics.rcThumb.right = scrollBarMetrics.rcTrack.right - iInset;
        scrollBarMetrics.rcThumb.bottom = scrollBarMetrics.rcTrack.top + iThumbPosition + iThumbLength - iInset;
    }
    else
    {
        scrollBarMetrics.rcThumb.left = scrollBarMetrics.rcTrack.left + iThumbPosition + iInset;
        scrollBarMetrics.rcThumb.top = scrollBarMetrics.rcTrack.top + iInset;
        scrollBarMetrics.rcThumb.right = scrollBarMetrics.rcTrack.left + iThumbPosition + iThumbLength - iInset;
        scrollBarMetrics.rcThumb.bottom = scrollBarMetrics.rcTrack.bottom - iInset;
    }
}

VOID Renderer::DrawScrollBars(_In_ COLORREF crDefaultBackground) noexcept
{
    D2D1_ROUNDED_RECT rcThumbRounded;
    D2D1_RECT_F rcCorner;
    D2D1_RECT_F rcTrack;
    D2D1_RECT_F rcThumb;
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
        rcTrack = D2D1::RectF(PixelsToDipsX(m_scrollBarVertical.rcTrack.left), PixelsToDipsY(m_scrollBarVertical.rcTrack.top),
                              PixelsToDipsX(m_scrollBarVertical.rcTrack.right), PixelsToDipsY(m_scrollBarVertical.rcTrack.bottom));
        rcThumb = D2D1::RectF(PixelsToDipsX(m_scrollBarVertical.rcThumb.left), PixelsToDipsY(m_scrollBarVertical.rcThumb.top),
                              PixelsToDipsX(m_scrollBarVertical.rcThumb.right), PixelsToDipsY(m_scrollBarVertical.rcThumb.bottom));
        m_brush->SetColor(colorTrack);
        m_renderTarget->FillRectangle(rcTrack, m_brush.Get());
        fThumbRadius = (std::min)((rcThumb.right - rcThumb.left) * 0.5f, (rcThumb.bottom - rcThumb.top) * 0.5f);
        rcThumbRounded = D2D1::RoundedRect(rcThumb, fThumbRadius, fThumbRadius);
        m_brush->SetColor((m_scrollBarVertical.bHot != FALSE) ? colorThumbHot : colorThumb);
        m_renderTarget->FillRoundedRectangle(rcThumbRounded, m_brush.Get());
    }

    if (m_scrollBarHorizontal.bVisible != FALSE)
    {
        rcTrack = D2D1::RectF(PixelsToDipsX(m_scrollBarHorizontal.rcTrack.left), PixelsToDipsY(m_scrollBarHorizontal.rcTrack.top),
                              PixelsToDipsX(m_scrollBarHorizontal.rcTrack.right), PixelsToDipsY(m_scrollBarHorizontal.rcTrack.bottom));
        rcThumb = D2D1::RectF(PixelsToDipsX(m_scrollBarHorizontal.rcThumb.left), PixelsToDipsY(m_scrollBarHorizontal.rcThumb.top),
                              PixelsToDipsX(m_scrollBarHorizontal.rcThumb.right), PixelsToDipsY(m_scrollBarHorizontal.rcThumb.bottom));
        m_brush->SetColor(colorTrack);
        m_renderTarget->FillRectangle(rcTrack, m_brush.Get());
        fThumbRadius = (std::min)((rcThumb.right - rcThumb.left) * 0.5f, (rcThumb.bottom - rcThumb.top) * 0.5f);
        rcThumbRounded = D2D1::RoundedRect(rcThumb, fThumbRadius, fThumbRadius);
        m_brush->SetColor((m_scrollBarHorizontal.bHot != FALSE) ? colorThumbHot : colorThumb);
        m_renderTarget->FillRoundedRectangle(rcThumbRounded, m_brush.Get());
    }

    if (m_scrollBarHorizontal.bVisible != FALSE && m_scrollBarVertical.bVisible != FALSE)
    {
        rcCorner = D2D1::RectF(PixelsToDipsX(m_rcViewport.right), PixelsToDipsY(m_rcViewport.bottom), PixelsToDipsX(m_iClientWidth),
                               PixelsToDipsY(m_iClientHeight));
        m_brush->SetColor(colorTrack);
        m_renderTarget->FillRectangle(rcCorner, m_brush.Get());
    }
}

VOID Renderer::ClearScrollBarAreas(_In_ COLORREF crDefaultBackground) noexcept
{
    D2D1_RECT_F rcArea;

    m_brush->SetColor(ToD2DColor(crDefaultBackground));
    if (m_scrollBarVertical.bVisible != FALSE)
    {
        rcArea = D2D1::RectF(PixelsToDipsX(m_scrollBarVertical.rcTrack.left), PixelsToDipsY(m_scrollBarVertical.rcTrack.top),
                             PixelsToDipsX(m_scrollBarVertical.rcTrack.right), PixelsToDipsY(m_scrollBarVertical.rcTrack.bottom));
        m_renderTarget->FillRectangle(rcArea, m_brush.Get());
    }
    if (m_scrollBarHorizontal.bVisible != FALSE)
    {
        rcArea = D2D1::RectF(PixelsToDipsX(m_scrollBarHorizontal.rcTrack.left), PixelsToDipsY(m_scrollBarHorizontal.rcTrack.top),
                             PixelsToDipsX(m_scrollBarHorizontal.rcTrack.right), PixelsToDipsY(m_scrollBarHorizontal.rcTrack.bottom));
        m_renderTarget->FillRectangle(rcArea, m_brush.Get());
    }
    if (m_scrollBarHorizontal.bVisible != FALSE && m_scrollBarVertical.bVisible != FALSE)
    {
        rcArea = D2D1::RectF(PixelsToDipsX(m_rcViewport.right), PixelsToDipsY(m_rcViewport.bottom), PixelsToDipsX(m_iClientWidth),
                             PixelsToDipsY(m_iClientHeight));
        m_renderTarget->FillRectangle(rcArea, m_brush.Get());
    }
}

VOID Renderer::DrawCells(_In_ const Buffer::Snapshot &sSnapshotBuffer, _In_ const CellRect_t &sRectDirty) noexcept
{
    INT iStartX;
    INT iStartY;
    INT iEndX;
    INT iEndY;
    INT iRow;
    INT iCol;
    size_t iIndex;

    iStartX = (std::max)(sRectDirty.iX, 0);
    iStartY = (std::max)(sRectDirty.iY, 0);
    iEndX = (std::min)(sRectDirty.iX + sRectDirty.iWidth, sSnapshotBuffer.iCols);
    iEndY = (std::min)(sRectDirty.iY + sRectDirty.iHeight, sSnapshotBuffer.iRows);
    for (iRow = iStartY; iRow < iEndY; ++iRow)
    {
        for (iCol = iStartX; iCol < iEndX; ++iCol)
        {
            iIndex = static_cast<size_t>(iRow * sSnapshotBuffer.iCols + iCol);
            DrawCell(sSnapshotBuffer.lpCells[iIndex], iCol, iRow, sSnapshotBuffer);
        }
    }
}

VOID Renderer::DrawCell(_In_ const Buffer::Cell &sCellCurrent, _In_ INT iCol, _In_ INT iRow,
                        _In_ const Buffer::Snapshot &sSnapshotBuffer) noexcept
{
    RECT rcCell;
    D2D1_RECT_F rcBackground;
    D2D1_RECT_F rcText;
    D2D1_RECT_F rcUnderline;
    COLORREF crForeground;
    COLORREF crBackground;
    FLOAT fUnderlineTop;
    FLOAT fUnderlineBottom;
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
    rcCell.left = m_iGridOffsetX + (m_metricsFont.iCellWidthPx * iCol);
    rcCell.top = m_iGridOffsetY + (m_metricsFont.iCellHeightPx * iRow);
    rcCell.right = rcCell.left + m_metricsFont.iCellWidthPx;
    rcCell.bottom = rcCell.top + m_metricsFont.iCellHeightPx;
    rcBackground =
        D2D1::RectF(PixelsToDipsX(rcCell.left), PixelsToDipsY(rcCell.top), PixelsToDipsX(rcCell.right), PixelsToDipsY(rcCell.bottom));
    m_brush->SetColor(ToD2DColor(crBackground));
    m_renderTarget->FillRectangle(rcBackground, m_brush.Get());

    bTextVisible = ((sCellCurrent.dwStyleFlags & Control::StyleBlink) == 0U || sSnapshotBuffer.bBlinkVisible != FALSE) ? TRUE : FALSE;
    if (bTextVisible != FALSE && sCellCurrent.chCodepointW != L' ')
    {
        rcText = rcBackground;
        DrawGlyph(sCellCurrent.chCodepointW, sCellCurrent.dwStyleFlags, crForeground, rcText);
    }

    if ((sCellCurrent.dwStyleFlags & Control::StyleUnderline) != 0U)
    {
        fUnderlineTop = PixelsToDipsY(rcCell.top + m_metricsFont.iBaselinePx - m_metricsFont.iUnderlineOffsetPx);
        fUnderlineBottom =
            PixelsToDipsY(rcCell.top + m_metricsFont.iBaselinePx - m_metricsFont.iUnderlineOffsetPx + m_metricsFont.iUnderlineThicknessPx);
        rcUnderline = D2D1::RectF(PixelsToDipsX(rcCell.left), fUnderlineTop, PixelsToDipsX(rcCell.right), fUnderlineBottom);
        m_brush->SetColor(ToD2DColor(crForeground));
        m_renderTarget->FillRectangle(rcUnderline, m_brush.Get());
    }
}

VOID Renderer::DrawCursor(_In_ const Buffer::Snapshot &sSnapshotBuffer) noexcept
{
    const Buffer::Cell *lpsCellCurrent;
    RECT rcCell;
    D2D1_RECT_F rcCursor;
    D2D1_RECT_F rcText;
    COLORREF crForeground;
    COLORREF crBackground;
    INT iCursorBarHeight;
    INT iCursorBarWidth;

    if (sSnapshotBuffer.bCursorVisible == FALSE || sSnapshotBuffer.bBlinkVisible == FALSE)
    {
        return;
    }
    if (sSnapshotBuffer.iCursorCol < 0 || sSnapshotBuffer.iCursorCol >= sSnapshotBuffer.iCols || sSnapshotBuffer.iCursorRow < 0 ||
        sSnapshotBuffer.iCursorRow >= sSnapshotBuffer.iRows)
    {
        return;
    }

    lpsCellCurrent =
        &sSnapshotBuffer.lpCells[static_cast<size_t>(sSnapshotBuffer.iCursorRow * sSnapshotBuffer.iCols + sSnapshotBuffer.iCursorCol)];
    if ((lpsCellCurrent->dwStyleFlags & Control::StyleInverse) == 0U)
    {
        crForeground = lpsCellCurrent->crForeground;
        crBackground = lpsCellCurrent->crBackground;
    }
    else
    {
        crForeground = lpsCellCurrent->crBackground;
        crBackground = lpsCellCurrent->crForeground;
    }

    rcCell.left = m_iGridOffsetX + (m_metricsFont.iCellWidthPx * sSnapshotBuffer.iCursorCol);
    rcCell.top = m_iGridOffsetY + (m_metricsFont.iCellHeightPx * sSnapshotBuffer.iCursorRow);
    rcCell.right = rcCell.left + m_metricsFont.iCellWidthPx;
    rcCell.bottom = rcCell.top + m_metricsFont.iCellHeightPx;
    rcCursor =
        D2D1::RectF(PixelsToDipsX(rcCell.left), PixelsToDipsY(rcCell.top), PixelsToDipsX(rcCell.right), PixelsToDipsY(rcCell.bottom));

    switch (sSnapshotBuffer.dwCursorStyle)
    {
        case Control::CursorBarLeft:
            iCursorBarWidth = (std::max)(m_metricsFont.iUnderlineThicknessPx, 2);
            rcCursor.right = PixelsToDipsX(rcCell.left + iCursorBarWidth);
            m_brush->SetColor(ToD2DColor(crForeground));
            m_renderTarget->FillRectangle(rcCursor, m_brush.Get());
            break;

        case Control::CursorUnderscore:
            iCursorBarHeight = (std::max)(m_metricsFont.iUnderlineThicknessPx, 2);
            iCursorBarHeight = (iCursorBarHeight * 3 + 1) / 2;
            rcCursor.top = PixelsToDipsY(rcCell.bottom - iCursorBarHeight);
            m_brush->SetColor(ToD2DColor(crForeground));
            m_renderTarget->FillRectangle(rcCursor, m_brush.Get());
            break;

        case Control::CursorBlock:
        default:
            m_brush->SetColor(ToD2DColor(crForeground));
            m_renderTarget->FillRectangle(rcCursor, m_brush.Get());
            if (lpsCellCurrent->chCodepointW != L' ' &&
                ((lpsCellCurrent->dwStyleFlags & Control::StyleBlink) == 0U || sSnapshotBuffer.bBlinkVisible != FALSE))
            {
                rcText = rcCursor;
                DrawGlyph(lpsCellCurrent->chCodepointW, lpsCellCurrent->dwStyleFlags, crBackground, rcText);
            }
            break;
    }
}

VOID Renderer::DrawGlyph(_In_ WCHAR chCodepointW, _In_ DWORD dwStyleFlags, _In_ COLORREF crForeground,
                         _In_ const D2D1_RECT_F &rcText) noexcept
{
    INT iStyle;

    iStyle = 0;
    if ((dwStyleFlags & Control::StyleBold) != 0U)
    {
        iStyle |= 1;
    }
    if ((dwStyleFlags & Control::StyleItalic) != 0U)
    {
        iStyle |= 2;
    }

    m_brush->SetColor(ToD2DColor(crForeground));
    m_renderTarget->DrawTextW(&chCodepointW, 1, m_textFormat[iStyle].Get(), rcText, m_brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP,
                              DWRITE_MEASURING_MODE_NATURAL);
}

} // namespace GuiTerminal::Internals

// -----------------------------------------------------------------------------

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor) noexcept
{
    return D2D1::ColorF(static_cast<FLOAT>(GetRValue(crColor)) / 255.0f, static_cast<FLOAT>(GetGValue(crColor)) / 255.0f,
                        static_cast<FLOAT>(GetBValue(crColor)) / 255.0f, 1.0f);
}

static D2D1_COLOR_F ToD2DColor(_In_ COLORREF crColor, _In_ FLOAT fAlpha) noexcept
{
    return D2D1::ColorF(static_cast<FLOAT>(GetRValue(crColor)) / 255.0f, static_cast<FLOAT>(GetGValue(crColor)) / 255.0f,
                        static_cast<FLOAT>(GetBValue(crColor)) / 255.0f, fAlpha);
}

static FLOAT GetColorLuminance(_In_ COLORREF crColor) noexcept
{
    return ((0.2126f * static_cast<FLOAT>(GetRValue(crColor))) + (0.7152f * static_cast<FLOAT>(GetGValue(crColor))) +
            (0.0722f * static_cast<FLOAT>(GetBValue(crColor)))) /
           255.0f;
}

static BOOL IsPointInRect(_In_ INT iX, _In_ INT iY, _In_ const RECT &rcCurrent) noexcept
{
    return (iX >= rcCurrent.left && iX < rcCurrent.right && iY >= rcCurrent.top && iY < rcCurrent.bottom) ? TRUE : FALSE;
}

static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept
{
    return (std::max)(iMinimum, (std::min)(iValue, iMaximumValue));
}

static BOOL AreCellsEqual(_In_ const GuiTerminal::Internals::Buffer::Cell &sCellFirst,
                          _In_ const GuiTerminal::Internals::Buffer::Cell &sCellSecond) noexcept
{
    return (sCellFirst.chCodepointW == sCellSecond.chCodepointW && sCellFirst.crForeground == sCellSecond.crForeground &&
            sCellFirst.crBackground == sCellSecond.crBackground && sCellFirst.dwStyleFlags == sCellSecond.dwStyleFlags)
               ? TRUE
               : FALSE;
}

static BOOL IntersectCellRects(_In_ const GuiTerminal::Internals::CellRect_t &sRectFirst,
                               _In_ const GuiTerminal::Internals::CellRect_t &sRectSecond) noexcept
{
    return (sRectFirst.iX < sRectSecond.iX + sRectSecond.iWidth && sRectSecond.iX < sRectFirst.iX + sRectFirst.iWidth &&
            sRectFirst.iY < sRectSecond.iY + sRectSecond.iHeight && sRectSecond.iY < sRectFirst.iY + sRectFirst.iHeight)
               ? TRUE
               : FALSE;
}

static VOID IncludeCellRect(_Inout_ GuiTerminal::Internals::CellRect_t &sRectTarget,
                            _In_ const GuiTerminal::Internals::CellRect_t &sRectSource) noexcept
{
    INT iLeft;
    INT iTop;
    INT iRight;
    INT iBottom;

    if (sRectSource.iWidth <= 0 || sRectSource.iHeight <= 0)
    {
        return;
    }
    if (sRectTarget.iWidth <= 0 || sRectTarget.iHeight <= 0)
    {
        sRectTarget = sRectSource;
        return;
    }
    iLeft = (std::min)(sRectTarget.iX, sRectSource.iX);
    iTop = (std::min)(sRectTarget.iY, sRectSource.iY);
    iRight = (std::max)(sRectTarget.iX + sRectTarget.iWidth, sRectSource.iX + sRectSource.iWidth);
    iBottom = (std::max)(sRectTarget.iY + sRectTarget.iHeight, sRectSource.iY + sRectSource.iHeight);
    sRectTarget.iX = iLeft;
    sRectTarget.iY = iTop;
    sRectTarget.iWidth = iRight - iLeft;
    sRectTarget.iHeight = iBottom - iTop;
}
