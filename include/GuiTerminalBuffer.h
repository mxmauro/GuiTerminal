#pragma once

#define WIN32_LEAN_AND_MEAN
#include "GuiTerminalDraw.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <windows.h>

// -----------------------------------------------------------------------------

namespace GuiTerminal {

namespace Internals {

/** @brief Stores the rendered character and attributes for one terminal cell. */
typedef struct Cell_s {
    WCHAR chCodepointW{};
    COLORREF crForeground{};
    COLORREF crBackground{};
    DWORD dwStyleFlags{};
} Cell_t;

/** @brief Stores a zero-based cursor position. */
typedef struct CursorState_s {
    INT iX{};
    INT iY{};
} CursorState_t;

/** @brief Stores foreground, background, and style attributes for a cell. */
typedef struct Attributes_s {
    COLORREF crForeground{};
    COLORREF crBackground{};
    DWORD dwStyleFlags{};
} Attributes_t;

/** @brief Describes a zero-based cell rectangle. */
typedef struct CellRect_s {
    INT iX{};
    INT iY{};
    INT iWidth{};
    INT iHeight{};
} CellRect_t;

/** @brief Stores the model state for a normal or custom-draw region. */
typedef struct Region_s {
    INT iId{};
    Region_t *lpParent{nullptr};
    INT iX{};
    INT iY{};
    INT iWidth{};
    INT iHeight{};
    INT iCursorX{};
    INT iCursorY{};
    CursorState_t sCursorSaved;
    BOOL bWrapPending{FALSE};
    BOOL bVisible{TRUE};
    PVOID lpContext{nullptr};
    BOOL bCustomDraw{FALSE};
    std::function<VOID(_In_ DrawContext &drawContext, _In_ RegionHandle hRegion)> fnCustomDrawCallback;
    PVOID lpCAdapterDrawState{nullptr};
    std::function<VOID(_In_ CustomDrawResourceCleanupReason cleanupReason)> fnResourceCleanupCallback;
    std::function<VOID(_In_ RegionHandle hRegion)> fnDestroyCallback;
    Attributes_t sAttributesCurrent{};
    std::vector<Cell_t> vecCells;
    std::vector<INT> vecChildRegionIds;
} Region_t;

} // namespace Internals

namespace Internals {

/**
 * @brief Owns terminal cells, region state, and composed snapshots.
 * @remarks This is a lower-level library contract. Applications normally use Control instead.
 */
class Buffer
{
  public:
    /** @brief Names the cell structure stored by the buffer. */
    using Cell = Internals::Cell_t;

    /** @brief Exposes the root-terminal snapshot consumed by Renderer. */
    typedef struct Snapshot_s {
        const Cell *lpCells{};
        INT iCols{};
        INT iRows{};
        BOOL bBlinkVisible{};
        BOOL bCursorVisible{};
        INT iCursorCol{};
        INT iCursorRow{};
        DWORD dwCursorStyle{};
        COLORREF crDefaultForeground{};
        COLORREF crDefaultBackground{};
    } Snapshot;

    /** @brief Describes one visible custom or normal region in render order. */
    typedef struct RenderItem_s {
        const Region_t *lpsRegion{};
        CellRect_t sVisibleRect{};
        LONGLONG llOriginX{};
        LONGLONG llOriginY{};
    } RenderItem;

  public:
    /** @brief Constructs an uninitialized buffer. */
    Buffer() noexcept = default;
    /** @brief Prevents copying buffer-owned region state. */
    Buffer(const Buffer &) = delete;
    /** @brief Prevents moving buffer-owned region state. */
    Buffer(Buffer &&) = delete;
    /** @brief Destroys the buffer and its regions. */
    ~Buffer() noexcept = default;

    /** @brief Prevents copy assignment. */
    Buffer &operator=(const Buffer &) = delete;
    /** @brief Prevents move assignment. */
    Buffer &operator=(Buffer &&) = delete;

    /** @brief Initializes the root grid and its default attributes. */
    HRESULT Initialize(_In_ INT iCols, _In_ INT iRows, _In_ COLORREF crDefaultForeground, _In_ COLORREF crDefaultBackground) noexcept;
    /** @brief Resizes the root grid while preserving overlapping cells. */
    HRESULT Resize(_In_ INT iCols, _In_ INT iRows) noexcept;

    /** @brief Clears a selected region, or the root region when the handle is null. */
    VOID Clear(_In_opt_ RegionHandle hRegion) noexcept;

    /** @brief Scrolls a selected region by a number of cell rows. */
    VOID Scroll(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;
    /** @brief Moves a cell rectangle within a region and fills the vacated cells. */
    VOID Move(_In_opt_ RegionHandle hRegion, _In_ INT iSourceX, _In_ INT iSourceY, _In_ INT iWidth, _In_ INT iHeight, _In_ INT iTargetX,
              _In_ INT iTargetY, _In_ WCHAR chFillW, _In_ COLORREF crForeground, _In_ COLORREF crBackground,
              _In_ DWORD dwStyleFlags) noexcept;

    /** @brief Fills a region-local cell rectangle with a character and attributes. */
    VOID Fill(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ WCHAR chCodepointW,
              _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

    /** @brief Draws a horizontal box-drawing or shading stroke in a region. */
    VOID DrawHorizontalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ DWORD dwStrokeType,
                            _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a vertical box-drawing or shading stroke in a region. */
    VOID DrawVerticalLine(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iHeight, _In_ DWORD dwStrokeType,
                          _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;
    /** @brief Draws a box with selected double-line sides in a region. */
    VOID DrawBox(_In_opt_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _In_ DWORD dwBoxSideFlags,
                 _In_ COLORREF crForeground, _In_ COLORREF crBackground, _In_ DWORD dwStyleFlags) noexcept;

    /** @brief Restores a region's current attributes to the root defaults. */
    VOID ResetAttributes(_In_opt_ RegionHandle hRegion) noexcept;

    /** @brief Writes one printable codepoint at a region's cursor. */
    VOID PutCodepoint(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept;
    /** @brief Processes one terminal control codepoint for a region. */
    VOID ProcessControl(_In_opt_ RegionHandle hRegion, _In_ WCHAR chCodepointW) noexcept;

    /** @brief Moves a region cursor by a signed cell offset. */
    VOID MoveCursorRelative(_In_opt_ RegionHandle hRegion, _In_ INT iDeltaX, _In_ INT iDeltaY) noexcept;
    /** @brief Sets a region cursor from one-based ANSI row and column coordinates. */
    VOID SetCursorPosition(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased, _In_ INT iColOneBased) noexcept;
    /** @brief Sets a region cursor column from a one-based ANSI coordinate. */
    VOID SetCursorColumn(_In_opt_ RegionHandle hRegion, _In_ INT iColOneBased) noexcept;
    /** @brief Sets a region cursor row from a one-based ANSI coordinate. */
    VOID SetCursorRow(_In_opt_ RegionHandle hRegion, _In_ INT iRowOneBased) noexcept;

    /** @brief Erases part or all of the current cursor row according to an ANSI erase mode. */
    VOID EraseInLine(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept;
    /** @brief Erases part or all of a region according to an ANSI erase mode. */
    VOID EraseInDisplay(_In_opt_ RegionHandle hRegion, _In_ INT iMode) noexcept;

    /** @brief Applies ANSI SGR parameters to a region's current attributes. */
    VOID SetGraphicsRendition(_In_opt_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams, _In_ SIZE_T uParamsCount) noexcept;

    /** @brief Creates a normal child region. */
    HRESULT CreateRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                         _In_opt_ RegionHandle hRegionParent) noexcept;
    /** @brief Creates a child region rendered by a Direct2D callback. */
    HRESULT CreateCustomDrawRegion(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight, _Out_ RegionHandle *lphRegion,
                                   _In_ const std::function<VOID(_In_ DrawContext &drawContext, _In_ RegionHandle hRegion)> &fnDrawCallback,
                                   _In_opt_ RegionHandle hRegionParent) noexcept;
    /** @brief Destroys a region and its descendant subtree. */
    HRESULT DestroyRegion(_In_ RegionHandle hRegion) noexcept;

    /** @brief Changes a region's parent-local location and size. */
    HRESULT RelocateRegion(_In_ RegionHandle hRegion, _In_ INT iX, _In_ INT iY, _In_ INT iWidth, _In_ INT iHeight) noexcept;
    /** @brief Shows or hides a region and its descendants. */
    HRESULT SetRegionVisible(_In_ RegionHandle hRegion, _In_ BOOL bVisible) noexcept;
    /** @brief Moves a region in front of its siblings. */
    HRESULT BringRegionToFront(_In_ RegionHandle hRegion) noexcept;
    /** @brief Moves a region behind its siblings. */
    HRESULT SendRegionToBack(_In_ RegionHandle hRegion) noexcept;
    /** @brief Places a region after a sibling, or last when the sibling is null. */
    HRESULT MoveRegionAfter(_In_ RegionHandle hRegion, _In_opt_ RegionHandle hRegionReference) noexcept;

    /** @brief Associates application-owned context with a region. */
    HRESULT SetRegionContext(_In_opt_ RegionHandle hRegion, _In_opt_ PVOID lpContext) noexcept;
    /** @brief Returns a region's application-owned context. */
    PVOID GetRegionContext(_In_opt_ RegionHandle hRegion) const noexcept;

    /** @brief Registers a callback invoked immediately before region destruction. */
    HRESULT SetRegionDestroyCallback(_In_ RegionHandle hRegion,
                                     _In_ const std::function<VOID(_In_ RegionHandle hRegion)> &fnCallback) noexcept;
    /** @brief Registers a callback that releases invalid custom-draw resources. */
    HRESULT SetCustomDrawRegionResourceCleanup(
        _In_ RegionHandle hRegion, _In_ const std::function<VOID(_In_ CustomDrawResourceCleanupReason cleanupReason)> &fnCallback) noexcept;
    /** @brief Invokes resource-cleanup callbacks for every custom-draw region. */
    VOID NotifyCustomDrawResourceCleanup(_In_ CustomDrawResourceCleanupReason cleanupReason) noexcept;

    /** @brief Returns the first root-level region, or nullptr when none exists. */
    RegionHandle GetFirstRegion() const noexcept;
    /** @brief Returns the last root-level region, or nullptr when none exists. */
    RegionHandle GetLastRegion() const noexcept;
    /** @brief Returns the next sibling region. */
    RegionHandle GetNextRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    /** @brief Returns the previous sibling region. */
    RegionHandle GetPreviousRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    /** @brief Returns the first child of a region. */
    RegionHandle GetChildFirstRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
    /** @brief Returns the last child of a region. */
    RegionHandle GetChildLastRegion(_In_opt_ RegionHandle hRegionParent) const noexcept;
    /** @brief Returns a region's parent, or nullptr for a root-level region. */
    RegionHandle GetParentRegion(_In_ RegionHandle hRegion) const noexcept;

    /** @brief Retrieves parent-local region geometry. */
    VOID GetRegionLocation(_In_ RegionHandle hRegion, _Out_opt_ LPINT lpiX, _Out_opt_ LPINT lpiY, _Out_opt_ LPINT lpiWidth,
                           _Out_opt_ LPINT lpiHeight) const noexcept;
    /** @brief Converts a terminal cell coordinate to a region-local coordinate. */
    BOOL ConvertToRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColTerminal, _In_ INT iRowTerminal, _Out_opt_ LPINT lpiColRegion,
                                    _Out_opt_ LPINT lpiRowRegion) const noexcept;
    /** @brief Converts a region-local cell coordinate to a terminal coordinate. */
    BOOL ConvertFromRegionCoordinates(_In_ RegionHandle hRegion, _In_ INT iColRegion, _In_ INT iRowRegion, _Out_opt_ LPINT lpiColTerminal,
                                      _Out_opt_ LPINT lpiRowTerminal) const noexcept;

    /** @brief Saves a region cursor position. */
    VOID SaveCursor(_In_opt_ RegionHandle hRegion) noexcept;
    /** @brief Restores a previously saved region cursor position. */
    VOID RestoreCursor(_In_opt_ RegionHandle hRegion) noexcept;
    /** @brief Shows the cursor in a region. */
    VOID ShowCursor(_In_opt_ RegionHandle hRegion) noexcept;
    /** @brief Hides the cursor. */
    VOID HideCursor() noexcept;
    /** @brief Sets the renderer cursor-style flags. */
    VOID SetCursorStyle(_In_ DWORD dwCursorStyle) noexcept;
    /** @brief Toggles the blink visibility state. */
    VOID ToggleBlinkVisibility() noexcept;
    /** @brief Explicitly sets the blink visibility state. */
    VOID SetBlinkVisible(_In_ BOOL bBlinkVisible) noexcept;

    /** @brief Builds a root-terminal snapshot for rendering. */
    HRESULT GetSnapshot(_Out_ Snapshot *lpSnapshot) const noexcept;
    /** @brief Builds visible regions in composition order for rendering. */
    HRESULT GetRenderPlan(_Out_ std::vector<RenderItem> *lpRenderItems) const noexcept;
    /** @brief Returns identifiers for all custom-draw regions. */
    HRESULT GetCustomDrawRegionIds(_Out_ std::vector<INT> *lpRegionIds) const noexcept;
    /** @brief Returns TRUE when a region is custom-draw backed. */
    BOOL IsCustomDrawRegion(_In_ RegionHandle hRegion) const noexcept;

  private:
    HRESULT InitializeRootRegion() noexcept;
    HRESULT ValidateRegionBounds(_In_ INT iWidth, _In_ INT iHeight) const noexcept;
    Region_t *ResolveRegion(_In_opt_ RegionHandle hRegion) noexcept;
    const Region_t *ResolveRegion(_In_opt_ RegionHandle hRegion) const noexcept;
    HRESULT RemoveRegionFromParent(_Inout_ Region_t &sRegion) noexcept;
    HRESULT DestroyRegionRecursive(_In_ RegionHandle hRegion) noexcept;
    VOID GetRegionTerminalOrigin(_In_ const Region_t &sRegion, _Out_ LONGLONG &llX, _Out_ LONGLONG &llY) const noexcept;
    VOID GetRegionVisibleTerminalRect(_In_ const Region_t &sRegion, _Out_ CellRect_t &sRectVisible) const noexcept;
    BOOL IsRegionVisible(_In_ const Region_t &sRegion) const noexcept;

    Cell MakeBlankCell() const noexcept;
    HRESULT InitializeRegionCells(_Inout_ Region_t &sRegion) const noexcept;
    HRESULT ResizeRegionCells(_In_ const Region_t &sRegionSource, _Out_ std::vector<Cell> &vecCellsTarget, _In_ INT iWidthTarget,
                              _In_ INT iHeightTarget) const noexcept;
    VOID ClearRegionCells(_Inout_ Region_t &sRegion) const noexcept;
    VOID SetCell(_Inout_ Region_t &sRegion, _In_ INT iX, _In_ INT iY, _In_ WCHAR chCodepointW,
                 _In_ const Attributes_t &sAttributesCell) noexcept;
    VOID FillCell(_Inout_ Region_t &sRegion, _In_ INT iX, _In_ INT iY, _In_ const Attributes_t &sAttributesCell) noexcept;
    VOID FillRange(_Inout_ Region_t &sRegion, _In_ INT iXStart, _In_ INT iYStart, _In_ INT iXEnd, _In_ INT iYEnd,
                   _In_ const Attributes_t &sAttributesCell) noexcept;
    BOOL IsCoordinateInsideRectangle(_In_ INT iX, _In_ INT iY, _In_ INT iStartX, _In_ INT iStartY, _In_ INT iEndX,
                                     _In_ INT iEndY) const noexcept;
    const Cell *GetCell(_In_ const Region_t &sRegion, _In_ INT iX, _In_ INT iY) const noexcept;
    VOID DrawStrokeCell(_Inout_ Region_t &sRegion, _In_ INT iX, _In_ INT iY, _In_ DWORD dwStrokeType, _In_ BYTE byUp, _In_ BYTE byRight,
                        _In_ BYTE byDown, _In_ BYTE byLeft, _In_ const Attributes_t &sAttributesCell) noexcept;
    BOOL ClipRectangle(_In_ const Region_t &sRegion, _In_ const CellRect_t &sRectSource, _Out_ CellRect_t &sRectClipped) const noexcept;

    VOID ScrollRegionUp(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;
    VOID ScrollRegionDown(_In_opt_ RegionHandle hRegion, _In_ INT iLineCount) noexcept;

    VOID AdvanceCursorAfterWrite(_In_opt_ RegionHandle hRegion) noexcept;

    BOOL GetCellIndex(_In_ INT iX, _In_ INT iY, _In_ INT iWidth, _Out_ PSIZE_T lpuIndex) const noexcept;
    HRESULT ComposeRegion(_In_ const Region_t &sRegion) const noexcept;
    HRESULT ComposeRegionTree(_In_ const Region_t &sRegionParent) const noexcept;
    HRESULT BuildRenderPlan(_In_ const Region_t &sRegion, _Inout_ std::vector<RenderItem> &vecRenderItems) const noexcept;

    VOID ApplySgrColor(_In_ RegionHandle hRegion, _In_reads_(uParamsCount) LPINT lpiParams, _In_ SIZE_T uParamsCount,
                       _Inout_ PSIZE_T lpuIndex, _In_ BOOL bForeground) noexcept;
    VOID AdvanceToNextTabStop(_In_opt_ RegionHandle hRegion) noexcept;

  private:
    INT m_iCols{};
    INT m_iRows{};
    mutable std::vector<Cell> m_vecSnapshotCells;
    Attributes_t m_sAttributesDefault{};
    std::unordered_map<INT, Region_t> m_mapRegions;
    RegionHandle m_hRegionCursorActive{};
    INT m_iNextRegionId{1};
    BOOL m_bBlinkVisible{TRUE};
    BOOL m_bCursorVisible{FALSE};
    DWORD m_dwCursorStyle{};
};

} // namespace Internals

} // namespace GuiTerminal
