#include "..\include\GuiTerminalBuffer.h"
#include "..\include\GuiTerminalControl.h"
#include <algorithm>
#include <array>

// -----------------------------------------------------------------------------

static COLORREF MakeColor(_In_ BYTE byRed, _In_ BYTE byGreen, _In_ BYTE byBlue) noexcept;
static GuiTerminal::Internals::Attributes MakeAttributes(_In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                                         _In_ DWORD dwStyleFlags) noexcept;
static BOOL IsWithinBounds(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumExclusive) noexcept;
static INT ClampInt(_In_ INT iValue, _In_ INT iMinimum, _In_ INT iMaximumValue) noexcept;
static COLORREF GetAnsi16Color(_In_ INT iIndex) noexcept;
static COLORREF GetXterm256Color(_In_ INT iIndex) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal::Internals
{
    HRESULT Buffer::Initialize(_In_ INT iCols, _In_ INT iRows, _In_ COLORREF crDefaultForeground,
                               _In_ COLORREF crDefaultBackground) noexcept
    {
        Region_s sDefaultRegion;
        HRESULT hr;

        m_sAttributesDefault = MakeAttributes(MakeColor(GetRValue(crDefaultForeground),
                                                        GetGValue(crDefaultForeground),
                                                        GetBValue(crDefaultForeground)),
                                              MakeColor(GetRValue(crDefaultBackground),
                                                        GetGValue(crDefaultBackground),
                                                        GetBValue(crDefaultBackground)),
                                              Control::StyleNone);
        m_iCols = iCols;
        m_iRows = iRows;
        hr = InitializeCells();
        if (FAILED(hr))
        {
            return hr;
        }
        m_bBlinkVisible = TRUE;

        sDefaultRegion = {};
        sDefaultRegion.iWidth = iCols;
        sDefaultRegion.iHeight = iRows;
        sDefaultRegion.sAttributesCurrent = m_sAttributesDefault;
        try
        {
            m_mapRegions.clear();
            m_mapRegions.emplace(0, sDefaultRegion);
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

    HRESULT Buffer::Resize(_In_ INT iCols, _In_ INT iRows) noexcept
    {
        std::vector<Cell> vecNewCells;
        std::unordered_map<INT, Region_s> mapNewRegions;
        Region_s sDefaultRegion;
        Cell cellBlank;
        INT iCopyCols;
        INT iCopyRows;
        INT iRow;
        INT iCol;
        size_t uCellsCount;

        if ((iCols <= 0) || (iRows <= 0))
        {
            return E_INVALIDARG;
        }

        iCopyCols = (std::min)(m_iCols, iCols);
        iCopyRows = (std::min)(m_iRows, iRows);

        sDefaultRegion = {};
        sDefaultRegion.iWidth = iCols;
        sDefaultRegion.iHeight = iRows;
        sDefaultRegion.sAttributesCurrent = m_sAttributesDefault;
        cellBlank = Cell{};
        cellBlank.chCodepointW = L' ';
        cellBlank.crForeground = m_sAttributesDefault.crForeground;
        cellBlank.crBackground = m_sAttributesDefault.crBackground;
        cellBlank.dwStyleFlags = Control::StyleNone;
        cellBlank.bIsDirty = TRUE;
        uCellsCount = static_cast<size_t>(iCols) * static_cast<size_t>(iRows);
        try
        {
            vecNewCells.assign(uCellsCount, cellBlank);
            for (iRow = 0; iRow < iCopyRows; ++iRow)
            {
                for (iCol = 0; iCol < iCopyCols; ++iCol)
                {
                    vecNewCells[static_cast<size_t>(iRow * iCols + iCol)] = m_vecCells[static_cast<size_t>(iRow * m_iCols + iCol)];
                }
            }

            mapNewRegions.emplace(0, sDefaultRegion);

            for (const auto& pairRegion : m_mapRegions)
            {
                Region_s sCurrentRegion;

                if (pairRegion.first != 0)
                {
                    sCurrentRegion = pairRegion.second;
                    sCurrentRegion.iX = ClampInt(sCurrentRegion.iX, 0, iCols - 1);
                    sCurrentRegion.iY = ClampInt(sCurrentRegion.iY, 0, iRows - 1);
                    sCurrentRegion.iWidth = ClampInt(sCurrentRegion.iWidth, 1, iCols - sCurrentRegion.iX);
                    sCurrentRegion.iHeight = ClampInt(sCurrentRegion.iHeight, 1, iRows - sCurrentRegion.iY);
                    sCurrentRegion.iCursorX = ClampInt(sCurrentRegion.iCursorX, 0, sCurrentRegion.iWidth - 1);
                    sCurrentRegion.iCursorY = ClampInt(sCurrentRegion.iCursorY, 0, sCurrentRegion.iHeight - 1);
                    sCurrentRegion.sCursorSaved.iX = ClampInt(sCurrentRegion.sCursorSaved.iX, 0, sCurrentRegion.iWidth - 1);
                    sCurrentRegion.sCursorSaved.iY = ClampInt(sCurrentRegion.sCursorSaved.iY, 0, sCurrentRegion.iHeight - 1);
                    mapNewRegions.emplace(sCurrentRegion.iId, sCurrentRegion);
                }
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

        m_iCols = iCols;
        m_iRows = iRows;
        m_vecCells = std::move(vecNewCells);
        m_mapRegions = std::move(mapNewRegions);
        return S_OK;
    }

    VOID Buffer::ClearRegion(_In_opt_ RegionHandle hRegion) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        for (INT iY = 0; iY < hRegion->iHeight; ++iY)
        {
            for (INT iX = 0; iX < hRegion->iWidth; ++iX)
            {
                FillCell(hRegion->iX + iX, hRegion->iY + iY, m_sAttributesDefault);
            }
        }

        hRegion->iCursorX = 0;
        hRegion->iCursorY = 0;
        hRegion->sCursorSaved = CursorState{ 0, 0 };
        hRegion->sAttributesCurrent = m_sAttributesDefault;
        hRegion->bWrapPending = FALSE;
    }

    VOID Buffer::FillArea(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                          _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                          _In_ DWORD dwStyleFlags) noexcept
    {
        INT iStartX;
        INT iStartY;
        INT iEndX;
        INT iEndY;
        INT iFillX;
        INT iFillY;
        Attributes attributesCell;

        if ((iWidth <= 0) || (iHeight <= 0))
        {
            return;
        }
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        iStartX = ClampInt(iX, 0, hRegion->iWidth - 1);
        iStartY = ClampInt(iY, 0, hRegion->iHeight - 1);
        iEndX = ClampInt(iX + iWidth - 1, 0, hRegion->iWidth - 1);
        iEndY = ClampInt(iY + iHeight - 1, 0, hRegion->iHeight - 1);
        attributesCell.crForeground = MakeColor(GetRValue(crForeground), GetGValue(crForeground), GetBValue(crForeground));
        attributesCell.crBackground = MakeColor(GetRValue(crBackground), GetGValue(crBackground), GetBValue(crBackground));
        attributesCell.dwStyleFlags = dwStyleFlags;

        for (iFillY = iStartY; iFillY <= iEndY; ++iFillY)
        {
            for (iFillX = iStartX; iFillX <= iEndX; ++iFillX)
            {
                SetCell(hRegion->iX + iFillX, hRegion->iY + iFillY, chCodepointW, attributesCell);
            }
        }
    }

    VOID Buffer::ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
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

    VOID Buffer::ResetAttributes(_In_opt_ RegionHandle hRegion) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }
        hRegion->sAttributesCurrent = m_sAttributesDefault;
    }

    VOID Buffer::PutCodepoint(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept
    {
        INT iAbsoluteX;
        INT iAbsoluteY;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        if (hRegion->bWrapPending != FALSE)
        {
            hRegion->iCursorX = 0;
            hRegion->iCursorY += 1;
            if (hRegion->iCursorY >= hRegion->iHeight)
            {
                ScrollRegionUp(hRegion, 1);
                hRegion->iCursorY = hRegion->iHeight - 1;
            }
            hRegion->bWrapPending = FALSE;
        }

        iAbsoluteX = hRegion->iX + hRegion->iCursorX;
        iAbsoluteY = hRegion->iY + hRegion->iCursorY;
        SetCell(iAbsoluteX, iAbsoluteY, chCodepointW, hRegion->sAttributesCurrent);
        AdvanceCursorAfterWrite(hRegion);
    }

    VOID Buffer::ProcessControl(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        switch (chCodepointW)
        {
            case L'\r':
                hRegion->iCursorX = 0;
                hRegion->bWrapPending = FALSE;
                break;

            case L'\n':
                hRegion->iCursorY += 1;
                if (hRegion->iCursorY >= hRegion->iHeight)
                {
                    ScrollRegionUp(hRegion, 1);
                    hRegion->iCursorY = hRegion->iHeight - 1;
                }
                hRegion->bWrapPending = FALSE;
                break;

            case L'\b':
                hRegion->iCursorX = (std::max)(0, hRegion->iCursorX - 1);
                hRegion->bWrapPending = FALSE;
                break;

            case L'\t':
                AdvanceToNextTabStop(hRegion);
                break;

            case L'\f':
                ClearRegion(hRegion);
                break;
        }
    }

    VOID Buffer::MoveCursorRelative(_In_opt_ RegionHandle hRegion, _In_ INT iDeltaX, _In_ INT iDeltaY) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        hRegion->iCursorX = ClampInt(hRegion->iCursorX + iDeltaX, 0, hRegion->iWidth - 1);
        hRegion->iCursorY = ClampInt(hRegion->iCursorY + iDeltaY, 0, hRegion->iHeight - 1);
        hRegion->bWrapPending = FALSE;
    }

    VOID Buffer::SetCursorPosition(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased, _In_ INT iColOneBased) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        hRegion->iCursorY = ClampInt(iRowOneBased - 1, 0, hRegion->iHeight - 1);
        hRegion->iCursorX = ClampInt(iColOneBased - 1, 0, hRegion->iWidth - 1);
        hRegion->bWrapPending = FALSE;
    }

    VOID Buffer::SetCursorColumn(_In_opt_ RegionHandle hRegion, _In_ INT iColOneBased) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        SetCursorPosition(hRegion, hRegion->iCursorY + 1, iColOneBased);
    }

    VOID Buffer::SetCursorRow(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        SetCursorPosition(hRegion, iRowOneBased, hRegion->iCursorX + 1);
    }

    VOID Buffer::EraseInLine(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept
    {
        INT iStartX;
        INT iEndX;
        INT iX;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        iStartX = 0;
        iEndX = hRegion->iWidth - 1;
        if (iMode == 0)
        {
            iStartX = hRegion->iCursorX;
        }
        else if (iMode == 1)
        {
            iEndX = hRegion->iCursorX;
        }
        for (iX = iStartX; iX <= iEndX; ++iX)
        {
            FillCell(hRegion->iX + iX, hRegion->iY + hRegion->iCursorY, m_sAttributesDefault);
        }
    }

    VOID Buffer::EraseInDisplay(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept
    {
        INT iYStart;
        INT iYEnd;
        INT iY;
        INT iXStart;
        INT iXEnd;
        INT iX;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        if (iMode == 2)
        {
            ClearRegion(hRegion);
            return;
        }

        iYStart = 0;
        iYEnd = hRegion->iHeight - 1;
        if (iMode == 0)
        {
            iYStart = hRegion->iCursorY;
        }
        else if (iMode == 1)
        {
            iYEnd = hRegion->iCursorY;
        }

        for (iY = iYStart; iY <= iYEnd; ++iY)
        {
            iXStart = 0;
            iXEnd = hRegion->iWidth - 1;
            if ((iMode == 0) && (iY == hRegion->iCursorY))
            {
                iXStart = hRegion->iCursorX;
            }
            if ((iMode == 1) && (iY == hRegion->iCursorY))
            {
                iXEnd = hRegion->iCursorX;
            }
            for (iX = iXStart; iX <= iXEnd; ++iX)
            {
                FillCell(hRegion->iX + iX, hRegion->iY + iY, m_sAttributesDefault);
            }
        }
    }

    VOID Buffer::SetGraphicsRendition(_In_opt_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams,
                                      _In_ SIZE_T uParamsCount) noexcept
    {
        SIZE_T uIndex;
        INT iValue;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        if (uParamsCount == 0)
        {
            hRegion->sAttributesCurrent = m_sAttributesDefault;
            return;
        }

        for (uIndex = 0; uIndex < uParamsCount; ++uIndex)
        {
            iValue = lpiParams[uIndex];
            if (iValue == 0)
            {
                hRegion->sAttributesCurrent = m_sAttributesDefault;
            }
            else if (iValue == 1)
            {
                hRegion->sAttributesCurrent.dwStyleFlags |= Control::StyleBold;
            }
            else if (iValue == 3)
            {
                hRegion->sAttributesCurrent.dwStyleFlags |= Control::StyleItalic;
            }
            else if (iValue == 4)
            {
                hRegion->sAttributesCurrent.dwStyleFlags |= Control::StyleUnderline;
            }
            else if (iValue == 5)
            {
                hRegion->sAttributesCurrent.dwStyleFlags |= Control::StyleBlink;
            }
            else if (iValue == 7)
            {
                hRegion->sAttributesCurrent.dwStyleFlags |= Control::StyleInverse;
            }
            else if (iValue == 22)
            {
                hRegion->sAttributesCurrent.dwStyleFlags &= ~Control::StyleBold;
            }
            else if (iValue == 23)
            {
                hRegion->sAttributesCurrent.dwStyleFlags &= ~Control::StyleItalic;
            }
            else if (iValue == 24)
            {
                hRegion->sAttributesCurrent.dwStyleFlags &= ~Control::StyleUnderline;
            }
            else if (iValue == 25)
            {
                hRegion->sAttributesCurrent.dwStyleFlags &= ~Control::StyleBlink;
            }
            else if (iValue == 27)
            {
                hRegion->sAttributesCurrent.dwStyleFlags &= ~Control::StyleInverse;
            }
            else if ((iValue >= 30) && (iValue <= 37))
            {
                hRegion->sAttributesCurrent.crForeground = GetAnsi16Color(iValue - 30);
            }
            else if ((iValue >= 40) && (iValue <= 47))
            {
                hRegion->sAttributesCurrent.crBackground = GetAnsi16Color(iValue - 40);
            }
            else if ((iValue >= 90) && (iValue <= 97))
            {
                hRegion->sAttributesCurrent.crForeground = GetAnsi16Color((iValue - 90) + 8);
            }
            else if ((iValue >= 100) && (iValue <= 107))
            {
                hRegion->sAttributesCurrent.crBackground = GetAnsi16Color((iValue - 100) + 8);
            }
            else if (iValue == 39)
            {
                hRegion->sAttributesCurrent.crForeground = m_sAttributesDefault.crForeground;
            }
            else if (iValue == 49)
            {
                hRegion->sAttributesCurrent.crBackground = m_sAttributesDefault.crBackground;
            }
            else if ((iValue == 38) || (iValue == 48))
            {
                ApplySgrColor(hRegion, lpiParams, uParamsCount, &uIndex, (iValue == 38) ? TRUE : FALSE);
            }
        }
    }

    HRESULT Buffer::CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion) noexcept
    {
        Region_s regionCurrent;
        HRESULT hr;

        if (!lphRegion)
        {
            return E_POINTER;
        }
        *lphRegion = nullptr;

        hr = ValidateRegionBounds(iX, iY, iWidth, iHeight);
        if (FAILED(hr))
        {
            return hr;
        }

        regionCurrent = Region_s{};
        regionCurrent.iId = m_iNextRegionId;
        regionCurrent.iX = iX;
        regionCurrent.iY = iY;
        regionCurrent.iWidth = iWidth;
        regionCurrent.iHeight = iHeight;
        regionCurrent.sAttributesCurrent = m_sAttributesDefault;
        try
        {
            m_mapRegions.emplace(regionCurrent.iId, regionCurrent);
            *lphRegion = &m_mapRegions.find(regionCurrent.iId)->second;
            m_iNextRegionId += 1;
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

    HRESULT Buffer::DestroyRegion(_In_ RegionHandle hRegion) noexcept
    {
        if ((!hRegion) || hRegion->iId == m_mapRegions.find(0)->second.iId)
        {
            return E_INVALIDARG;
        }

        try
        {
            if (m_mapRegions.erase(hRegion->iId) == 0U)
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
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

    HRESULT Buffer::RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept
    {
        if ((!hRegion) || hRegion->iId == m_mapRegions.find(0)->second.iId)
        {
            return E_INVALIDARG;
        }
        hRegion->iX = ClampInt(iX, 0, m_iCols - 1);
        hRegion->iY = ClampInt(iY, 0, m_iRows - 1);
        hRegion->iWidth = ClampInt(iWidth, 1, m_iCols - hRegion->iX);
        hRegion->iHeight = ClampInt(iHeight, 1, m_iRows - hRegion->iY);
        hRegion->iCursorX = ClampInt(hRegion->iCursorX, 0, hRegion->iWidth - 1);
        hRegion->iCursorY = ClampInt(hRegion->iCursorY, 0, hRegion->iHeight - 1);
        hRegion->sCursorSaved.iX = ClampInt(hRegion->sCursorSaved.iX, 0, hRegion->iWidth - 1);
        hRegion->sCursorSaved.iY = ClampInt(hRegion->sCursorSaved.iY, 0, hRegion->iHeight - 1);
        return S_OK;
    }

    VOID Buffer::GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                                   _Out_opt_ LPINT lpiHeight) const noexcept
    {
        if (hRegion && hRegion->iId != m_mapRegions.find(0)->second.iId)
        {
            if (lpiX)
            {
                *lpiX = hRegion->iX;
            }
            if (lpiY)
            {
                *lpiY = hRegion->iY;
            }
            if (lpiWidth)
            {
                *lpiWidth = hRegion->iWidth;
            }
            if (lpiHeight)
            {
                *lpiHeight = hRegion->iHeight;
            }
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
                *lpiWidth = m_iCols;
            }
            if (lpiHeight)
            {
                *lpiHeight = m_iRows;
            }
        }
    }

    VOID Buffer::SaveCursor(_In_opt_ RegionHandle hRegion) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        hRegion->sCursorSaved.iX = hRegion->iCursorX;
        hRegion->sCursorSaved.iY = hRegion->iCursorY;
    }

    VOID Buffer::RestoreCursor(_In_opt_ RegionHandle hRegion) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        hRegion->iCursorX = ClampInt(hRegion->sCursorSaved.iX, 0, hRegion->iWidth - 1);
        hRegion->iCursorY = ClampInt(hRegion->sCursorSaved.iY, 0, hRegion->iHeight - 1);
        hRegion->bWrapPending = FALSE;
    }

    VOID Buffer::ToggleBlinkVisibility() noexcept
    {
        m_bBlinkVisible = (m_bBlinkVisible == FALSE) ? TRUE : FALSE;
    }

    VOID Buffer::SetBlinkVisible(_In_ BOOL bBlinkVisible) noexcept
    {
        m_bBlinkVisible = (bBlinkVisible != FALSE) ? TRUE : FALSE;
    }

    HRESULT Buffer::GetSnapshot(_Out_ Snapshot* lpSnapshot) const noexcept
    {
        if (!lpSnapshot)
        {
            return E_POINTER;
        }

        lpSnapshot->lpCells = m_vecCells.data();
        lpSnapshot->iCols = m_iCols;
        lpSnapshot->iRows = m_iRows;
        lpSnapshot->bBlinkVisible = m_bBlinkVisible;
        lpSnapshot->crDefaultForeground = m_sAttributesDefault.crForeground;
        lpSnapshot->crDefaultBackground = m_sAttributesDefault.crBackground;
        return S_OK;
    }

    HRESULT Buffer::InitializeCells() noexcept
    {
        size_t uCellsCount;
        Cell cellBlank;

        if ((m_iCols <= 0) || (m_iRows <= 0))
        {
            return E_INVALIDARG;
        }

        uCellsCount = static_cast<size_t>(m_iCols) * static_cast<size_t>(m_iRows);
        cellBlank = Cell{};
        cellBlank.chCodepointW = L' ';
        cellBlank.crForeground = m_sAttributesDefault.crForeground;
        cellBlank.crBackground = m_sAttributesDefault.crBackground;
        cellBlank.dwStyleFlags = Control::StyleNone;
        cellBlank.bIsDirty = TRUE;
        try
        {
            m_vecCells.assign(uCellsCount, cellBlank);
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

    HRESULT Buffer::ValidateRegionBounds(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) const noexcept
    {
        if (iX < 0 || iY < 0 || iWidth <= 0 || iHeight <= 0)
        {
            return E_INVALIDARG;
        }
        if (iX + iWidth > m_iCols || iY + iHeight > m_iRows)
        {
            return E_INVALIDARG;
        }
        return S_OK;
    }

    VOID Buffer::SetCell(_In_ INT iX, _In_ INT iY, _In_ WCHAR chCodepointW, _In_ const Attributes& attributesCell) noexcept
    {
        SIZE_T iIndex;

        if (GetCellIndex(iX, iY, &iIndex) == FALSE)
        {
            return;
        }

        try
        {
            m_vecCells[iIndex].chCodepointW = chCodepointW;
            m_vecCells[iIndex].crForeground = attributesCell.crForeground;
            m_vecCells[iIndex].crBackground = attributesCell.crBackground;
            m_vecCells[iIndex].dwStyleFlags = attributesCell.dwStyleFlags;
            m_vecCells[iIndex].bIsDirty = TRUE;
        }
        catch (...)
        {
        }
    }

    VOID Buffer::FillCell(_In_ INT iX, _In_ INT iY, _In_ const Attributes& attributesCell) noexcept
    {
        SetCell(iX, iY, L' ', attributesCell);
    }

    VOID Buffer::FillRange(_In_ INT iXStart, _In_ INT iYStart, _In_ INT iXEnd, _In_ INT iYEnd,
                           _In_ const Attributes& attributesCell) noexcept
    {
        for (INT iY = iYStart; iY <= iYEnd; ++iY)
        {
            for (INT iX = iXStart; iX <= iXEnd; ++iX)
            {
                FillCell(iX, iY, attributesCell);
            }
        }
    }

    VOID Buffer::ScrollRegionUp(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
    {
        INT iY;
        INT iX;
        SIZE_T uSourceIndex;
        SIZE_T uTargetIndex;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        if (iLineCount <= 0)
        {
            return;
        }
        if (iLineCount >= hRegion->iHeight)
        {
            ClearRegion(hRegion);
            return;
        }

        for (iY = 0; iY < hRegion->iHeight - iLineCount; ++iY)
        {
            for (iX = 0; iX < hRegion->iWidth; ++iX)
            {
                if (GetCellIndex(hRegion->iX + iX, hRegion->iY + iY + iLineCount, &uSourceIndex) != FALSE &&
                    GetCellIndex(hRegion->iX + iX, hRegion->iY + iY, &uTargetIndex) != FALSE)
                {
                    m_vecCells[uTargetIndex] = m_vecCells[uSourceIndex];
                    m_vecCells[uTargetIndex].bIsDirty = TRUE;
                }
            }
        }
        FillRange(hRegion->iX, hRegion->iY + hRegion->iHeight - iLineCount, hRegion->iX + hRegion->iWidth - 1,
                  hRegion->iY + hRegion->iHeight - 1, m_sAttributesDefault);
    }

    VOID Buffer::ScrollRegionDown(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept
    {
        INT iY;
        INT iX;
        SIZE_T uSourceIndex;
        SIZE_T uTargetIndex;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        if (iLineCount <= 0)
        {
            return;
        }
        if (iLineCount >= hRegion->iHeight)
        {
            ClearRegion(hRegion);
            return;
        }

        for (iY = hRegion->iHeight - 1; iY >= iLineCount; --iY)
        {
            for (iX = 0; iX < hRegion->iWidth; ++iX)
            {
                if (GetCellIndex(hRegion->iX + iX, hRegion->iY + iY - iLineCount, &uSourceIndex) != FALSE &&
                    GetCellIndex(hRegion->iX + iX, hRegion->iY + iY, &uTargetIndex) != FALSE)
                {
                    m_vecCells[uTargetIndex] = m_vecCells[uSourceIndex];
                    m_vecCells[uTargetIndex].bIsDirty = TRUE;
                }
            }
        }
        FillRange(hRegion->iX, hRegion->iY, hRegion->iX + hRegion->iWidth - 1, hRegion->iY + iLineCount - 1, m_sAttributesDefault);
    }

    VOID Buffer::AdvanceCursorAfterWrite(_In_opt_ RegionHandle hRegion) noexcept
    {
        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        if (hRegion->iCursorX == hRegion->iWidth - 1)
        {
            hRegion->bWrapPending = TRUE;
        }
        else
        {
            hRegion->iCursorX += 1;
        }
    }

    BOOL Buffer::GetCellIndex(_In_ INT iX, _In_ INT iY, _Out_ PSIZE_T lpuIndex) const noexcept
    {
        if (IsWithinBounds(iX, 0, m_iCols) == FALSE || IsWithinBounds(iY, 0, m_iRows) == FALSE)
        {
            *lpuIndex = 0;
            return FALSE;
        }
        *lpuIndex = static_cast<SIZE_T>(iY * m_iCols + iX);
        return TRUE;
    }

    VOID Buffer::ApplySgrColor(_In_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams, _In_ SIZE_T uParamsCount,
                               _Inout_ PSIZE_T lpuIndex, _In_ BOOL bForeground) noexcept
    {
        SIZE_T uIndex;
        INT iMode;
        COLORREF crColor;

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
            hRegion->sAttributesCurrent.crForeground = crColor;
        }
        else
        {
            hRegion->sAttributesCurrent.crBackground = crColor;
        }
    }

    VOID Buffer::AdvanceToNextTabStop(_In_opt_ RegionHandle hRegion) noexcept
    {
        INT iAbsoluteX;
        INT iNextStop;
        INT iStop;

        if (!hRegion)
        {
            hRegion = &m_mapRegions.find(0)->second;
        }

        iAbsoluteX = hRegion->iX + hRegion->iCursorX;
        iNextStop = hRegion->iX + hRegion->iWidth - 1;
        for (iStop = 0; iStop < m_iCols; iStop += 8)
        {
            if (iStop > iAbsoluteX)
            {
                iNextStop = (std::min)(iStop, hRegion->iX + hRegion->iWidth - 1);
                break;
            }
        }
        hRegion->iCursorX = iNextStop - hRegion->iX;
        hRegion->bWrapPending = FALSE;
    }
}

// -----------------------------------------------------------------------------

static COLORREF MakeColor(_In_ BYTE byRed, _In_ BYTE byGreen, _In_ BYTE byBlue) noexcept
{
    return RGB(byRed, byGreen, byBlue);
}

static GuiTerminal::Internals::Attributes MakeAttributes(_In_ COLORREF crForeground, _In_ COLORREF crBackground,
                                                         _In_ DWORD dwStyleFlags) noexcept
{
    GuiTerminal::Internals::Attributes attributesCell;

    attributesCell = GuiTerminal::Internals::Attributes{};
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
    static const std::array<COLORREF, 16U> s_vecPalette =
    {
        MakeColor(12U, 12U, 12U),
        MakeColor(197U, 15U, 31U),
        MakeColor(19U, 161U, 14U),
        MakeColor(193U, 156U, 0U),
        MakeColor(0U, 55U, 218U),
        MakeColor(136U, 23U, 152U),
        MakeColor(58U, 150U, 221U),
        MakeColor(204U, 204U, 204U),
        MakeColor(118U, 118U, 118U),
        MakeColor(231U, 72U, 86U),
        MakeColor(22U, 198U, 12U),
        MakeColor(249U, 241U, 165U),
        MakeColor(59U, 120U, 255U),
        MakeColor(180U, 0U, 158U),
        MakeColor(97U, 214U, 214U),
        MakeColor(242U, 242U, 242U)
    };
    INT iPaletteIndex;

    iPaletteIndex = ClampInt(iIndex, 0, 15);
    return s_vecPalette[static_cast<size_t>(iPaletteIndex)];
}

static COLORREF GetXterm256Color(_In_ INT iIndex) noexcept
{
    static const std::array<INT, 6U> s_vecLevels = { 0, 95, 135, 175, 215, 255 };
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
        return MakeColor(static_cast<BYTE>(s_vecLevels[static_cast<size_t>(iRed)]),
                         static_cast<BYTE>(s_vecLevels[static_cast<size_t>(iGreen)]),
                         static_cast<BYTE>(s_vecLevels[static_cast<size_t>(iBlue)]));
    }
    iGray = 8 + ((iColorIndex - 232) * 10);
    return MakeColor(static_cast<BYTE>(iGray), static_cast<BYTE>(iGray), static_cast<BYTE>(iGray));
}
