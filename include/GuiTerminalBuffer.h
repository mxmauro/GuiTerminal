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
        };
    }

    typedef struct Internals::Region_s *RegionHandle;

    namespace Internals
    {
        class Buffer
        {
        public:
            struct Cell
            {
                WCHAR chCodepointW{};
                COLORREF crForeground{};
                COLORREF crBackground{};
                DWORD dwStyleFlags{};
                BOOL bIsDirty{};
            };

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
            VOID GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                                   _Out_opt_ LPINT lpiHeight) const noexcept;

            VOID SaveCursor(_In_opt_ RegionHandle hRegion) noexcept;
            VOID RestoreCursor(_In_opt_ RegionHandle hRegion) noexcept;

            VOID ToggleBlinkVisibility() noexcept;
            VOID SetBlinkVisible(_In_ BOOL bBlinkVisible) noexcept;

            HRESULT GetSnapshot(_Out_ Snapshot* lpSnapshot) const noexcept;

        private:
            HRESULT InitializeCells() noexcept;
            HRESULT ValidateRegionBounds(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) const noexcept;

            VOID SetCell(_In_ INT iX, _In_ INT iY, _In_ WCHAR chCodepointW, _In_ const Attributes& sAttributesCell) noexcept;
            VOID FillCell(_In_ INT iX, _In_ INT iY, _In_ const Attributes& sAttributesCell) noexcept;
            VOID FillRange(_In_ INT iXStart, _In_ INT iYStart, _In_ INT iXEnd, _In_ INT iYEnd,
                           _In_ const Attributes& sAttributesCell) noexcept;

            VOID ScrollRegionUp(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;
            VOID ScrollRegionDown(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

            VOID AdvanceCursorAfterWrite(_In_opt_ RegionHandle hRegion) noexcept;

            BOOL GetCellIndex(_In_ INT iX, _In_ INT iY, _Out_ PSIZE_T lpuIndex) const noexcept;

            VOID ApplySgrColor(_In_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams, _In_ SIZE_T uParamsCount,
                               _Inout_ PSIZE_T lpuIndex, _In_ BOOL bForeground) noexcept;
            VOID AdvanceToNextTabStop(_In_opt_ RegionHandle hRegion) noexcept;

        private:
            INT m_iCols{};
            INT m_iRows{};
            std::vector<Cell> m_vecCells;
            Attributes m_sAttributesDefault{};
            std::unordered_map<INT, Region_s> m_mapRegions;
            INT m_iNextRegionId{ 1 };
            BOOL m_bBlinkVisible{ TRUE };
        };
    }
}
