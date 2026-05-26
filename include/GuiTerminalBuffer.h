#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    namespace Internals
    {
        struct Cell
        {
            WCHAR chCodepointW{};
            COLORREF crForeground{};
            COLORREF crBackground{};
            DWORD dwStyleFlags{};
            BOOL bIsDirty{};
        };

        struct CursorState
        {
            INT iX{};
            INT iY{};
        };

        struct Attributes
        {
            COLORREF crForeground{};
            COLORREF crBackground{};
            DWORD dwStyleFlags{};
        };

        struct Region_s
        {
            INT iId{};
            INT iX{};
            INT iY{};
            INT iWidth{};
            INT iHeight{};
            INT iCursorX{};
            INT iCursorY{};
            CursorState sCursorSaved;
            BOOL bWrapPending{ FALSE };
            Attributes sAttributesCurrent{};
            std::vector<Cell> vecCells;
        };
    }

    typedef struct Internals::Region_s *RegionHandle;

    namespace Internals
    {
        class Buffer
        {
        public:
            using Cell = Internals::Cell;

            struct Snapshot
            {
                const Cell *lpCells{};
                INT iCols{};
                INT iRows{};
                BOOL bBlinkVisible{};
                COLORREF crDefaultForeground{};
                COLORREF crDefaultBackground{};
            };

        public:
            Buffer() noexcept = default;
            Buffer(const Buffer&) = delete;
            Buffer(Buffer&&) = delete;
            ~Buffer() noexcept = default;

            Buffer& operator=(const Buffer&) = delete;
            Buffer& operator=(Buffer&&) = delete;

            HRESULT Initialize(_In_ INT iCols, _In_ INT iRows, _In_ COLORREF crDefaultForeground,
                               _In_ COLORREF crDefaultBackground) noexcept;
            HRESULT Resize(_In_ INT iCols, _In_ INT iRows) noexcept;

            VOID ClearRegion(_In_opt_ RegionHandle hRegion) noexcept;

            VOID FillArea(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                          _In_ WCHAR chCodepointW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                          _In_ DWORD dwStyleFlags) noexcept;
            VOID DrawHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ DWORD dwStrokeType,
                                    _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
            VOID DrawVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ DWORD dwStrokeType,
                                  _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
            VOID DrawBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                         _In_ DWORD dwBoxSideFlags, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
                         _In_ DWORD dwStyleFlags) noexcept;

            VOID ScrollRegion(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

            VOID ResetAttributes(_In_opt_ RegionHandle hRegion) noexcept;

            VOID PutCodepoint(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept;
            VOID ProcessControl(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept;

            VOID MoveCursorRelative(_In_opt_ RegionHandle hRegion, _In_ INT iDeltaX, _In_ INT iDeltaY) noexcept;
            VOID SetCursorPosition(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased, _In_ INT iColOneBased) noexcept;
            VOID SetCursorColumn(_In_opt_ RegionHandle hRegion, _In_ INT iColOneBased) noexcept;
            VOID SetCursorRow(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased) noexcept;

            VOID EraseInLine(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept;
            VOID EraseInDisplay(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept;

            VOID SetGraphicsRendition(_In_opt_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams,
                                      _In_ SIZE_T uParamsCount) noexcept;

            HRESULT CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle* lphRegion) noexcept;
            HRESULT DestroyRegion(_In_ RegionHandle hRegion) noexcept;

            HRESULT RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept;
            HRESULT BringRegionToFront(_In_ RegionHandle hRegion) noexcept;
            HRESULT SendRegionToBack(_In_ RegionHandle hRegion) noexcept;
            VOID GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                                   _Out_opt_ LPINT lpiHeight) const noexcept;
            BOOL ConvertToRegionCoordinates(_In_opt_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal,
                                            _Out_opt_ LPINT lpiColRegion, _Out_opt_ LPINT lpiRowRegion) const noexcept;
            BOOL ConvertFromRegionCoordinates(_In_opt_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion,
                                              _Out_opt_ LPINT lpiColTerminal, _Out_opt_ LPINT lpiRowTerminal) const noexcept;

            VOID SaveCursor(_In_opt_ RegionHandle hRegion) noexcept;
            VOID RestoreCursor(_In_opt_ RegionHandle hRegion) noexcept;

            VOID ToggleBlinkVisibility() noexcept;
            VOID SetBlinkVisible(_In_ BOOL bBlinkVisible) noexcept;

            HRESULT GetSnapshot(_Out_ Snapshot* lpSnapshot) const noexcept;

        private:
            HRESULT InitializeRootRegion() noexcept;
            HRESULT ValidateRegionBounds(_In_ INT iWidth, _In_ INT iHeight) const noexcept;
            Region_s* ResolveRegion(_In_opt_ RegionHandle hRegion) noexcept;
            const Region_s* ResolveRegion(_In_opt_ RegionHandle hRegion) const noexcept;

            Cell MakeBlankCell() const noexcept;
            HRESULT InitializeRegionCells(_Inout_ Region_s& sRegion) const noexcept;
            HRESULT ResizeRegionCells(_In_ const Region_s& sRegionSource, _Out_ std::vector<Cell>& vecCellsTarget,
                                      _In_ INT iWidthTarget, _In_ INT iHeightTarget) const noexcept;
            VOID ClearRegionCells(_Inout_ Region_s& sRegion) const noexcept;
            VOID SetCell(_Inout_ Region_s& sRegion, _In_ INT iX, _In_ INT iY, _In_ WCHAR chCodepointW,
                         _In_ const Attributes& sAttributesCell) noexcept;
            VOID FillCell(_Inout_ Region_s& sRegion, _In_ INT iX, _In_ INT iY, _In_ const Attributes& sAttributesCell) noexcept;
            VOID FillRange(_Inout_ Region_s& sRegion, _In_ INT iXStart, _In_ INT iYStart, _In_ INT iXEnd, _In_ INT iYEnd,
                           _In_ const Attributes& sAttributesCell) noexcept;
            const Cell* GetCell(_In_ const Region_s& sRegion, _In_ INT iX, _In_ INT iY) const noexcept;
            VOID DrawStrokeCell(_Inout_ Region_s& sRegion, _In_ INT iX, _In_ INT iY, _In_ DWORD dwStrokeType, _In_ BYTE byUp,
                                _In_ BYTE byRight, _In_ BYTE byDown, _In_ BYTE byLeft, _In_ const Attributes& sAttributesCell) noexcept;
            BOOL ClipRectangle(_In_ const Region_s& sRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight,
                               _Out_ LPINT lpiStartX, _Out_ LPINT lpiStartY, _Out_ LPINT lpiEndX,
                               _Out_ LPINT lpiEndY) const noexcept;

            VOID ScrollRegionUp(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;
            VOID ScrollRegionDown(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

            VOID AdvanceCursorAfterWrite(_In_opt_ RegionHandle hRegion) noexcept;

            BOOL GetCellIndex(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _Out_ PSIZE_T lpuIndex) const noexcept;
            VOID ComposeRegion(_In_ const Region_s& sRegion) const noexcept;

            VOID ApplySgrColor(_In_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams, _In_ SIZE_T uParamsCount,
                               _Inout_ PSIZE_T lpuIndex, _In_ BOOL bForeground) noexcept;
            VOID AdvanceToNextTabStop(_In_opt_ RegionHandle hRegion) noexcept;

        private:
            INT m_iCols{};
            INT m_iRows{};
            mutable std::vector<Cell> m_vecSnapshotCells;
            Attributes m_sAttributesDefault{};
            std::unordered_map<INT, Region_s> m_mapRegions;
            std::vector<INT> m_vecRegionOrder;
            INT m_iNextRegionId{ 1 };
            BOOL m_bBlinkVisible{ TRUE };
        };
    }
}
