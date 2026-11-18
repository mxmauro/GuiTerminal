#pragma once

#include "GuiTerminalBuffer.h"
#include <d2d1_1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <dxgi1_3.h>
#include <string>
#include <unordered_map>
#include <wrl\client.h>

// -----------------------------------------------------------------------------

namespace GuiTerminal {

namespace Internals {

/**
 * @brief Renders Buffer snapshots into the control window and implements Direct2D drawing support.
 * @remarks This is a lower-level library contract. Applications normally use Control and DrawContext instead.
 */
class Renderer
{
  public:
    /** @brief Constructs an uninitialized renderer. */
    Renderer() noexcept = default;
    /** @brief Prevents copying renderer-owned Direct2D resources. */
    Renderer(const Renderer &) = delete;
    /** @brief Prevents moving renderer-owned Direct2D resources. */
    Renderer(Renderer &&) = delete;
    /** @brief Releases renderer resources. */
    ~Renderer() noexcept;

    /** @brief Prevents copy assignment. */
    Renderer &operator=(const Renderer &) = delete;
    /** @brief Prevents move assignment. */
    Renderer &operator=(Renderer &&) = delete;

    /** @brief Initializes rendering resources for a control window. */
    HRESULT Initialize(_In_ HWND hWnd, _In_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize) noexcept;
    /** @brief Updates the client-pixel dimensions used for rendering. */
    HRESULT Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;
    /** @brief Builds the next frame from a buffer snapshot. */
    HRESULT Render(_In_ const Buffer &bufferGuiTerminal) noexcept;
    /** @brief Presents the most recently rendered frame. */
    HRESULT Present() noexcept;
    /** @brief Returns the frame-latency waitable handle, or nullptr before device resources exist. */
    HANDLE GetFrameLatencyWaitableObject() const noexcept;
    /** @brief Returns TRUE once after a successful present, when the render thread must wait for frame latency. */
    BOOL ConsumeFrameLatencyWait() noexcept;
    /** @brief Marks one custom-draw region, or all regions when null, for redraw. */
    VOID InvalidateCustomDrawRegion(_In_opt_ RegionHandle hRegion) noexcept;
    /** @brief Marks the terminal cell composition for full redraw. */
    VOID InvalidateComposition() noexcept;

    /** @brief Retrieves the client rectangle occupied by a terminal cell. */
    BOOL GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept;
    /** @brief Retrieves the current terminal-cell size in pixels. */
    HRESULT GetCellSize(_Out_ LPSIZE lpSize) const noexcept;
    /** @brief Computes the client size required to display a specified terminal grid. */
    HRESULT GetPreferredClientSize(_In_ INT iCols, _In_ INT iRows, _Out_ LPSIZE lpSize) const noexcept;
    /** @brief Updates the terminal content dimensions used to calculate scrollbars. */
    VOID SetContentSize(_In_ INT iCols, _In_ INT iRows) noexcept;
    /** @brief Recalculates scrollbar geometry and visibility. */
    VOID UpdateScrollBars() noexcept;
    /** @brief Returns TRUE when either scrollbar is currently visible. */
    BOOL HasVisibleScrollBars() const noexcept;
    /** @brief Returns the scrollbar thickness in client pixels. */
    INT GetScrollBarThickness() const noexcept;

    /** @brief Converts a client-pixel position to a terminal cell. */
    BOOL HitTestCell(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept;
    /** @brief Updates scrollbar hover state for a client-pixel mouse position. */
    BOOL HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept;
    /** @brief Clears scrollbar hover state after the pointer leaves the control. */
    BOOL HandleMouseLeave() noexcept;
    /** @brief Hit-tests the scrollbar controls at a client-pixel mouse position. */
    BOOL HitTestScrollBars(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbVertical, _Out_opt_ PBOOL lpbThumb) const noexcept;
    /** @brief Scrolls by clicking a scrollbar track. */
    BOOL ScrollByTrackClick(_In_ BOOL bVertical, _In_ INT iPointerCoordinate) noexcept;
    /** @brief Scrolls from a mouse-wheel delta. */
    BOOL ScrollByWheelDelta(_In_ SHORT iDelta) noexcept;
    /** @brief Scrolls one viewport page in the requested axis and direction. */
    BOOL ScrollByPage(_In_ BOOL bVertical, _In_ BOOL bForward) noexcept;
    /** @brief Sets horizontal and vertical scroll offsets. */
    BOOL SetScrollOffset(_In_ INT iOffsetX, _In_ INT iOffsetY) noexcept;
    /** @brief Returns the horizontal scroll offset in pixels. */
    INT GetScrollOffsetX() const noexcept;
    /** @brief Returns the vertical scroll offset in pixels. */
    INT GetScrollOffsetY() const noexcept;
    /** @brief Scrolls from a scrollbar-thumb drag operation. */
    BOOL ScrollFromThumbDrag(_In_ BOOL bVertical, _In_ INT iPointerCoordinate, _In_ INT iPointerOrigin, _In_ INT iOffsetOrigin) noexcept;
    /** @brief Refreshes DPI-dependent rendering metrics. */
    VOID RefreshDpi() noexcept;

    /** @brief Invokes a custom region's Direct2D draw callback. */
    VOID DrawCustomRegion(_In_ const Buffer::RenderItem &sRenderItem) noexcept;
    /** @brief Clears the current custom-draw region. */
    VOID DrawContextClear(_In_ COLORREF crColor) noexcept;
    /** @brief Draws a filled custom-draw point. */
    VOID DrawContextPoint(_In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor, _In_ FLOAT fRadius) noexcept;
    /** @brief Draws a custom-draw line segment. */
    VOID DrawContextLine(_In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2, _In_ FLOAT fY2, _In_ COLORREF crColor,
                         _In_ FLOAT fStrokeWidth) noexcept;
    /** @brief Draws or fills a custom-draw rectangle. */
    VOID DrawContextRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                              _In_ BOOL bFill) noexcept;
    /** @brief Draws or fills a custom-draw rounded rectangle. */
    VOID DrawContextRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                                     _In_ BOOL bFill) noexcept;
    /** @brief Draws or fills a custom-draw ellipse. */
    VOID DrawContextEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth, _In_ BOOL bFill) noexcept;
    /** @brief Draws styled custom-draw text. */
    VOID DrawContextWrite(_In_z_ LPCWSTR szTextW, _In_ const D2D1_POINT_2F &pointReference, _In_ COLORREF crColor,
                          _In_opt_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize, _In_ DWORD dwStyleFlags,
                          _In_ DrawContext::TextAlignment textAlignment, _In_ FLOAT fRotationDegrees) noexcept;

    /** @brief Starts a new custom-draw path. */
    VOID BeginPath() noexcept;
    /** @brief Starts a path figure. */
    VOID MovePathTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept;
    /** @brief Adds a straight path segment. */
    VOID AddPathLine(_In_ FLOAT fX, _In_ FLOAT fY) noexcept;
    /** @brief Adds a quadratic Bezier path segment. */
    VOID AddPathQuadraticBezier(_In_ FLOAT fControlX, _In_ FLOAT fControlY, _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept;
    /** @brief Adds a cubic Bezier path segment. */
    VOID AddPathCubicBezier(_In_ FLOAT fControl1X, _In_ FLOAT fControl1Y, _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y, _In_ FLOAT fEndX,
                            _In_ FLOAT fEndY) noexcept;
    /** @brief Adds an arc path segment. */
    VOID AddPathArc(_In_ FLOAT fEndX, _In_ FLOAT fEndY, _In_ FLOAT fRadiusX, _In_ FLOAT fRadiusY, _In_ FLOAT fRotationDegrees,
                    _In_ D2D1_SWEEP_DIRECTION sweepDirection, _In_ D2D1_ARC_SIZE arcSize) noexcept;
    /** @brief Closes the current path figure. */
    VOID ClosePath() noexcept;
    /** @brief Strokes the completed path. */
    VOID StrokePath(_In_ COLORREF crColor, _In_ FLOAT fStrokeWidth) noexcept;
    /** @brief Fills the completed path. */
    VOID FillPath(_In_ COLORREF crColor) noexcept;

    /** @brief Returns the Direct2D target active during custom drawing. */
    ID2D1RenderTarget *GetRenderTarget() const noexcept;

  private:
    typedef struct FontMetrics_s {
        INT iCellWidthPx{};
        INT iCellHeightPx{};
        INT iBaselinePx{};
        INT iUnderlineOffsetPx{};
        INT iUnderlineThicknessPx{};
        std::wstring strFontFamilyW;
        FLOAT fFontSize{12.0f};
    } FontMetrics;

    typedef struct ScrollBarMetrics_s {
        BOOL bVisible{FALSE};
        BOOL bHot{FALSE};
        RECT rcTrack{};
        RECT rcThumb{};
        INT iThumbTravel{};
        INT iViewportSize{};
        INT iContentSize{};
        INT iOffset{};
        INT iMaxOffset{};
    } ScrollBarMetrics;

    typedef struct CellRenderState_s {
        Buffer::Cell sCell{};
        BOOL bDirty{};
        BOOL bHasBlink{};
    } CellRenderState;

    typedef struct CustomDrawCache_s {
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
        UINT uiWidth{};
        UINT uiHeight{};
        BOOL bDirty{TRUE};
    } CustomDrawCache;

  private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    HRESULT CreateDeviceResources(_In_ const Buffer &bufferGuiTerminal) noexcept;
    HRESULT CreateTargetBitmap() noexcept;
    HRESULT CreateCompositionBitmap() noexcept;
    HRESULT ResizeDeviceResources(_In_ const Buffer &bufferGuiTerminal) noexcept;
    VOID DiscardDeviceResources() noexcept;
    VOID AddDirtyRect(_In_ const RECT &rcDirty) noexcept;
    HRESULT UpdateCustomDrawCache(_In_ const Buffer::RenderItem &sRenderItem) noexcept;
    VOID PruneCustomDrawCaches(_In_ const std::vector<INT> &vecCustomDrawRegionIds) noexcept;
    HRESULT CreateTextFormatAndMetrics() noexcept;
    FLOAT PixelsToDipsX(_In_ INT iPixels) const noexcept;
    FLOAT PixelsToDipsY(_In_ INT iPixels) const noexcept;
    INT DipsToPixelsX(_In_ FLOAT fDips) const noexcept;
    INT DipsToPixelsY(_In_ FLOAT fDips) const noexcept;
    VOID UpdateViewportLayout() noexcept;
    VOID UpdateScrollBarMetrics(_Inout_ ScrollBarMetrics &scrollBarMetrics, _In_ BOOL bVertical) noexcept;
    VOID DrawScrollBars(_In_ COLORREF crDefaultBackground) noexcept;
    VOID ClearScrollBarAreas(_In_ COLORREF crDefaultBackground) noexcept;
    VOID DrawCells(_In_ const Buffer::Snapshot &sSnapshotBuffer, _In_ const CellRect_t &sRectDirty) noexcept;
    VOID DrawRegionCells(_In_ const Buffer::RenderItem &sRenderItem, _In_ const Buffer::Snapshot &sSnapshotBuffer,
                         _In_ const CellRect_t &sRectDirty) noexcept;
    VOID DrawCell(_In_ const Buffer::Cell &sCellCurrent, _In_ INT iCol, _In_ INT iRow,
                  _In_ const Buffer::Snapshot &sSnapshotBuffer) noexcept;
    VOID DrawCursor(_In_ const Buffer::Snapshot &sSnapshotBuffer) noexcept;
    VOID DrawGlyph(_In_ WCHAR chCodepointW, _In_ DWORD dwStyleFlags, _In_ COLORREF crForeground, _In_ const D2D1_RECT_F &rcText) noexcept;

  private:
    HWND m_hWnd{};
    FLOAT m_fDpiX{96.0f};
    FLOAT m_fDpiY{96.0f};
    INT m_iGridOffsetX{};
    INT m_iGridOffsetY{};
    RECT m_rcViewport{};
    INT m_iCols{};
    INT m_iRows{};
    INT m_iClientWidth{};
    INT m_iClientHeight{};
    INT m_iScrollBarThickness{};
    INT m_iScrollOffsetX{};
    INT m_iScrollOffsetY{};
    FontMetrics m_metricsFont;
    ScrollBarMetrics m_scrollBarHorizontal;
    ScrollBarMetrics m_scrollBarVertical;
    Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain2> m_swapChain;
    Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_targetBitmap;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_compositionBitmap;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> m_pathGeometry;
    Microsoft::WRL::ComPtr<ID2D1GeometrySink> m_pathSink;
    BOOL m_bPathFigureOpen{FALSE};
    BOOL m_bPathClosed{FALSE};
    UINT m_uiRenderTargetWidth{};
    UINT m_uiRenderTargetHeight{};
    HANDLE m_hFrameLatencyWaitableObject{};
    BOOL m_bWaitForFrameLatency{FALSE};
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat[4];
    std::vector<CellRenderState> m_vecCachedCells;
    std::unordered_map<INT, CustomDrawCache> m_mapCustomDrawCaches;
    RECT m_rcDirty[3]{};
    UINT m_uDirtyCount{};
    INT m_iCachedCols{};
    INT m_iCachedRows{};
    BOOL m_bCachedBlinkVisible{};
    BOOL m_bCachedCursorVisible{};
    INT m_iCachedCursorCol{};
    INT m_iCachedCursorRow{};
    DWORD m_dwCachedCursorStyle{};
    BOOL m_bCompositionInvalid{TRUE};
    BOOL m_bScrollBarsDirty{TRUE};
    BOOL m_bFrameReady{FALSE};
    BOOL m_bFrameFull{TRUE};
};

} // namespace Internals

} // namespace GuiTerminal
