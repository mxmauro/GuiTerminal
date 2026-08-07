#pragma once

#include "GuiTerminalBuffer.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <wrl\client.h>

// -----------------------------------------------------------------------------

namespace GuiTerminal {

namespace Internals {

class Renderer
{
  public:
    Renderer() noexcept = default;
    Renderer(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    ~Renderer() noexcept = default;

    Renderer &operator=(const Renderer &) = delete;
    Renderer &operator=(Renderer &&) = delete;

    HRESULT Initialize(_In_ HWND hWnd, _In_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize) noexcept;
    HRESULT Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;
    HRESULT Render(_In_ const Buffer &bufferGuiTerminal) noexcept;

    BOOL GetCellPosition(_In_ INT iCol, _In_ INT iRow, _Out_ LPRECT lprcCell) const noexcept;
    HRESULT GetCellSize(_Out_ LPSIZE lpSize) const noexcept;
    HRESULT GetPreferredClientSize(_In_ INT iCols, _In_ INT iRows, _Out_ LPSIZE lpSize) const noexcept;
    VOID SetContentSize(_In_ INT iCols, _In_ INT iRows) noexcept;
    VOID UpdateScrollBars() noexcept;
    BOOL HasVisibleScrollBars() const noexcept;

    BOOL HitTestCell(_In_ INT iX, _In_ INT iY, _Out_opt_ LPINT lpiCol, _Out_opt_ LPINT lpiRow) const noexcept;
    BOOL HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept;
    BOOL HandleMouseLeave() noexcept;
    BOOL HitTestScrollBars(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbVertical, _Out_opt_ PBOOL lpbThumb) const noexcept;
    BOOL ScrollByTrackClick(_In_ BOOL bVertical, _In_ INT iPointerCoordinate) noexcept;
    BOOL ScrollByWheelDelta(_In_ SHORT iDelta) noexcept;
    BOOL ScrollByPage(_In_ BOOL bVertical, _In_ BOOL bForward) noexcept;
    BOOL SetScrollOffset(_In_ INT iOffsetX, _In_ INT iOffsetY) noexcept;
    INT GetScrollOffsetX() const noexcept;
    INT GetScrollOffsetY() const noexcept;
    BOOL ScrollFromThumbDrag(_In_ BOOL bVertical, _In_ INT iPointerCoordinate, _In_ INT iPointerOrigin, _In_ INT iOffsetOrigin) noexcept;
    VOID RefreshDpi() noexcept;

    VOID DrawCustomRegion(_In_ const Buffer::RenderItem &sRenderItem) noexcept;
    VOID DrawContextClear(_In_ COLORREF crColor) noexcept;
    VOID DrawContextPoint(_In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor, _In_ FLOAT fRadius) noexcept;
    VOID DrawContextLine(_In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2, _In_ FLOAT fY2, _In_ COLORREF crColor,
                         _In_ FLOAT fStrokeWidth) noexcept;
    VOID DrawContextRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                              _In_ BOOL bFill) noexcept;
    VOID DrawContextRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth,
                                     _In_ BOOL bFill) noexcept;
    VOID DrawContextEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth, _In_ BOOL bFill) noexcept;
    VOID DrawContextWrite(_In_z_ LPCWSTR szTextW, _In_ const D2D1_POINT_2F &pointReference, _In_ COLORREF crColor,
                          _In_opt_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize, _In_ DWORD dwStyleFlags,
                          _In_ DrawContext::TextAlignment textAlignment, _In_ FLOAT fRotationDegrees) noexcept;

    VOID BeginPath() noexcept;
    VOID MovePathTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept;
    VOID AddPathLine(_In_ FLOAT fX, _In_ FLOAT fY) noexcept;
    VOID AddPathQuadraticBezier(_In_ FLOAT fControlX, _In_ FLOAT fControlY, _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept;
    VOID AddPathCubicBezier(_In_ FLOAT fControl1X, _In_ FLOAT fControl1Y, _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y, _In_ FLOAT fEndX,
                            _In_ FLOAT fEndY) noexcept;
    VOID AddPathArc(_In_ FLOAT fEndX, _In_ FLOAT fEndY, _In_ FLOAT fRadiusX, _In_ FLOAT fRadiusY, _In_ FLOAT fRotationDegrees,
                    _In_ D2D1_SWEEP_DIRECTION sweepDirection, _In_ D2D1_ARC_SIZE arcSize) noexcept;
    VOID ClosePath() noexcept;
    VOID StrokePath(_In_ COLORREF crColor, _In_ FLOAT fStrokeWidth) noexcept;
    VOID FillPath(_In_ COLORREF crColor) noexcept;

    UINT GetDeviceGeneration() const noexcept;
    ID2D1RenderTarget *GetRenderTarget() const noexcept;

  private:
    typedef struct FontMetrics_s
    {
        INT iCellWidthPx{};
        INT iCellHeightPx{};
        INT iBaselinePx{};
        INT iUnderlineOffsetPx{};
        INT iUnderlineThicknessPx{};
        std::wstring strFontFamilyW;
        FLOAT fFontSize{12.0f};
    } FontMetrics;

    typedef struct ScrollBarMetrics_s
    {
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

  private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    HRESULT CreateDeviceResources() noexcept;
    HRESULT CreateTextFormatAndMetrics() noexcept;
    FLOAT PixelsToDipsX(_In_ INT iPixels) const noexcept;
    FLOAT PixelsToDipsY(_In_ INT iPixels) const noexcept;
    INT DipsToPixelsX(_In_ FLOAT fDips) const noexcept;
    INT DipsToPixelsY(_In_ FLOAT fDips) const noexcept;
    VOID UpdateViewportLayout() noexcept;
    VOID UpdateScrollBarMetrics(_Inout_ ScrollBarMetrics &scrollBarMetrics, _In_ BOOL bVertical) noexcept;
    VOID DrawScrollBars(_In_ COLORREF crDefaultBackground) noexcept;
    VOID DrawCells(_In_ const Buffer::Snapshot &sSnapshotBuffer) noexcept;
    VOID DrawRegionCells(_In_ const Buffer::RenderItem &sRenderItem, _In_ const Buffer::Snapshot &sSnapshotBuffer) noexcept;
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
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> m_pathGeometry;
    Microsoft::WRL::ComPtr<ID2D1GeometrySink> m_pathSink;
    BOOL m_bPathFigureOpen{FALSE};
    BOOL m_bPathClosed{FALSE};
    UINT m_uiDeviceGeneration{};
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat[4];
};

} // namespace Internals

} // namespace GuiTerminal
