#include "..\include\GuiTerminalBuffer.h"
#include "..\include\GuiTerminalControl.h"
#include <algorithm>

// -----------------------------------------------------------------------------

typedef struct BoxEdges_s {
    BYTE byUp{};
    BYTE byRight{};
    BYTE byDown{};
    BYTE byLeft{};
} BoxEdges;

// -----------------------------------------------------------------------------

static constexpr BYTE EDGE_NONE = 0U;
static constexpr BYTE EDGE_SINGLE = 1U;
static constexpr BYTE EDGE_DOUBLE = 2U;

static COLORREF MakeColor(_In_ BYTE byRed, _In_ BYTE byGreen, _In_ BYTE byBlue) noexcept;
static GuiTerminal::Internals::Attributes_t MakeAttributes(_In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                                           _In_ DWORD dwStyleFlags) noexcept;
static BOOL IsWithinBounds(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumExclusive) noexcept;
static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept;
static COLORREF GetAnsi16Color(_In_ INT iIndex) noexcept;
static COLORREF GetXterm256Color(_In_ INT iIndex) noexcept;
static BOOL IsStrokeMergeable(_In_ DWORD dwStrokeType) noexcept;
static WCHAR GetStrokeGlyph(_In_ DWORD dwStrokeType) noexcept;
static BoxEdges MakeBoxEdges(_In_ BYTE byUp, _In_ BYTE byRight, _In_ BYTE byDown, _In_ BYTE byLeft) noexcept;
static BOOL TryDecodeBoxGlyph(_In_ WCHAR chGlyphW, _Out_ BoxEdges *lpsEdges) noexcept;
static BOOL TryEncodeBoxGlyph(_In_ const BoxEdges &sEdges, _Out_ WCHAR *lpchGlyphW) noexcept;
static BOOL TryMergeBoxEdges(_In_ const BoxEdges &sExisting, _In_ const BoxEdges &sIncoming, _Out_ BoxEdges *lpsMerged) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal::Internals {

HRESULT Buffer::Initialize(_In_ INT iCols, _In_ INT iRows, _In_ COLORREF crDefaultForeground, _In_ COLORREF crDefaultBackground) noexcept
{
    m_sAttributesDefault = MakeAttributes(
        MakeColor(GetRValue(crDefaultForeground), GetGValue(crDefaultForeground), GetBValue(crDefaultForeground)),
        MakeColor(GetRValue(crDefaultBackground), GetGValue(crDefaultBackground), GetBValue(crDefaultBackground)), Control::StyleNone);
    m_iCols = iCols;
    m_iRows = iRows;
    m_iNextRegionId = 1;
    m_bBlinkVisible = TRUE;
    return InitializeRootRegion();
}

HRESULT Buffer::Resize(_In_ INT iCols, _In_ INT iRows) noexcept
{
    Region_t *lpsRegionRoot;
    std::vector<Cell> vecCellsRoot;
    std::vector<Cell> vecSnapshotCells;
    HRESULT hr;

    if (iCols <= 0 || iRows <= 0)
    {
        return E_INVALIDARG;
    }

    lpsRegionRoot = ResolveRegion(nullptr);
    if (!lpsRegionRoot)
    {
        return E_UNEXPECTED;
    }

    hr = ResizeRegionCells(*lpsRegionRoot, vecCellsRoot, iCols, iRows);
    if (FAILED(hr))
    {
        return hr;
    }

    try
    {
        vecSnapshotCells.assign(static_cast<size_t>(iCols) * static_cast<size_t>(iRows), MakeBlankCell());
    }
    catch (const std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }

    lpsRegionRoot->iWidth = iCols;
    lpsRegionRoot->iHeight = iRows;
    lpsRegionRoot->iCursorX = ClampInt(lpsRegionRoot->iCursorX, 0, iCols - 1);
    lpsRegionRoot->iCursorY = ClampInt(lpsRegionRoot->iCursorY, 0, iRows - 1);
    lpsRegionRoot->sCursorSaved.iX = ClampInt(lpsRegionRoot->sCursorSaved.iX, 0, iCols - 1);
    lpsRegionRoot->sCursorSaved.iY = ClampInt(lpsRegionRoot->sCursorSaved.iY, 0, iRows - 1);
    lpsRegionRoot->vecCells = std::move(vecCellsRoot);
    m_iCols = iCols;
    m_iRows = iRows;
    m_vecSnapshotCells = std::move(vecSnapshotCells);
    return S_OK;
}

VOID Buffer::Clear(_In_opt_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    ClearRegionCells(*lpsRegionCurrent);
    lpsRegionCurrent->iCursorX = 0;
    lpsRegionCurrent->iCursorY = 0;
    lpsRegionCurrent->sCursorSaved = CursorState_t{0, 0};
    lpsRegionCurrent->sAttributesCurrent = m_sAttributesDefault;
    lpsRegionCurrent->bWrapPending = FALSE;
}

VOID Buffer::Scroll(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
{
    if (iLineCount > 0)
    {
        ScrollRegionUp(hRegion, iLineCount);
    }
    else if (iLineCount < 0)
    {
        ScrollRegionDown(hRegion, -iLineCount);
    }
}

VOID Buffer::Move(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX,
                  _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                  _In_ DWORD dwStyleFlags) noexcept
{
    Region_t *lpsRegionCurrent;
    Attributes_t sFillAttributes;
    CellRect_t sRectSource;
    CellRect_t sRectSourceClipped;
    INT iDestinationStartX;
    INT iDestinationStartY;
    INT iDestinationEndX;
    INT iDestinationEndY;
    INT iDeltaX;
    INT iDeltaY;
    INT iStepX;
    INT iStepY;
    INT iStartX;
    INT iStartY;
    INT iEndXExclusive;
    INT iEndYExclusive;
    INT iSourceCurrentX;
    INT iSourceCurrentY;
    INT iDestinationX;
    INT iDestinationY;
    SIZE_T uSourceIndex;
    SIZE_T uDestinationIndex;

    if (iWidth <= 0 || iHeight <= 0)
    {
        return;
    }

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }
    sRectSource.iX = iSourceX;
    sRectSource.iY = iSourceY;
    sRectSource.iWidth = iWidth;
    sRectSource.iHeight = iHeight;
    if (ClipRectangle(*lpsRegionCurrent, sRectSource, sRectSourceClipped) == FALSE)
    {
        return;
    }

    iDeltaX = iTargetX - iSourceX;
    iDeltaY = iTargetY - iSourceY;
    iDestinationStartX = sRectSourceClipped.iX + iDeltaX;
    iDestinationStartY = sRectSourceClipped.iY + iDeltaY;
    iDestinationEndX = iDestinationStartX + sRectSourceClipped.iWidth - 1;
    iDestinationEndY = iDestinationStartY + sRectSourceClipped.iHeight - 1;

    iStepX = (iDeltaX > 0) ? -1 : 1;
    iStepY = (iDeltaY > 0) ? -1 : 1;
    iStartX = (iStepX > 0) ? sRectSourceClipped.iX : (sRectSourceClipped.iX + sRectSourceClipped.iWidth - 1);
    iStartY = (iStepY > 0) ? sRectSourceClipped.iY : (sRectSourceClipped.iY + sRectSourceClipped.iHeight - 1);
    iEndXExclusive = (iStepX > 0) ? (sRectSourceClipped.iX + sRectSourceClipped.iWidth) : (sRectSourceClipped.iX - 1);
    iEndYExclusive = (iStepY > 0) ? (sRectSourceClipped.iY + sRectSourceClipped.iHeight) : (sRectSourceClipped.iY - 1);

    for (iSourceCurrentY = iStartY; iSourceCurrentY != iEndYExclusive; iSourceCurrentY += iStepY)
    {
        for (iSourceCurrentX = iStartX; iSourceCurrentX != iEndXExclusive; iSourceCurrentX += iStepX)
        {
            iDestinationX = iSourceCurrentX + iDeltaX;
            iDestinationY = iSourceCurrentY + iDeltaY;
            if (iDestinationX < 0 || iDestinationX >= lpsRegionCurrent->iWidth || iDestinationY < 0 ||
                iDestinationY >= lpsRegionCurrent->iHeight)
            {
                continue;
            }
            if (GetCellIndex(iSourceCurrentX, iSourceCurrentY, lpsRegionCurrent->iWidth, &uSourceIndex) != FALSE &&
                GetCellIndex(iDestinationX, iDestinationY, lpsRegionCurrent->iWidth, &uDestinationIndex) != FALSE)
            {
                lpsRegionCurrent->vecCells[uDestinationIndex] = lpsRegionCurrent->vecCells[uSourceIndex];
                lpsRegionCurrent->vecCells[uDestinationIndex].bIsDirty = TRUE;
            }
        }
    }

    sFillAttributes = MakeAttributes(MakeColor(GetRValue(crForeground), GetGValue(crForeground), GetBValue(crForeground)),
                                     MakeColor(GetRValue(crBackground), GetGValue(crBackground), GetBValue(crBackground)), dwStyleFlags);
    for (iSourceCurrentY = sRectSourceClipped.iY; iSourceCurrentY < sRectSourceClipped.iY + sRectSourceClipped.iHeight; ++iSourceCurrentY)
    {
        for (iSourceCurrentX = sRectSourceClipped.iX; iSourceCurrentX < sRectSourceClipped.iX + sRectSourceClipped.iWidth;
             ++iSourceCurrentX)
        {
            if (IsCoordinateInsideRectangle(iSourceCurrentX, iSourceCurrentY, iDestinationStartX, iDestinationStartY, iDestinationEndX,
                                            iDestinationEndY) != FALSE)
            {
                continue;
            }
            SetCell(*lpsRegionCurrent, iSourceCurrentX, iSourceCurrentY, chFillW, sFillAttributes);
        }
    }
}

VOID Buffer::Fill(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW,
                  _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    Region_t *lpsRegionCurrent;
    CellRect_t sRectArea;
    CellRect_t sRectClipped;
    INT iFillX;
    INT iFillY;
    Attributes_t attributesCell;

    if (iWidth <= 0 || iHeight <= 0)
    {
        return;
    }

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    sRectArea.iX = iX;
    sRectArea.iY = iY;
    sRectArea.iWidth = iWidth;
    sRectArea.iHeight = iHeight;
    if (ClipRectangle(*lpsRegionCurrent, sRectArea, sRectClipped) == FALSE)
    {
        return;
    }

    attributesCell.crForeground = MakeColor(GetRValue(crForeground), GetGValue(crForeground), GetBValue(crForeground));
    attributesCell.crBackground = MakeColor(GetRValue(crBackground), GetGValue(crBackground), GetBValue(crBackground));
    attributesCell.dwStyleFlags = dwStyleFlags;

    for (iFillY = sRectClipped.iY; iFillY < sRectClipped.iY + sRectClipped.iHeight; ++iFillY)
    {
        for (iFillX = sRectClipped.iX; iFillX < sRectClipped.iX + sRectClipped.iWidth; ++iFillX)
        {
            SetCell(*lpsRegionCurrent, iFillX, iFillY, chCodepointW, attributesCell);
        }
    }
}

VOID Buffer::DrawHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ DWORD dwStrokeType,
                                _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    Region_t *lpsRegionCurrent;
    Attributes_t sAttributesCell;
    BYTE byWeight;
    INT iStartX;
    INT iEndX;
    INT iDrawX;

    if (iWidth <= 0)
    {
        return;
    }

    lpsRegionCurrent = ResolveRegion(hRegion);
    if ((!lpsRegionCurrent) || iY < 0 || iY >= lpsRegionCurrent->iHeight)
    {
        return;
    }

    sAttributesCell = MakeAttributes(MakeColor(GetRValue(crForeground), GetGValue(crForeground), GetBValue(crForeground)),
                                     MakeColor(GetRValue(crBackground), GetGValue(crBackground), GetBValue(crBackground)), dwStyleFlags);
    iStartX = (std::max)(iX, 0);
    iEndX = (std::min)(iX + iWidth - 1, lpsRegionCurrent->iWidth - 1);
    if (iStartX > iEndX)
    {
        return;
    }

    if (dwStrokeType == Control::StrokeSingleLine || dwStrokeType == Control::StrokeDoubleLine)
    {
        byWeight = (dwStrokeType == Control::StrokeDoubleLine) ? EDGE_DOUBLE : EDGE_SINGLE;
        for (iDrawX = iStartX; iDrawX <= iEndX; ++iDrawX)
        {
            DrawStrokeCell(*lpsRegionCurrent, iDrawX, iY, dwStrokeType, EDGE_NONE, byWeight, EDGE_NONE, byWeight, sAttributesCell);
        }
        return;
    }

    for (iDrawX = iStartX; iDrawX <= iEndX; ++iDrawX)
    {
        DrawStrokeCell(*lpsRegionCurrent, iDrawX, iY, dwStrokeType, EDGE_NONE, EDGE_NONE, EDGE_NONE, EDGE_NONE, sAttributesCell);
    }
}

VOID Buffer::DrawVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ DWORD dwStrokeType,
                              _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    Region_t *lpsRegionCurrent;
    Attributes_t sAttributesCell;
    BYTE byWeight;
    INT iStartY;
    INT iEndY;
    INT iDrawY;

    if (iHeight <= 0)
    {
        return;
    }

    lpsRegionCurrent = ResolveRegion(hRegion);
    if ((!lpsRegionCurrent) || iX < 0 || iX >= lpsRegionCurrent->iWidth)
    {
        return;
    }

    sAttributesCell = MakeAttributes(MakeColor(GetRValue(crForeground), GetGValue(crForeground), GetBValue(crForeground)),
                                     MakeColor(GetRValue(crBackground), GetGValue(crBackground), GetBValue(crBackground)), dwStyleFlags);
    iStartY = (std::max)(iY, 0);
    iEndY = (std::min)(iY + iHeight - 1, lpsRegionCurrent->iHeight - 1);
    if (iStartY > iEndY)
    {
        return;
    }

    if (dwStrokeType == Control::StrokeSingleLine || dwStrokeType == Control::StrokeDoubleLine)
    {
        byWeight = (dwStrokeType == Control::StrokeDoubleLine) ? EDGE_DOUBLE : EDGE_SINGLE;
        for (iDrawY = iStartY; iDrawY <= iEndY; ++iDrawY)
        {
            DrawStrokeCell(*lpsRegionCurrent, iX, iDrawY, dwStrokeType, byWeight, EDGE_NONE, byWeight, EDGE_NONE, sAttributesCell);
        }
        return;
    }

    for (iDrawY = iStartY; iDrawY <= iEndY; ++iDrawY)
    {
        DrawStrokeCell(*lpsRegionCurrent, iX, iDrawY, dwStrokeType, EDGE_NONE, EDGE_NONE, EDGE_NONE, EDGE_NONE, sAttributesCell);
    }
}

VOID Buffer::DrawBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags,
                     _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept
{
    Region_t *lpsRegionCurrent;
    Attributes_t sAttributesCell;
    BYTE byTop;
    BYTE byRight;
    BYTE byBottom;
    BYTE byLeft;

    if (iWidth < 0 || iHeight < 0)
    {
        return;
    }
    if (iWidth == 0 && iHeight == 0)
    {
        return;
    }

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    sAttributesCell = MakeAttributes(MakeColor(GetRValue(crForeground), GetGValue(crForeground), GetBValue(crForeground)),
                                     MakeColor(GetRValue(crBackground), GetGValue(crBackground), GetBValue(crBackground)), dwStyleFlags);
    byTop = ((dwBoxSideFlags & Control::BoxSideTopDouble) != 0U) ? EDGE_DOUBLE : EDGE_SINGLE;
    byRight = ((dwBoxSideFlags & Control::BoxSideRightDouble) != 0U) ? EDGE_DOUBLE : EDGE_SINGLE;
    byBottom = ((dwBoxSideFlags & Control::BoxSideBottomDouble) != 0U) ? EDGE_DOUBLE : EDGE_SINGLE;
    byLeft = ((dwBoxSideFlags & Control::BoxSideLeftDouble) != 0U) ? EDGE_DOUBLE : EDGE_SINGLE;

    if (iWidth == 0)
    {
        DrawVerticalLine(hRegion, iX, iY, iHeight,
                         ((byLeft == EDGE_SINGLE) && (byRight == EDGE_SINGLE)) ? Control::StrokeSingleLine : Control::StrokeDoubleLine,
                         crForeground, crBackground, dwStyleFlags);
        return;
    }
    if (iHeight == 0)
    {
        DrawHorizontalLine(hRegion, iX, iY, iWidth,
                           ((byTop == EDGE_SINGLE) && (byBottom == EDGE_SINGLE)) ? Control::StrokeSingleLine : Control::StrokeDoubleLine,
                           crForeground, crBackground, dwStyleFlags);
        return;
    }
    if (iWidth == 1)
    {
        DrawVerticalLine(hRegion, iX, iY, iHeight,
                         ((byLeft == EDGE_SINGLE) && (byRight == EDGE_SINGLE)) ? Control::StrokeSingleLine : Control::StrokeDoubleLine,
                         crForeground, crBackground, dwStyleFlags);
        return;
    }
    if (iHeight == 1)
    {
        DrawHorizontalLine(hRegion, iX, iY, iWidth,
                           ((byTop == EDGE_SINGLE) && (byBottom == EDGE_SINGLE)) ? Control::StrokeSingleLine : Control::StrokeDoubleLine,
                           crForeground, crBackground, dwStyleFlags);
        return;
    }

    if (iWidth > 2)
    {
        DrawHorizontalLine(hRegion, iX + 1, iY, iWidth - 2, (byTop == EDGE_DOUBLE) ? Control::StrokeDoubleLine : Control::StrokeSingleLine,
                           crForeground, crBackground, dwStyleFlags);
        DrawHorizontalLine(hRegion, iX + 1, iY + iHeight - 1, iWidth - 2,
                           (byBottom == EDGE_DOUBLE) ? Control::StrokeDoubleLine : Control::StrokeSingleLine, crForeground, crBackground,
                           dwStyleFlags);
    }
    if (iHeight > 2)
    {
        DrawVerticalLine(hRegion, iX, iY + 1, iHeight - 2, (byLeft == EDGE_DOUBLE) ? Control::StrokeDoubleLine : Control::StrokeSingleLine,
                         crForeground, crBackground, dwStyleFlags);
        DrawVerticalLine(hRegion, iX + iWidth - 1, iY + 1, iHeight - 2,
                         (byRight == EDGE_DOUBLE) ? Control::StrokeDoubleLine : Control::StrokeSingleLine, crForeground, crBackground,
                         dwStyleFlags);
    }

    DrawStrokeCell(*lpsRegionCurrent, iX, iY, Control::StrokeSingleLine, EDGE_NONE, byTop, byLeft, EDGE_NONE, sAttributesCell);
    DrawStrokeCell(*lpsRegionCurrent, iX + iWidth - 1, iY, Control::StrokeSingleLine, EDGE_NONE, EDGE_NONE, byRight, byTop,
                   sAttributesCell);
    DrawStrokeCell(*lpsRegionCurrent, iX, iY + iHeight - 1, Control::StrokeSingleLine, byLeft, byBottom, EDGE_NONE, EDGE_NONE,
                   sAttributesCell);
    DrawStrokeCell(*lpsRegionCurrent, iX + iWidth - 1, iY + iHeight - 1, Control::StrokeSingleLine, byRight, EDGE_NONE, EDGE_NONE, byBottom,
                   sAttributesCell);
}

VOID Buffer::ResetAttributes(_In_opt_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }
    lpsRegionCurrent->sAttributesCurrent = m_sAttributesDefault;
}

VOID Buffer::PutCodepoint(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    if (lpsRegionCurrent->bWrapPending != FALSE)
    {
        lpsRegionCurrent->iCursorX = 0;
        lpsRegionCurrent->iCursorY += 1;
        if (lpsRegionCurrent->iCursorY >= lpsRegionCurrent->iHeight)
        {
            ScrollRegionUp(lpsRegionCurrent, 1);
            lpsRegionCurrent->iCursorY = lpsRegionCurrent->iHeight - 1;
        }
        lpsRegionCurrent->bWrapPending = FALSE;
    }

    SetCell(*lpsRegionCurrent, lpsRegionCurrent->iCursorX, lpsRegionCurrent->iCursorY, chCodepointW, lpsRegionCurrent->sAttributesCurrent);
    AdvanceCursorAfterWrite(lpsRegionCurrent);
}

VOID Buffer::ProcessControl(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    switch (chCodepointW)
    {
    case L'\r':
        lpsRegionCurrent->iCursorX = 0;
        lpsRegionCurrent->bWrapPending = FALSE;
        break;

    case L'\n':
        lpsRegionCurrent->iCursorY += 1;
        if (lpsRegionCurrent->iCursorY >= lpsRegionCurrent->iHeight)
        {
            ScrollRegionUp(lpsRegionCurrent, 1);
            lpsRegionCurrent->iCursorY = lpsRegionCurrent->iHeight - 1;
        }
        lpsRegionCurrent->bWrapPending = FALSE;
        break;

    case L'\b':
        lpsRegionCurrent->iCursorX = (std::max)(0, lpsRegionCurrent->iCursorX - 1);
        lpsRegionCurrent->bWrapPending = FALSE;
        break;

    case L'\t':
        AdvanceToNextTabStop(lpsRegionCurrent);
        break;

    case L'\f':
        Clear(lpsRegionCurrent);
        break;
    }
}

VOID Buffer::MoveCursorRelative(_In_opt_ RegionHandle hRegion, _In_ INT iDeltaX, _In_ INT iDeltaY) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    lpsRegionCurrent->iCursorX = ClampInt(lpsRegionCurrent->iCursorX + iDeltaX, 0, lpsRegionCurrent->iWidth - 1);
    lpsRegionCurrent->iCursorY = ClampInt(lpsRegionCurrent->iCursorY + iDeltaY, 0, lpsRegionCurrent->iHeight - 1);
    lpsRegionCurrent->bWrapPending = FALSE;
}

VOID Buffer::SetCursorPosition(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased, _In_ INT iColOneBased) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    lpsRegionCurrent->iCursorY = ClampInt(iRowOneBased - 1, 0, lpsRegionCurrent->iHeight - 1);
    lpsRegionCurrent->iCursorX = ClampInt(iColOneBased - 1, 0, lpsRegionCurrent->iWidth - 1);
    lpsRegionCurrent->bWrapPending = FALSE;
}

VOID Buffer::SetCursorColumn(_In_opt_ RegionHandle hRegion, _In_ INT iColOneBased) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    SetCursorPosition(lpsRegionCurrent, lpsRegionCurrent->iCursorY + 1, iColOneBased);
}

VOID Buffer::SetCursorRow(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    SetCursorPosition(lpsRegionCurrent, iRowOneBased, lpsRegionCurrent->iCursorX + 1);
}

VOID Buffer::EraseInLine(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept
{
    Region_t *lpsRegionCurrent;
    INT iStartX;
    INT iEndX;
    INT iX;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    iStartX = 0;
    iEndX = lpsRegionCurrent->iWidth - 1;
    if (iMode == 0)
    {
        iStartX = lpsRegionCurrent->iCursorX;
    }
    else if (iMode == 1)
    {
        iEndX = lpsRegionCurrent->iCursorX;
    }

    for (iX = iStartX; iX <= iEndX; ++iX)
    {
        FillCell(*lpsRegionCurrent, iX, lpsRegionCurrent->iCursorY, m_sAttributesDefault);
    }
}

VOID Buffer::EraseInDisplay(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept
{
    Region_t *lpsRegionCurrent;
    INT iYStart;
    INT iYEnd;
    INT iY;
    INT iXStart;
    INT iXEnd;
    INT iX;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    if (iMode == 2)
    {
        Clear(lpsRegionCurrent);
        return;
    }

    iYStart = 0;
    iYEnd = lpsRegionCurrent->iHeight - 1;
    if (iMode == 0)
    {
        iYStart = lpsRegionCurrent->iCursorY;
    }
    else if (iMode == 1)
    {
        iYEnd = lpsRegionCurrent->iCursorY;
    }

    for (iY = iYStart; iY <= iYEnd; ++iY)
    {
        iXStart = 0;
        iXEnd = lpsRegionCurrent->iWidth - 1;
        if ((iMode == 0) && (iY == lpsRegionCurrent->iCursorY))
        {
            iXStart = lpsRegionCurrent->iCursorX;
        }
        if ((iMode == 1) && (iY == lpsRegionCurrent->iCursorY))
        {
            iXEnd = lpsRegionCurrent->iCursorX;
        }
        for (iX = iXStart; iX <= iXEnd; ++iX)
        {
            FillCell(*lpsRegionCurrent, iX, iY, m_sAttributesDefault);
        }
    }
}

VOID Buffer::SetGraphicsRendition(_In_opt_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams,
                                  _In_ SIZE_T uParamsCount) noexcept
{
    Region_t *lpsRegionCurrent;
    SIZE_T uIndex;
    INT iValue;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    if (uParamsCount == 0)
    {
        lpsRegionCurrent->sAttributesCurrent = m_sAttributesDefault;
        return;
    }

    for (uIndex = 0; uIndex < uParamsCount; ++uIndex)
    {
        iValue = lpiParams[uIndex];
        if (iValue == 0)
        {
            lpsRegionCurrent->sAttributesCurrent = m_sAttributesDefault;
        }
        else if (iValue == 1)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags |= Control::StyleBold;
        }
        else if (iValue == 3)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags |= Control::StyleItalic;
        }
        else if (iValue == 4)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags |= Control::StyleUnderline;
        }
        else if (iValue == 5)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags |= Control::StyleBlink;
        }
        else if (iValue == 7)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags |= Control::StyleInverse;
        }
        else if (iValue == 22)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags &= ~Control::StyleBold;
        }
        else if (iValue == 23)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags &= ~Control::StyleItalic;
        }
        else if (iValue == 24)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags &= ~Control::StyleUnderline;
        }
        else if (iValue == 25)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags &= ~Control::StyleBlink;
        }
        else if (iValue == 27)
        {
            lpsRegionCurrent->sAttributesCurrent.dwStyleFlags &= ~Control::StyleInverse;
        }
        else if ((iValue >= 30) && (iValue <= 37))
        {
            lpsRegionCurrent->sAttributesCurrent.crForeground = GetAnsi16Color(iValue - 30);
        }
        else if ((iValue >= 40) && (iValue <= 47))
        {
            lpsRegionCurrent->sAttributesCurrent.crBackground = GetAnsi16Color(iValue - 40);
        }
        else if ((iValue >= 90) && (iValue <= 97))
        {
            lpsRegionCurrent->sAttributesCurrent.crForeground = GetAnsi16Color((iValue - 90) + 8);
        }
        else if ((iValue >= 100) && (iValue <= 107))
        {
            lpsRegionCurrent->sAttributesCurrent.crBackground = GetAnsi16Color((iValue - 100) + 8);
        }
        else if (iValue == 39)
        {
            lpsRegionCurrent->sAttributesCurrent.crForeground = m_sAttributesDefault.crForeground;
        }
        else if (iValue == 49)
        {
            lpsRegionCurrent->sAttributesCurrent.crBackground = m_sAttributesDefault.crBackground;
        }
        else if ((iValue == 38) || (iValue == 48))
        {
            ApplySgrColor(lpsRegionCurrent, lpiParams, uParamsCount, &uIndex, (iValue == 38) ? TRUE : FALSE);
        }
    }
}

HRESULT Buffer::CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                             _In_opt_ RegionHandle hRegionParent) noexcept
{
    Region_t sRegionCurrent;
    Region_t *lpsRegionParent;
    Region_t *lpsRegionCreated;
    HRESULT hr;

    if (!lphRegion)
    {
        return E_POINTER;
    }
    *lphRegion = nullptr;

    hr = ValidateRegionBounds(iWidth, iHeight);
    if (FAILED(hr))
    {
        return hr;
    }

    lpsRegionParent = ResolveRegion(hRegionParent);
    if (!lpsRegionParent)
    {
        return hRegionParent ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : E_UNEXPECTED;
    }

    sRegionCurrent = Region_t{};
    sRegionCurrent.iId = m_iNextRegionId;
    sRegionCurrent.lpParent = lpsRegionParent;
    sRegionCurrent.iX = iX;
    sRegionCurrent.iY = iY;
    sRegionCurrent.iWidth = iWidth;
    sRegionCurrent.iHeight = iHeight;
    sRegionCurrent.sAttributesCurrent = m_sAttributesDefault;
    hr = InitializeRegionCells(sRegionCurrent);
    if (FAILED(hr))
    {
        return hr;
    }

    try
    {
        m_mapRegions.emplace(sRegionCurrent.iId, std::move(sRegionCurrent));
        lpsRegionCreated = &m_mapRegions.find(m_iNextRegionId)->second;
        lpsRegionParent->vecChildRegionIds.push_back(m_iNextRegionId);
        *lphRegion = lpsRegionCreated;
        m_iNextRegionId += 1;
    }
    catch (const std::bad_alloc &)
    {
        m_mapRegions.erase(m_iNextRegionId);
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        m_mapRegions.erase(m_iNextRegionId);
        return E_UNEXPECTED;
    }
    return S_OK;
}

HRESULT Buffer::CreateCustomDrawRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                                       _In_ const CustomDrawCallback &fnDrawCallback, _In_opt_ RegionHandle hRegionParent) noexcept
{
    HRESULT hr;

    if (!fnDrawCallback)
    {
        return E_INVALIDARG;
    }
    hr = CreateRegion(iX, iY, iWidth, iHeight, lphRegion, hRegionParent);
    if (FAILED(hr))
    {
        return hr;
    }
    try
    {
        (*lphRegion)->fnCustomDrawCallback = fnDrawCallback;
    }
    catch (const std::bad_alloc &)
    {
        DestroyRegion(*lphRegion);
        *lphRegion = nullptr;
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        DestroyRegion(*lphRegion);
        *lphRegion = nullptr;
        return E_UNEXPECTED;
    }
    (*lphRegion)->bCustomDraw = TRUE;
    (*lphRegion)->vecCells.clear();
    return S_OK;
}

HRESULT Buffer::DestroyRegion(_In_ RegionHandle hRegion) noexcept
{
    if ((!hRegion) || hRegion->iId == 0)
    {
        return E_INVALIDARG;
    }
    if (ResolveRegion(hRegion) != hRegion)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    return DestroyRegionRecursive(hRegion);
}

HRESULT Buffer::RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept
{
    Region_t *lpsRegionCurrent;
    std::vector<Cell> vecCellsResized;
    HRESULT hr;

    if (!hRegion)
    {
        return E_POINTER;
    }
    lpsRegionCurrent = ResolveRegion(hRegion);
    if ((!lpsRegionCurrent) || lpsRegionCurrent->iId == 0)
    {
        return E_INVALIDARG;
    }

    hr = ValidateRegionBounds(iWidth, iHeight);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = S_OK;
    if (lpsRegionCurrent->bCustomDraw == FALSE)
    {
        hr = ResizeRegionCells(*lpsRegionCurrent, vecCellsResized, iWidth, iHeight);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    lpsRegionCurrent->iX = iX;
    lpsRegionCurrent->iY = iY;
    lpsRegionCurrent->iWidth = iWidth;
    lpsRegionCurrent->iHeight = iHeight;
    lpsRegionCurrent->iCursorX = ClampInt(lpsRegionCurrent->iCursorX, 0, iWidth - 1);
    lpsRegionCurrent->iCursorY = ClampInt(lpsRegionCurrent->iCursorY, 0, iHeight - 1);
    lpsRegionCurrent->sCursorSaved.iX = ClampInt(lpsRegionCurrent->sCursorSaved.iX, 0, iWidth - 1);
    lpsRegionCurrent->sCursorSaved.iY = ClampInt(lpsRegionCurrent->sCursorSaved.iY, 0, iHeight - 1);
    if (lpsRegionCurrent->bCustomDraw == FALSE)
    {
        lpsRegionCurrent->vecCells = std::move(vecCellsResized);
    }
    return S_OK;
}

HRESULT Buffer::BringRegionToFront(_In_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;
    Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::iterator{};

    if (!hRegion)
    {
        return E_POINTER;
    }
    if (hRegion->iId == 0)
    {
        return E_INVALIDARG;
    }
    lpsRegionCurrent = ResolveRegion(hRegion);
    if (lpsRegionCurrent != hRegion)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    lpsRegionParent = ResolveRegion(lpsRegionCurrent->lpParent);
    if (!lpsRegionParent)
    {
        return E_UNEXPECTED;
    }

    itOrder = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), hRegion->iId);
    if (itOrder == lpsRegionParent->vecChildRegionIds.end())
    {
        return E_UNEXPECTED;
    }
    if ((itOrder + 1) == lpsRegionParent->vecChildRegionIds.end())
    {
        return S_OK;
    }

    lpsRegionParent->vecChildRegionIds.erase(itOrder);
    lpsRegionParent->vecChildRegionIds.push_back(hRegion->iId);
    return S_OK;
}

HRESULT Buffer::SendRegionToBack(_In_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;
    Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::iterator{};

    if (!hRegion)
    {
        return E_POINTER;
    }
    if (hRegion->iId == 0)
    {
        return E_INVALIDARG;
    }
    lpsRegionCurrent = ResolveRegion(hRegion);
    if (lpsRegionCurrent != hRegion)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    lpsRegionParent = ResolveRegion(lpsRegionCurrent->lpParent);
    if (!lpsRegionParent)
    {
        return E_UNEXPECTED;
    }

    itOrder = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), hRegion->iId);
    if (itOrder == lpsRegionParent->vecChildRegionIds.end())
    {
        return E_UNEXPECTED;
    }
    if (itOrder == lpsRegionParent->vecChildRegionIds.begin())
    {
        return S_OK;
    }

    lpsRegionParent->vecChildRegionIds.erase(itOrder);
    lpsRegionParent->vecChildRegionIds.insert(lpsRegionParent->vecChildRegionIds.begin(), hRegion->iId);
    return S_OK;
}

HRESULT Buffer::MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept
{
    Region_t *lpsRegionCurrent;
    Region_t *lpsRegionParent;
    Region_t *lpsRegionReference;
    auto itOrder = std::vector<INT>::iterator{};
    auto itReference = std::vector<INT>::iterator{};
    INT iRegionId;

    if (!hRegion)
    {
        return E_POINTER;
    }
    if (hRegion->iId == 0)
    {
        return E_INVALIDARG;
    }
    lpsRegionCurrent = ResolveRegion(hRegion);
    if (lpsRegionCurrent != hRegion)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    lpsRegionParent = ResolveRegion(lpsRegionCurrent->lpParent);
    if (!lpsRegionParent)
    {
        return E_UNEXPECTED;
    }

    lpsRegionReference = nullptr;
    if (hRegionReference)
    {
        if (hRegionReference->iId == 0)
        {
            return E_INVALIDARG;
        }
        lpsRegionReference = ResolveRegion(hRegionReference);
        if (lpsRegionReference != hRegionReference)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
        if (lpsRegionReference->lpParent != lpsRegionCurrent->lpParent)
        {
            return E_INVALIDARG;
        }
    }
    if (hRegion == hRegionReference)
    {
        return S_OK;
    }

    itOrder = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), hRegion->iId);
    if (itOrder == lpsRegionParent->vecChildRegionIds.end())
    {
        return E_UNEXPECTED;
    }

    iRegionId = *itOrder;
    lpsRegionParent->vecChildRegionIds.erase(itOrder);

    if (!hRegionReference)
    {
        lpsRegionParent->vecChildRegionIds.push_back(iRegionId);
        return S_OK;
    }

    itReference = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), hRegionReference->iId);
    if (itReference == lpsRegionParent->vecChildRegionIds.end())
    {
        return E_UNEXPECTED;
    }

    lpsRegionParent->vecChildRegionIds.insert(itReference + 1, iRegionId);
    return S_OK;
}

HRESULT Buffer::SetRegionContext(_In_opt_ RegionHandle hRegion, _In_opt_ PVOID lpContext) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    lpsRegionCurrent->lpContext = lpContext;
    return S_OK;
}

PVOID Buffer::GetRegionContext(_In_opt_ RegionHandle hRegion) const noexcept
{
    const Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return nullptr;
    }

    return lpsRegionCurrent->lpContext;
}

HRESULT Buffer::SetRegionDestroyCallback(_In_ RegionHandle hRegion, _In_ const RegionDestroyCallback &fnCallback) noexcept
{
    Region_t *lpsRegion;

    if (!hRegion)
    {
        return E_INVALIDARG;
    }
    lpsRegion = ResolveRegion(hRegion);
    if (!lpsRegion)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    try
    {
        lpsRegion->fnDestroyCallback = fnCallback;
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

HRESULT Buffer::SetCustomDrawRegionResourceCleanup(_In_ RegionHandle hRegion,
                                                   _In_ const CustomDrawResourceCleanupCallback &fnCallback) noexcept
{
    Region_t *lpsRegion;

    lpsRegion = ResolveRegion(hRegion);
    if ((!lpsRegion) || (lpsRegion->bCustomDraw == FALSE))
    {
        return E_INVALIDARG;
    }
    try
    {
        lpsRegion->fnResourceCleanupCallback = fnCallback;
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

VOID Buffer::NotifyCustomDrawResourceCleanup(_In_ CustomDrawResourceCleanupReason cleanupReason) noexcept
{
    for (auto &[iRegionId, sRegion] : m_mapRegions)
    {
        (void)iRegionId;
        if ((sRegion.bCustomDraw != FALSE) && sRegion.fnResourceCleanupCallback)
        {
            sRegion.fnResourceCleanupCallback(cleanupReason);
        }
    }
}

RegionHandle Buffer::GetFirstRegion() const noexcept
{
    return GetChildFirstRegion(nullptr);
}

RegionHandle Buffer::GetLastRegion() const noexcept
{
    return GetChildLastRegion(nullptr);
}

RegionHandle Buffer::GetNextRegion(_In_opt_ RegionHandle hRegion) const noexcept
{
    const Region_t *lpsRegionCurrent;
    const Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::const_iterator{};
    auto itRegion = m_mapRegions.end();

    if (!hRegion)
    {
        return GetFirstRegion();
    }
    lpsRegionCurrent = ResolveRegion(hRegion);
    if (lpsRegionCurrent != hRegion || hRegion->iId == 0)
    {
        return nullptr;
    }
    lpsRegionParent = ResolveRegion(lpsRegionCurrent->lpParent);
    if (!lpsRegionParent)
    {
        return nullptr;
    }

    itOrder = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), hRegion->iId);
    if (itOrder == lpsRegionParent->vecChildRegionIds.end())
    {
        return nullptr;
    }

    while (itOrder != lpsRegionParent->vecChildRegionIds.begin())
    {
        --itOrder;
        itRegion = m_mapRegions.find(*itOrder);
        if (itRegion != m_mapRegions.end())
        {
            return const_cast<Region_t *>(&itRegion->second);
        }
    }

    return nullptr;
}

RegionHandle Buffer::GetPreviousRegion(_In_opt_ RegionHandle hRegion) const noexcept
{
    const Region_t *lpsRegionCurrent;
    const Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::const_iterator{};
    auto itRegion = m_mapRegions.end();

    if (!hRegion)
    {
        return GetLastRegion();
    }
    lpsRegionCurrent = ResolveRegion(hRegion);
    if (lpsRegionCurrent != hRegion || hRegion->iId == 0)
    {
        return nullptr;
    }
    lpsRegionParent = ResolveRegion(lpsRegionCurrent->lpParent);
    if (!lpsRegionParent)
    {
        return nullptr;
    }

    itOrder = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), hRegion->iId);
    if (itOrder == lpsRegionParent->vecChildRegionIds.end())
    {
        return nullptr;
    }

    for (++itOrder; itOrder != lpsRegionParent->vecChildRegionIds.end(); ++itOrder)
    {
        itRegion = m_mapRegions.find(*itOrder);
        if (itRegion != m_mapRegions.end())
        {
            return const_cast<Region_t *>(&itRegion->second);
        }
    }

    return nullptr;
}

RegionHandle Buffer::GetChildFirstRegion(_In_opt_ RegionHandle hRegionParent) const noexcept
{
    const Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::const_reverse_iterator{};
    auto itRegion = m_mapRegions.end();

    lpsRegionParent = ResolveRegion(hRegionParent);
    if (!lpsRegionParent)
    {
        return nullptr;
    }

    for (itOrder = lpsRegionParent->vecChildRegionIds.rbegin(); itOrder != lpsRegionParent->vecChildRegionIds.rend(); ++itOrder)
    {
        itRegion = m_mapRegions.find(*itOrder);
        if (itRegion != m_mapRegions.end())
        {
            return const_cast<Region_t *>(&itRegion->second);
        }
    }

    return nullptr;
}

RegionHandle Buffer::GetChildLastRegion(_In_opt_ RegionHandle hRegionParent) const noexcept
{
    const Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::const_iterator{};
    auto itRegion = m_mapRegions.end();

    lpsRegionParent = ResolveRegion(hRegionParent);
    if (!lpsRegionParent)
    {
        return nullptr;
    }

    for (itOrder = lpsRegionParent->vecChildRegionIds.begin(); itOrder != lpsRegionParent->vecChildRegionIds.end(); ++itOrder)
    {
        itRegion = m_mapRegions.find(*itOrder);
        if (itRegion != m_mapRegions.end())
        {
            return const_cast<Region_t *>(&itRegion->second);
        }
    }

    return nullptr;
}

RegionHandle Buffer::GetParentRegion(_In_ RegionHandle hRegion) const noexcept
{
    const Region_t *lpsRegionCurrent;
    const Region_t *lpsRegionParent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if ((!lpsRegionCurrent) || lpsRegionCurrent->iId == 0)
    {
        return nullptr;
    }

    lpsRegionParent = ResolveRegion(lpsRegionCurrent->lpParent);
    if ((!lpsRegionParent) || lpsRegionParent->iId == 0)
    {
        return nullptr;
    }
    return const_cast<Region_t *>(lpsRegionParent);
}

VOID Buffer::GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                               _Out_opt_ LPINT lpiHeight) const noexcept
{
    const Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
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
        return;
    }

    if (lpiX)
    {
        *lpiX = lpsRegionCurrent->iX;
    }
    if (lpiY)
    {
        *lpiY = lpsRegionCurrent->iY;
    }
    if (lpiWidth)
    {
        *lpiWidth = lpsRegionCurrent->iWidth;
    }
    if (lpiHeight)
    {
        *lpiHeight = lpsRegionCurrent->iHeight;
    }
}

BOOL Buffer::ConvertToRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal,
                                        _Out_opt_ LPINT lpiColRegion, _Out_opt_ LPINT lpiRowRegion) const noexcept
{
    const Region_t *lpsRegionCurrent;
    LONGLONG llColRegion;
    LONGLONG llRowRegion;

    if (!hRegion)
    {
    zero_and_return_false:
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

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        goto zero_and_return_false;
    }

    GetRegionTerminalOrigin(*lpsRegionCurrent, llColRegion, llRowRegion);

    llColRegion = static_cast<LONGLONG>(iColTerminal) - llColRegion;
    llRowRegion = static_cast<LONGLONG>(iRowTerminal) - llRowRegion;
    if ((llColRegion < static_cast<LONGLONG>(INT_MIN)) || (llColRegion > static_cast<LONGLONG>(INT_MAX)) ||
        (llRowRegion < static_cast<LONGLONG>(INT_MIN)) || (llRowRegion > static_cast<LONGLONG>(INT_MAX)))
    {
        goto zero_and_return_false;
    }

    if (lpiColRegion)
    {
        *lpiColRegion = static_cast<INT>(llColRegion);
    }
    if (lpiRowRegion)
    {
        *lpiRowRegion = static_cast<INT>(llRowRegion);
    }
    return TRUE;
}

BOOL Buffer::ConvertFromRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion,
                                          _Out_opt_ LPINT lpiColTerminal, _Out_opt_ LPINT lpiRowTerminal) const noexcept
{
    const Region_t *lpsRegionCurrent;
    LONGLONG llColTerminal;
    LONGLONG llRowTerminal;
    LONGLONG llOriginX;
    LONGLONG llOriginY;

    if (!hRegion)
    {
    zero_and_return_false:
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

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        goto zero_and_return_false;
    }
    if (iColRegion < 0 || iColRegion >= lpsRegionCurrent->iWidth || iRowRegion < 0 || iRowRegion >= lpsRegionCurrent->iHeight)
    {
        goto zero_and_return_false;
    }

    GetRegionTerminalOrigin(*lpsRegionCurrent, llOriginX, llOriginY);

    llColTerminal = llOriginX + static_cast<LONGLONG>(iColRegion);
    llRowTerminal = llOriginY + static_cast<LONGLONG>(iRowRegion);
    if ((llColTerminal < static_cast<LONGLONG>(INT_MIN)) || (llColTerminal > static_cast<LONGLONG>(INT_MAX)) ||
        (llRowTerminal < static_cast<LONGLONG>(INT_MIN)) || (llRowTerminal > static_cast<LONGLONG>(INT_MAX)))
    {
        goto zero_and_return_false;
    }

    if (lpiColTerminal)
    {
        *lpiColTerminal = static_cast<INT>(llColTerminal);
    }
    if (lpiRowTerminal)
    {
        *lpiRowTerminal = static_cast<INT>(llRowTerminal);
    }
    return TRUE;
}

VOID Buffer::SaveCursor(_In_opt_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    lpsRegionCurrent->sCursorSaved.iX = lpsRegionCurrent->iCursorX;
    lpsRegionCurrent->sCursorSaved.iY = lpsRegionCurrent->iCursorY;
}

VOID Buffer::RestoreCursor(_In_opt_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    lpsRegionCurrent->iCursorX = ClampInt(lpsRegionCurrent->sCursorSaved.iX, 0, lpsRegionCurrent->iWidth - 1);
    lpsRegionCurrent->iCursorY = ClampInt(lpsRegionCurrent->sCursorSaved.iY, 0, lpsRegionCurrent->iHeight - 1);
    lpsRegionCurrent->bWrapPending = FALSE;
}

VOID Buffer::ShowCursor(_In_opt_ RegionHandle hRegion) noexcept
{
    if (!ResolveRegion(hRegion))
    {
        return;
    }

    m_hRegionCursorActive = hRegion;
    m_bCursorVisible = TRUE;
}

VOID Buffer::HideCursor() noexcept
{
    m_bCursorVisible = FALSE;
    m_hRegionCursorActive = nullptr;
}

VOID Buffer::SetCursorStyle(_In_ DWORD dwCursorStyle) noexcept
{
    switch (dwCursorStyle)
    {
    case Control::CursorBlock:
    case Control::CursorUnderscore:
    case Control::CursorBarLeft:
        m_dwCursorStyle = dwCursorStyle;
        break;
    }
}

VOID Buffer::ToggleBlinkVisibility() noexcept
{
    m_bBlinkVisible = (m_bBlinkVisible == FALSE) ? TRUE : FALSE;
}

VOID Buffer::SetBlinkVisible(_In_ BOOL bBlinkVisible) noexcept
{
    m_bBlinkVisible = (bBlinkVisible != FALSE) ? TRUE : FALSE;
}

HRESULT Buffer::GetSnapshot(_Out_ Snapshot *lpSnapshot) const noexcept
{
    const Region_t *lpsRegionRoot;
    const Region_t *lpsRegionCursor;
    LONGLONG llCursorCol;
    LONGLONG llCursorRow;
    HRESULT hr;

    if (!lpSnapshot)
    {
        return E_POINTER;
    }

    lpsRegionRoot = ResolveRegion(nullptr);
    if (!lpsRegionRoot)
    {
        return E_UNEXPECTED;
    }

    try
    {
        if (m_vecSnapshotCells.size() != static_cast<size_t>(m_iCols) * static_cast<size_t>(m_iRows))
        {
            m_vecSnapshotCells.assign(static_cast<size_t>(m_iCols) * static_cast<size_t>(m_iRows), MakeBlankCell());
        }
        m_vecSnapshotCells = lpsRegionRoot->vecCells;
        hr = ComposeRegionTree(*lpsRegionRoot);
        if (FAILED(hr))
        {
            return hr;
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

    lpSnapshot->lpCells = m_vecSnapshotCells.data();
    lpSnapshot->iCols = m_iCols;
    lpSnapshot->iRows = m_iRows;
    lpSnapshot->bBlinkVisible = m_bBlinkVisible;
    lpSnapshot->bCursorVisible = FALSE;
    lpSnapshot->iCursorCol = 0;
    lpSnapshot->iCursorRow = 0;
    lpSnapshot->dwCursorStyle = m_dwCursorStyle;
    lpSnapshot->crDefaultForeground = m_sAttributesDefault.crForeground;
    lpSnapshot->crDefaultBackground = m_sAttributesDefault.crBackground;

    lpsRegionCursor = ResolveRegion(m_hRegionCursorActive);
    if (m_bCursorVisible != FALSE && lpsRegionCursor)
    {
        CellRect_t sRectVisible;

        GetRegionTerminalOrigin(*lpsRegionCursor, llCursorCol, llCursorRow);
        llCursorCol += static_cast<LONGLONG>(lpsRegionCursor->iCursorX);
        llCursorRow += static_cast<LONGLONG>(lpsRegionCursor->iCursorY);

        GetRegionVisibleTerminalRect(*lpsRegionCursor, sRectVisible);
        if ((llCursorCol >= static_cast<LONGLONG>(sRectVisible.iX)) &&
            (llCursorCol < static_cast<LONGLONG>(sRectVisible.iX + sRectVisible.iWidth)) &&
            (llCursorRow >= static_cast<LONGLONG>(sRectVisible.iY)) &&
            (llCursorRow < static_cast<LONGLONG>(sRectVisible.iY + sRectVisible.iHeight)) &&
            (llCursorCol >= static_cast<LONGLONG>(INT_MIN)) && (llCursorCol <= static_cast<LONGLONG>(INT_MAX)) &&
            (llCursorRow >= static_cast<LONGLONG>(INT_MIN)) && (llCursorRow <= static_cast<LONGLONG>(INT_MAX)))
        {
            lpSnapshot->bCursorVisible = TRUE;
            lpSnapshot->iCursorCol = static_cast<INT>(llCursorCol);
            lpSnapshot->iCursorRow = static_cast<INT>(llCursorRow);
        }
    }

    hr = S_OK;
    return hr;
}

HRESULT Buffer::GetRenderPlan(_Out_ std::vector<RenderItem> *lpRenderItems) const noexcept
{
    const Region_t *lpsRegionRoot;

    if (!lpRenderItems)
    {
        return E_POINTER;
    }
    lpsRegionRoot = ResolveRegion(nullptr);
    if (!lpsRegionRoot)
    {
        return E_UNEXPECTED;
    }
    try
    {
        lpRenderItems->clear();
        return BuildRenderPlan(*lpsRegionRoot, *lpRenderItems);
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

HRESULT Buffer::InitializeRootRegion() noexcept
{
    Region_t sRegionRoot;
    HRESULT hr;

    sRegionRoot = Region_t{};
    sRegionRoot.iId = 0;
    sRegionRoot.iWidth = m_iCols;
    sRegionRoot.iHeight = m_iRows;
    sRegionRoot.sAttributesCurrent = m_sAttributesDefault;
    hr = InitializeRegionCells(sRegionRoot);
    if (FAILED(hr))
    {
        return hr;
    }

    try
    {
        m_mapRegions.clear();
        m_mapRegions.emplace(0, std::move(sRegionRoot));
        m_vecSnapshotCells.assign(static_cast<size_t>(m_iCols) * static_cast<size_t>(m_iRows), MakeBlankCell());
    }
    catch (const std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }
    m_hRegionCursorActive = nullptr;
    m_bCursorVisible = FALSE;
    m_dwCursorStyle = Control::CursorBlock;
    return S_OK;
}

HRESULT Buffer::ValidateRegionBounds(_In_ INT iWidth, _In_ INT iHeight) const noexcept
{
    if (iWidth <= 0 || iHeight <= 0)
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

Region_t *Buffer::ResolveRegion(_In_opt_ RegionHandle hRegion) noexcept
{
    auto itRegion = m_mapRegions.end();

    if (!hRegion)
    {
        itRegion = m_mapRegions.find(0);
        return (itRegion != m_mapRegions.end()) ? &itRegion->second : nullptr;
    }

    itRegion = m_mapRegions.find(hRegion->iId);
    if (itRegion == m_mapRegions.end() || &itRegion->second != hRegion)
    {
        return nullptr;
    }
    return &itRegion->second;
}

const Region_t *Buffer::ResolveRegion(_In_opt_ RegionHandle hRegion) const noexcept
{
    auto itRegion = m_mapRegions.end();

    if (!hRegion)
    {
        itRegion = m_mapRegions.find(0);
        return (itRegion != m_mapRegions.end()) ? &itRegion->second : nullptr;
    }

    itRegion = m_mapRegions.find(hRegion->iId);
    if (itRegion == m_mapRegions.end() || &itRegion->second != hRegion)
    {
        return nullptr;
    }
    return &itRegion->second;
}

HRESULT Buffer::RemoveRegionFromParent(_Inout_ Region_t &sRegion) noexcept
{
    Region_t *lpsRegionParent;
    auto itOrder = std::vector<INT>::iterator{};

    lpsRegionParent = ResolveRegion(sRegion.lpParent);
    if (!lpsRegionParent)
    {
        return E_UNEXPECTED;
    }

    itOrder = std::find(lpsRegionParent->vecChildRegionIds.begin(), lpsRegionParent->vecChildRegionIds.end(), sRegion.iId);
    if (itOrder == lpsRegionParent->vecChildRegionIds.end())
    {
        return E_UNEXPECTED;
    }

    lpsRegionParent->vecChildRegionIds.erase(itOrder);
    sRegion.lpParent = nullptr;
    return S_OK;
}

HRESULT Buffer::DestroyRegionRecursive(_In_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;
    HRESULT hr;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if ((!lpsRegionCurrent) || lpsRegionCurrent->iId == 0)
    {
        return E_INVALIDARG;
    }
    if (m_hRegionCursorActive == hRegion)
    {
        HideCursor();
    }

    while (!lpsRegionCurrent->vecChildRegionIds.empty())
    {
        auto itRegion = m_mapRegions.find(lpsRegionCurrent->vecChildRegionIds.back());

        if (itRegion == m_mapRegions.end())
        {
            return E_UNEXPECTED;
        }

        hr = DestroyRegionRecursive(&itRegion->second);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if ((lpsRegionCurrent->bCustomDraw != FALSE) && lpsRegionCurrent->fnResourceCleanupCallback)
    {
        lpsRegionCurrent->fnResourceCleanupCallback(CustomDrawResourceCleanupReason::RegionDestroyed);
    }
    if (lpsRegionCurrent->fnDestroyCallback)
    {
        lpsRegionCurrent->fnDestroyCallback(hRegion);
    }
    hr = RemoveRegionFromParent(*lpsRegionCurrent);
    if (FAILED(hr))
    {
        return hr;
    }

    m_mapRegions.erase(lpsRegionCurrent->iId);
    return S_OK;
}

VOID Buffer::GetRegionTerminalOrigin(_In_ const Region_t &sRegion, _Out_ LONGLONG &llX, _Out_ LONGLONG &llY) const noexcept
{
    const Region_t *lpsRegionCurrent;

    llX = 0;
    llY = 0;
    lpsRegionCurrent = &sRegion;
    while (lpsRegionCurrent && lpsRegionCurrent->iId != 0)
    {
        llX += static_cast<LONGLONG>(lpsRegionCurrent->iX);
        llY += static_cast<LONGLONG>(lpsRegionCurrent->iY);
        lpsRegionCurrent = lpsRegionCurrent->lpParent;
    }
}

VOID Buffer::GetRegionVisibleTerminalRect(_In_ const Region_t &sRegion, _Out_ CellRect_t &sRectVisible) const noexcept
{
    const Region_t *lpsRegionCurrent;
    LONGLONG llRegionLeft;
    LONGLONG llRegionTop;
    LONGLONG llRegionRight;
    LONGLONG llRegionBottom;
    LONGLONG llParentLeft;
    LONGLONG llParentTop;

    GetRegionTerminalOrigin(sRegion, llRegionLeft, llRegionTop);

    llRegionRight = llRegionLeft + static_cast<LONGLONG>(sRegion.iWidth);
    llRegionBottom = llRegionTop + static_cast<LONGLONG>(sRegion.iHeight);

    lpsRegionCurrent = sRegion.lpParent;
    while (lpsRegionCurrent)
    {
        GetRegionTerminalOrigin(*lpsRegionCurrent, llParentLeft, llParentTop);

        llRegionLeft = (std::max)(llRegionLeft, llParentLeft);
        llRegionTop = (std::max)(llRegionTop, llParentTop);
        llRegionRight = (std::min)(llRegionRight, llParentLeft + static_cast<LONGLONG>(lpsRegionCurrent->iWidth));
        llRegionBottom = (std::min)(llRegionBottom, llParentTop + static_cast<LONGLONG>(lpsRegionCurrent->iHeight));

        lpsRegionCurrent = lpsRegionCurrent->lpParent;
    }

    llRegionLeft = (std::max)(llRegionLeft, 0LL);
    llRegionTop = (std::max)(llRegionTop, 0LL);
    llRegionRight = (std::min)(llRegionRight, static_cast<LONGLONG>(m_iCols));
    llRegionBottom = (std::min)(llRegionBottom, static_cast<LONGLONG>(m_iRows));
    if (llRegionLeft >= llRegionRight || llRegionTop >= llRegionBottom)
    {
        sRectVisible = CellRect_t{};
        return;
    }

    sRectVisible.iX = static_cast<INT>(llRegionLeft);
    sRectVisible.iY = static_cast<INT>(llRegionTop);
    sRectVisible.iWidth = static_cast<INT>(llRegionRight - llRegionLeft);
    sRectVisible.iHeight = static_cast<INT>(llRegionBottom - llRegionTop);
}

Buffer::Cell Buffer::MakeBlankCell() const noexcept
{
    Cell sCellBlank;

    sCellBlank = Cell{};
    sCellBlank.chCodepointW = L' ';
    sCellBlank.crForeground = m_sAttributesDefault.crForeground;
    sCellBlank.crBackground = m_sAttributesDefault.crBackground;
    sCellBlank.dwStyleFlags = Control::StyleNone;
    sCellBlank.bIsDirty = TRUE;
    return sCellBlank;
}

HRESULT Buffer::InitializeRegionCells(_Inout_ Region_t &sRegion) const noexcept
{
    try
    {
        sRegion.vecCells.assign(static_cast<size_t>(sRegion.iWidth) * static_cast<size_t>(sRegion.iHeight), MakeBlankCell());
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

HRESULT Buffer::ResizeRegionCells(_In_ const Region_t &sRegionSource, _Out_ std::vector<Cell> &vecCellsTarget, _In_ INT iWidthTarget,
                                  _In_ INT iHeightTarget) const noexcept
{
    INT iCopyWidth;
    INT iCopyHeight;
    INT iY;
    INT iX;
    SIZE_T uSourceIndex;
    SIZE_T uTargetIndex;

    try
    {
        vecCellsTarget.assign(static_cast<size_t>(iWidthTarget) * static_cast<size_t>(iHeightTarget), MakeBlankCell());
        iCopyWidth = (std::min)(sRegionSource.iWidth, iWidthTarget);
        iCopyHeight = (std::min)(sRegionSource.iHeight, iHeightTarget);
        for (iY = 0; iY < iCopyHeight; ++iY)
        {
            for (iX = 0; iX < iCopyWidth; ++iX)
            {
                if (GetCellIndex(iX, iY, sRegionSource.iWidth, &uSourceIndex) != FALSE &&
                    GetCellIndex(iX, iY, iWidthTarget, &uTargetIndex) != FALSE)
                {
                    vecCellsTarget[uTargetIndex] = sRegionSource.vecCells[uSourceIndex];
                    vecCellsTarget[uTargetIndex].bIsDirty = TRUE;
                }
            }
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

VOID Buffer::ClearRegionCells(_Inout_ Region_t &sRegion) const noexcept
{
    std::fill(sRegion.vecCells.begin(), sRegion.vecCells.end(), MakeBlankCell());
}

VOID Buffer::SetCell(_Inout_ Region_t &sRegion, _In_ INT iX, _In_ INT iY, _In_ WCHAR chCodepointW,
                     _In_ const Attributes_t &sAttributesCell) noexcept
{
    SIZE_T uIndex;

    if (GetCellIndex(iX, iY, sRegion.iWidth, &uIndex) == FALSE || iY < 0 || iY >= sRegion.iHeight)
    {
        return;
    }

    sRegion.vecCells[uIndex].chCodepointW = chCodepointW;
    sRegion.vecCells[uIndex].crForeground = sAttributesCell.crForeground;
    sRegion.vecCells[uIndex].crBackground = sAttributesCell.crBackground;
    sRegion.vecCells[uIndex].dwStyleFlags = sAttributesCell.dwStyleFlags;
    sRegion.vecCells[uIndex].bIsDirty = TRUE;
}

VOID Buffer::FillCell(_Inout_ Region_t &sRegion, _In_ INT iX, _In_ INT iY, _In_ const Attributes_t &sAttributesCell) noexcept
{
    SetCell(sRegion, iX, iY, L' ', sAttributesCell);
}

VOID Buffer::FillRange(_Inout_ Region_t &sRegion, _In_ INT iXStart, _In_ INT iYStart, _In_ INT iXEnd, _In_ INT iYEnd,
                       _In_ const Attributes_t &sAttributesCell) noexcept
{
    INT iY;
    INT iX;

    for (iY = iYStart; iY <= iYEnd; ++iY)
    {
        for (iX = iXStart; iX <= iXEnd; ++iX)
        {
            FillCell(sRegion, iX, iY, sAttributesCell);
        }
    }
}

BOOL Buffer::IsCoordinateInsideRectangle(_In_ INT iX, _In_ INT iY, _In_ INT iStartX, _In_ INT iStartY, _In_ INT iEndX,
                                         _In_ INT iEndY) const noexcept
{
    return ((iX >= iStartX) && (iX <= iEndX) && (iY >= iStartY) && (iY <= iEndY)) ? TRUE : FALSE;
}

const Buffer::Cell *Buffer::GetCell(_In_ const Region_t &sRegion, _In_ INT iX, _In_ INT iY) const noexcept
{
    SIZE_T uIndex;

    if (iY < 0 || iY >= sRegion.iHeight || GetCellIndex(iX, iY, sRegion.iWidth, &uIndex) == FALSE)
    {
        return nullptr;
    }
    return &sRegion.vecCells[uIndex];
}

VOID Buffer::DrawStrokeCell(_Inout_ Region_t &sRegion, _In_ INT iX, _In_ INT iY, _In_ DWORD dwStrokeType, _In_ BYTE byUp, _In_ BYTE byRight,
                            _In_ BYTE byDown, _In_ BYTE byLeft, _In_ const Attributes_t &sAttributesCell) noexcept
{
    const Cell *lpsCellCurrent;
    WCHAR chGlyphW;
    BoxEdges sIncoming;
    BoxEdges sExisting;
    BoxEdges sMerged;

    if (iX < 0 || iX >= sRegion.iWidth || iY < 0 || iY >= sRegion.iHeight)
    {
        return;
    }

    if (IsStrokeMergeable(dwStrokeType) == FALSE)
    {
        SetCell(sRegion, iX, iY, GetStrokeGlyph(dwStrokeType), sAttributesCell);
        return;
    }

    sIncoming = MakeBoxEdges(byUp, byRight, byDown, byLeft);
    if (TryEncodeBoxGlyph(sIncoming, &chGlyphW) == FALSE)
    {
        return;
    }

    lpsCellCurrent = GetCell(sRegion, iX, iY);
    if (lpsCellCurrent && TryDecodeBoxGlyph(lpsCellCurrent->chCodepointW, &sExisting) != FALSE &&
        TryMergeBoxEdges(sExisting, sIncoming, &sMerged) != FALSE && TryEncodeBoxGlyph(sMerged, &chGlyphW) != FALSE)
    {
        SetCell(sRegion, iX, iY, chGlyphW, sAttributesCell);
        return;
    }

    SetCell(sRegion, iX, iY, chGlyphW, sAttributesCell);
}

BOOL Buffer::ClipRectangle(_In_ const Region_t &sRegion, _In_ const CellRect_t &sRectSource, _Out_ CellRect_t &sRectClipped) const noexcept
{
    LONGLONG llStartX;
    LONGLONG llStartY;
    LONGLONG llEndXExclusive;
    LONGLONG llEndYExclusive;

    if (sRectSource.iWidth <= 0 || sRectSource.iHeight <= 0)
    {
        sRectClipped = CellRect_t{};
        return FALSE;
    }

    llStartX = static_cast<LONGLONG>(sRectSource.iX);
    llStartY = static_cast<LONGLONG>(sRectSource.iY);
    llEndXExclusive = llStartX + static_cast<LONGLONG>(sRectSource.iWidth);
    llEndYExclusive = llStartY + static_cast<LONGLONG>(sRectSource.iHeight);
    if (llEndXExclusive <= 0 || llEndYExclusive <= 0 || llStartX >= sRegion.iWidth || llStartY >= sRegion.iHeight)
    {
        sRectClipped = CellRect_t{};
        return FALSE;
    }

    sRectClipped.iX = static_cast<INT>((std::max)(llStartX, 0LL));
    sRectClipped.iY = static_cast<INT>((std::max)(llStartY, 0LL));
    sRectClipped.iWidth =
        static_cast<INT>((std::min)(llEndXExclusive, static_cast<LONGLONG>(sRegion.iWidth)) - static_cast<LONGLONG>(sRectClipped.iX));
    sRectClipped.iHeight =
        static_cast<INT>((std::min)(llEndYExclusive, static_cast<LONGLONG>(sRegion.iHeight)) - static_cast<LONGLONG>(sRectClipped.iY));
    return (sRectClipped.iWidth > 0 && sRectClipped.iHeight > 0) ? TRUE : FALSE;
}

VOID Buffer::ScrollRegionUp(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
{
    Region_t *lpsRegionCurrent;
    INT iY;
    INT iX;
    SIZE_T uSourceIndex;
    SIZE_T uTargetIndex;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    if (iLineCount <= 0)
    {
        return;
    }
    if (iLineCount >= lpsRegionCurrent->iHeight)
    {
        Clear(lpsRegionCurrent);
        return;
    }

    for (iY = 0; iY < lpsRegionCurrent->iHeight - iLineCount; ++iY)
    {
        for (iX = 0; iX < lpsRegionCurrent->iWidth; ++iX)
        {
            if (GetCellIndex(iX, iY + iLineCount, lpsRegionCurrent->iWidth, &uSourceIndex) != FALSE &&
                GetCellIndex(iX, iY, lpsRegionCurrent->iWidth, &uTargetIndex) != FALSE)
            {
                lpsRegionCurrent->vecCells[uTargetIndex] = lpsRegionCurrent->vecCells[uSourceIndex];
                lpsRegionCurrent->vecCells[uTargetIndex].bIsDirty = TRUE;
            }
        }
    }
    FillRange(*lpsRegionCurrent, 0, lpsRegionCurrent->iHeight - iLineCount, lpsRegionCurrent->iWidth - 1, lpsRegionCurrent->iHeight - 1,
              m_sAttributesDefault);
}

VOID Buffer::ScrollRegionDown(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
{
    Region_t *lpsRegionCurrent;
    INT iY;
    INT iX;
    SIZE_T uSourceIndex;
    SIZE_T uTargetIndex;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    if (iLineCount <= 0)
    {
        return;
    }
    if (iLineCount >= lpsRegionCurrent->iHeight)
    {
        Clear(lpsRegionCurrent);
        return;
    }

    for (iY = lpsRegionCurrent->iHeight - 1; iY >= iLineCount; --iY)
    {
        for (iX = 0; iX < lpsRegionCurrent->iWidth; ++iX)
        {
            if (GetCellIndex(iX, iY - iLineCount, lpsRegionCurrent->iWidth, &uSourceIndex) != FALSE &&
                GetCellIndex(iX, iY, lpsRegionCurrent->iWidth, &uTargetIndex) != FALSE)
            {
                lpsRegionCurrent->vecCells[uTargetIndex] = lpsRegionCurrent->vecCells[uSourceIndex];
                lpsRegionCurrent->vecCells[uTargetIndex].bIsDirty = TRUE;
            }
        }
    }
    FillRange(*lpsRegionCurrent, 0, 0, lpsRegionCurrent->iWidth - 1, iLineCount - 1, m_sAttributesDefault);
}

VOID Buffer::AdvanceCursorAfterWrite(_In_opt_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    if (lpsRegionCurrent->iCursorX == lpsRegionCurrent->iWidth - 1)
    {
        lpsRegionCurrent->bWrapPending = TRUE;
    }
    else
    {
        lpsRegionCurrent->iCursorX += 1;
    }
}

BOOL Buffer::GetCellIndex(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _Out_ PSIZE_T lpuIndex) const noexcept
{
    if (!lpuIndex)
    {
        return FALSE;
    }
    if (IsWithinBounds(iX, 0, iWidth) == FALSE || iY < 0)
    {
        *lpuIndex = 0;
        return FALSE;
    }
    *lpuIndex = static_cast<SIZE_T>(iY * iWidth + iX);
    return TRUE;
}

HRESULT Buffer::ComposeRegion(_In_ const Region_t &sRegion) const noexcept
{
    CellRect_t sRectVisible;
    INT iLocalStartX;
    INT iLocalStartY;
    INT iLocalEndX;
    INT iLocalEndY;
    INT iLocalX;
    INT iLocalY;
    INT iTerminalX;
    INT iTerminalY;
    SIZE_T uSourceIndex;
    SIZE_T uTargetIndex;
    LONGLONG llOriginX;
    LONGLONG llOriginY;

    GetRegionTerminalOrigin(sRegion, llOriginX, llOriginY);
    if (sRegion.bCustomDraw != FALSE)
    {
        return S_OK;
    }
    GetRegionVisibleTerminalRect(sRegion, sRectVisible);
    if (sRectVisible.iWidth <= 0 || sRectVisible.iHeight <= 0)
    {
        return S_OK;
    }

    iLocalStartX = static_cast<INT>(static_cast<LONGLONG>(sRectVisible.iX) - llOriginX);
    iLocalStartY = static_cast<INT>(static_cast<LONGLONG>(sRectVisible.iY) - llOriginY);
    iLocalEndX = iLocalStartX + sRectVisible.iWidth;
    iLocalEndY = iLocalStartY + sRectVisible.iHeight;
    if (iLocalStartX >= iLocalEndX || iLocalStartY >= iLocalEndY)
    {
        return S_OK;
    }

    for (iLocalY = iLocalStartY; iLocalY < iLocalEndY; ++iLocalY)
    {
        iTerminalY = static_cast<INT>(llOriginY + static_cast<LONGLONG>(iLocalY));
        for (iLocalX = iLocalStartX; iLocalX < iLocalEndX; ++iLocalX)
        {
            iTerminalX = static_cast<INT>(llOriginX + static_cast<LONGLONG>(iLocalX));
            if (GetCellIndex(iLocalX, iLocalY, sRegion.iWidth, &uSourceIndex) != FALSE &&
                GetCellIndex(iTerminalX, iTerminalY, m_iCols, &uTargetIndex) != FALSE)
            {
                m_vecSnapshotCells[uTargetIndex] = sRegion.vecCells[uSourceIndex];
                m_vecSnapshotCells[uTargetIndex].bIsDirty = TRUE;
            }
        }
    }

    return S_OK;
}

HRESULT Buffer::ComposeRegionTree(_In_ const Region_t &sRegionParent) const noexcept
{
    HRESULT hr;

    for (const auto iRegionId : sRegionParent.vecChildRegionIds)
    {
        auto itRegion = m_mapRegions.find(iRegionId);

        if (itRegion == m_mapRegions.end())
        {
            return E_UNEXPECTED;
        }

        hr = ComposeRegion(itRegion->second);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = ComposeRegionTree(itRegion->second);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT Buffer::BuildRenderPlan(_In_ const Region_t &sRegion, _Inout_ std::vector<RenderItem> &vecRenderItems) const noexcept
{
    RenderItem sRenderItem;
    HRESULT hr;

    GetRegionVisibleTerminalRect(sRegion, sRenderItem.sVisibleRect);
    if ((sRenderItem.sVisibleRect.iWidth > 0) && (sRenderItem.sVisibleRect.iHeight > 0))
    {
        sRenderItem.lpsRegion = &sRegion;
        GetRegionTerminalOrigin(sRegion, sRenderItem.llOriginX, sRenderItem.llOriginY);
        vecRenderItems.push_back(sRenderItem);
    }
    for (const auto iRegionId : sRegion.vecChildRegionIds)
    {
        const auto itRegion = m_mapRegions.find(iRegionId);
        if (itRegion == m_mapRegions.end())
        {
            return E_UNEXPECTED;
        }
        hr = BuildRenderPlan(itRegion->second, vecRenderItems);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    return S_OK;
}

VOID Buffer::ApplySgrColor(_In_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams, _In_ SIZE_T uParamsCount,
                           _Inout_ PSIZE_T lpuIndex, _In_ BOOL bForeground) noexcept
{
    Region_t *lpsRegionCurrent;
    SIZE_T uIndex;
    INT iMode;
    COLORREF crColor;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    uIndex = *lpuIndex;
    if (uIndex + 1U >= uParamsCount)
    {
        return;
    }
    iMode = lpiParams[uIndex + 1U];
    if (iMode == 5)
    {
        if (uIndex + 2U >= uParamsCount)
        {
            return;
        }
        crColor = GetXterm256Color(lpiParams[uIndex + 2U]);
        *lpuIndex = uIndex + 2U;
    }
    else if (iMode == 2)
    {
        if (uIndex + 4U >= uParamsCount)
        {
            return;
        }
        crColor = MakeColor(static_cast<BYTE>(ClampInt(lpiParams[uIndex + 2U], 0, 255)),
                            static_cast<BYTE>(ClampInt(lpiParams[uIndex + 3U], 0, 255)),
                            static_cast<BYTE>(ClampInt(lpiParams[uIndex + 4U], 0, 255)));
        *lpuIndex = uIndex + 4U;
    }
    else
    {
        return;
    }

    if (bForeground != FALSE)
    {
        lpsRegionCurrent->sAttributesCurrent.crForeground = crColor;
    }
    else
    {
        lpsRegionCurrent->sAttributesCurrent.crBackground = crColor;
    }
}

VOID Buffer::AdvanceToNextTabStop(_In_opt_ RegionHandle hRegion) noexcept
{
    Region_t *lpsRegionCurrent;
    INT iNextStop;
    INT iStop;

    lpsRegionCurrent = ResolveRegion(hRegion);
    if (!lpsRegionCurrent)
    {
        return;
    }

    iNextStop = lpsRegionCurrent->iWidth - 1;
    for (iStop = 0; iStop < lpsRegionCurrent->iWidth; iStop += 8)
    {
        if (iStop > lpsRegionCurrent->iCursorX)
        {
            iNextStop = (std::min)(iStop, lpsRegionCurrent->iWidth - 1);
            break;
        }
    }
    lpsRegionCurrent->iCursorX = iNextStop;
    lpsRegionCurrent->bWrapPending = FALSE;
}

} // namespace GuiTerminal::Internals

// -----------------------------------------------------------------------------

static BOOL IsStrokeMergeable(_In_ DWORD dwStrokeType) noexcept
{
    return ((dwStrokeType == GuiTerminal::Control::StrokeSingleLine) || (dwStrokeType == GuiTerminal::Control::StrokeDoubleLine)) ? TRUE
                                                                                                                                  : FALSE;
}

static WCHAR GetStrokeGlyph(_In_ DWORD dwStrokeType) noexcept
{
    switch (dwStrokeType)
    {
    case GuiTerminal::Control::StrokeDoubleLine:
        return L'\x2550';
    case GuiTerminal::Control::StrokeShadeLight:
        return L'\x2591';
    case GuiTerminal::Control::StrokeShadeMedium:
        return L'\x2592';
    case GuiTerminal::Control::StrokeShadeDark:
        return L'\x2593';
    case GuiTerminal::Control::StrokeSolidBlock:
        return L'\x2588';
    case GuiTerminal::Control::StrokeSingleLine:
    default:
        return L'\x2500';
    }
}

static BoxEdges MakeBoxEdges(_In_ BYTE byUp, _In_ BYTE byRight, _In_ BYTE byDown, _In_ BYTE byLeft) noexcept
{
    BoxEdges sEdges;

    sEdges.byUp = byUp;
    sEdges.byRight = byRight;
    sEdges.byDown = byDown;
    sEdges.byLeft = byLeft;
    return sEdges;
}

static BOOL TryDecodeBoxGlyph(_In_ WCHAR chGlyphW, _Out_ BoxEdges *lpsEdges) noexcept
{
    typedef struct BoxGlyphEntry_s
    {
        WCHAR chGlyphW;
        BoxEdges sEdges;
    } BoxGlyphEntry;

    static const BoxGlyphEntry s_arrGlyphs[40U] = {{L'\x2500', MakeBoxEdges(EDGE_NONE, EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE)},
                                                   {L'\x2502', MakeBoxEdges(EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE, EDGE_NONE)},
                                                   {L'\x250C', MakeBoxEdges(EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE)},
                                                   {L'\x2510', MakeBoxEdges(EDGE_NONE, EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE)},
                                                   {L'\x2514', MakeBoxEdges(EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE, EDGE_NONE)},
                                                   {L'\x2518', MakeBoxEdges(EDGE_SINGLE, EDGE_NONE, EDGE_NONE, EDGE_SINGLE)},
                                                   {L'\x251C', MakeBoxEdges(EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE)},
                                                   {L'\x2524', MakeBoxEdges(EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE)},
                                                   {L'\x252C', MakeBoxEdges(EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE)},
                                                   {L'\x2534', MakeBoxEdges(EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE)},
                                                   {L'\x253C', MakeBoxEdges(EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE)},
                                                   {L'\x2550', MakeBoxEdges(EDGE_NONE, EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE)},
                                                   {L'\x2551', MakeBoxEdges(EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE, EDGE_NONE)},
                                                   {L'\x2554', MakeBoxEdges(EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE)},
                                                   {L'\x2557', MakeBoxEdges(EDGE_NONE, EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE)},
                                                   {L'\x255A', MakeBoxEdges(EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE, EDGE_NONE)},
                                                   {L'\x255D', MakeBoxEdges(EDGE_DOUBLE, EDGE_NONE, EDGE_NONE, EDGE_DOUBLE)},
                                                   {L'\x2560', MakeBoxEdges(EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE)},
                                                   {L'\x2563', MakeBoxEdges(EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE)},
                                                   {L'\x2566', MakeBoxEdges(EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE)},
                                                   {L'\x2569', MakeBoxEdges(EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE)},
                                                   {L'\x256C', MakeBoxEdges(EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE)},
                                                   {L'\x2552', MakeBoxEdges(EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE)},
                                                   {L'\x2553', MakeBoxEdges(EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE)},
                                                   {L'\x2555', MakeBoxEdges(EDGE_NONE, EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE)},
                                                   {L'\x2556', MakeBoxEdges(EDGE_NONE, EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE)},
                                                   {L'\x2558', MakeBoxEdges(EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE, EDGE_NONE)},
                                                   {L'\x2559', MakeBoxEdges(EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE, EDGE_NONE)},
                                                   {L'\x255B', MakeBoxEdges(EDGE_SINGLE, EDGE_NONE, EDGE_NONE, EDGE_DOUBLE)},
                                                   {L'\x255C', MakeBoxEdges(EDGE_DOUBLE, EDGE_NONE, EDGE_NONE, EDGE_SINGLE)},
                                                   {L'\x255E', MakeBoxEdges(EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE)},
                                                   {L'\x255F', MakeBoxEdges(EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE)},
                                                   {L'\x2561', MakeBoxEdges(EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE)},
                                                   {L'\x2562', MakeBoxEdges(EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE)},
                                                   {L'\x2564', MakeBoxEdges(EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE)},
                                                   {L'\x2565', MakeBoxEdges(EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE)},
                                                   {L'\x2567', MakeBoxEdges(EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE)},
                                                   {L'\x2568', MakeBoxEdges(EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE)},
                                                   {L'\x256A', MakeBoxEdges(EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE)},
                                                   {L'\x256B', MakeBoxEdges(EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE)}};

    if (!lpsEdges)
    {
        return FALSE;
    }

    for (const auto &sEntry : s_arrGlyphs)
    {
        if (sEntry.chGlyphW == chGlyphW)
        {
            *lpsEdges = sEntry.sEdges;
            return TRUE;
        }
    }

    *lpsEdges = BoxEdges{};
    return FALSE;
}

static BOOL TryEncodeBoxGlyph(_In_ const BoxEdges &sEdges, _Out_ WCHAR *lpchGlyphW) noexcept
{
    typedef struct BoxEncodeEntry_s
    {
        BoxEdges sEdges;
        WCHAR chGlyphW;
    } BoxEncodeEntry;

    static const BoxEncodeEntry s_arrEncodings[40U] = {{{EDGE_NONE, EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE}, L'\x2500'},
                                                       {{EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE, EDGE_NONE}, L'\x2502'},
                                                       {{EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE}, L'\x250C'},
                                                       {{EDGE_NONE, EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE}, L'\x2510'},
                                                       {{EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE, EDGE_NONE}, L'\x2514'},
                                                       {{EDGE_SINGLE, EDGE_NONE, EDGE_NONE, EDGE_SINGLE}, L'\x2518'},
                                                       {{EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE}, L'\x251C'},
                                                       {{EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE}, L'\x2524'},
                                                       {{EDGE_NONE, EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE}, L'\x252C'},
                                                       {{EDGE_SINGLE, EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE}, L'\x2534'},
                                                       {{EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE, EDGE_SINGLE}, L'\x253C'},
                                                       {{EDGE_NONE, EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE}, L'\x2550'},
                                                       {{EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE, EDGE_NONE}, L'\x2551'},
                                                       {{EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE}, L'\x2554'},
                                                       {{EDGE_NONE, EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE}, L'\x2557'},
                                                       {{EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE, EDGE_NONE}, L'\x255A'},
                                                       {{EDGE_DOUBLE, EDGE_NONE, EDGE_NONE, EDGE_DOUBLE}, L'\x255D'},
                                                       {{EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE}, L'\x2560'},
                                                       {{EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE}, L'\x2563'},
                                                       {{EDGE_NONE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE}, L'\x2566'},
                                                       {{EDGE_DOUBLE, EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE}, L'\x2569'},
                                                       {{EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE, EDGE_DOUBLE}, L'\x256C'},
                                                       {{EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE}, L'\x2552'},
                                                       {{EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE}, L'\x2553'},
                                                       {{EDGE_NONE, EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE}, L'\x2555'},
                                                       {{EDGE_NONE, EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE}, L'\x2556'},
                                                       {{EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE, EDGE_NONE}, L'\x2558'},
                                                       {{EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE, EDGE_NONE}, L'\x2559'},
                                                       {{EDGE_SINGLE, EDGE_NONE, EDGE_NONE, EDGE_DOUBLE}, L'\x255B'},
                                                       {{EDGE_DOUBLE, EDGE_NONE, EDGE_NONE, EDGE_SINGLE}, L'\x255C'},
                                                       {{EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE}, L'\x255E'},
                                                       {{EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE}, L'\x255F'},
                                                       {{EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE}, L'\x2561'},
                                                       {{EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE}, L'\x2562'},
                                                       {{EDGE_NONE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE}, L'\x2564'},
                                                       {{EDGE_NONE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE}, L'\x2565'},
                                                       {{EDGE_SINGLE, EDGE_DOUBLE, EDGE_NONE, EDGE_DOUBLE}, L'\x2567'},
                                                       {{EDGE_DOUBLE, EDGE_SINGLE, EDGE_NONE, EDGE_SINGLE}, L'\x2568'},
                                                       {{EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE}, L'\x256A'},
                                                       {{EDGE_DOUBLE, EDGE_SINGLE, EDGE_DOUBLE, EDGE_SINGLE}, L'\x256B'}};

    if (!lpchGlyphW)
    {
        return FALSE;
    }

    for (const auto &sEncoding : s_arrEncodings)
    {
        if (sEncoding.sEdges.byUp == sEdges.byUp && sEncoding.sEdges.byRight == sEdges.byRight &&
            sEncoding.sEdges.byDown == sEdges.byDown && sEncoding.sEdges.byLeft == sEdges.byLeft)
        {
            *lpchGlyphW = sEncoding.chGlyphW;
            return TRUE;
        }
    }

    *lpchGlyphW = 0;
    return FALSE;
}

static BOOL TryMergeBoxEdges(_In_ const BoxEdges &sExisting, _In_ const BoxEdges &sIncoming, _Out_ BoxEdges *lpsMerged) noexcept
{
    auto mergeEdge = [](_In_ BYTE byExisting, _In_ BYTE byIncoming, _Out_ BYTE *lpbyMerged) -> BOOL {
        if (!lpbyMerged)
        {
            return FALSE;
        }
        if (byExisting == EDGE_NONE)
        {
            *lpbyMerged = byIncoming;
            return TRUE;
        }
        if (byIncoming == EDGE_NONE)
        {
            *lpbyMerged = byExisting;
            return TRUE;
        }
        if (byExisting == byIncoming)
        {
            *lpbyMerged = byExisting;
            return TRUE;
        }
        return FALSE;
    };

    if (!lpsMerged)
    {
        return FALSE;
    }
    if (mergeEdge(sExisting.byUp, sIncoming.byUp, &lpsMerged->byUp) == FALSE ||
        mergeEdge(sExisting.byRight, sIncoming.byRight, &lpsMerged->byRight) == FALSE ||
        mergeEdge(sExisting.byDown, sIncoming.byDown, &lpsMerged->byDown) == FALSE ||
        mergeEdge(sExisting.byLeft, sIncoming.byLeft, &lpsMerged->byLeft) == FALSE)
    {
        return FALSE;
    }
    return TRUE;
}

// -----------------------------------------------------------------------------

static COLORREF MakeColor(_In_ BYTE byRed, _In_ BYTE byGreen, _In_ BYTE byBlue) noexcept
{
    return RGB(byRed, byGreen, byBlue);
}

static GuiTerminal::Internals::Attributes_t MakeAttributes(_In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                                           _In_ DWORD dwStyleFlags) noexcept
{
    GuiTerminal::Internals::Attributes_t attributesCell;

    attributesCell = GuiTerminal::Internals::Attributes_t{};
    attributesCell.crForeground = crForeground;
    attributesCell.crBackground = crBackground;
    attributesCell.dwStyleFlags = dwStyleFlags;
    return attributesCell;
}

static BOOL IsWithinBounds(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumExclusive) noexcept
{
    return ((iValue >= iMinimum) && (iValue < iMaximumExclusive)) ? TRUE : FALSE;
}

static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept
{
    return (std::max)(iMinimum, (std::min)(iValue, iMaximumValue));
}

static COLORREF GetAnsi16Color(_In_ INT iIndex) noexcept
{
    static const COLORREF s_arrPalette[16U] = {
        MakeColor(12U, 12U, 12U),    MakeColor(197U, 15U, 31U),  MakeColor(19U, 161U, 14U),  MakeColor(193U, 156U, 0U),
        MakeColor(0U, 55U, 218U),    MakeColor(136U, 23U, 152U), MakeColor(58U, 150U, 221U), MakeColor(204U, 204U, 204U),
        MakeColor(118U, 118U, 118U), MakeColor(231U, 72U, 86U),  MakeColor(22U, 198U, 12U),  MakeColor(249U, 241U, 165U),
        MakeColor(59U, 120U, 255U),  MakeColor(180U, 0U, 158U),  MakeColor(97U, 214U, 214U), MakeColor(242U, 242U, 242U)};
    INT iPaletteIndex;

    iPaletteIndex = ClampInt(iIndex, 0, 15);
    return s_arrPalette[static_cast<size_t>(iPaletteIndex)];
}

static COLORREF GetXterm256Color(_In_ INT iIndex) noexcept
{
    static const INT s_arrLevels[6U] = {0, 95, 135, 175, 215, 255};
    INT iColorIndex;
    INT iCubeIndex;
    INT iRed;
    INT iGreen;
    INT iBlue;
    INT iGray;

    iColorIndex = ClampInt(iIndex, 0, 255);
    if (iColorIndex < 16)
    {
        return GetAnsi16Color(iColorIndex);
    }
    if (iColorIndex < 232)
    {
        iCubeIndex = iColorIndex - 16;
        iRed = iCubeIndex / 36;
        iGreen = (iCubeIndex / 6) % 6;
        iBlue = iCubeIndex % 6;
        return MakeColor(static_cast<BYTE>(s_arrLevels[static_cast<size_t>(iRed)]),
                         static_cast<BYTE>(s_arrLevels[static_cast<size_t>(iGreen)]),
                         static_cast<BYTE>(s_arrLevels[static_cast<size_t>(iBlue)]));
    }
    iGray = 8 + ((iColorIndex - 232) * 10);
    return MakeColor(static_cast<BYTE>(iGray), static_cast<BYTE>(iGray), static_cast<BYTE>(iGray));
}
