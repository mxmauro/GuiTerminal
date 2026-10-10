#pragma once

#define WIN32_LEAN_AND_MEAN
#include <d2d1.h>
#include <windows.h>

// -----------------------------------------------------------------------------

namespace GuiTerminal {

namespace Internals {

typedef struct Region_s Region_t;
class Renderer;

} // namespace Internals

/** @brief Opaque handle that identifies a root-level or child region. */
typedef Internals::Region_t *RegionHandle;

/** @brief Identifies why custom-draw Direct2D resources must be released. */
typedef enum class CustomDrawResourceCleanupReason_e : DWORD {
    TargetLost = 0U, /**< Direct2D or DXGI requires the render target to be recreated. */
    RegionDestroyed, /**< The region and its callback state are being removed. */
    TargetResized,   /**< The swap-chain target is about to be resized. */
    DpiChanged       /**< DPI-sensitive custom resources must be discarded. */
} CustomDrawResourceCleanupReason;

// -----------------------------------------------------------------------------

/**
 * @brief Provides Direct2D drawing operations to a custom-draw region callback.
 */
class DrawContext
{
  public:
    /** @brief Selects horizontal and vertical alignment for DrawContext::Write. */
    typedef enum TextAlignment_e : DWORD {
        AlignLeft = 0U,        /**< Align text to the reference point's left. */
        AlignCenter = 1U << 0, /**< Center text horizontally on the reference point. */
        AlignRight = 2U << 0,  /**< Align text to the reference point's right. */
        AlignTop = 0U,         /**< Align text to the reference point's top. */
        AlignMiddle = 1U << 2, /**< Center text vertically on the reference point. */
        AlignBottom = 2U << 2, /**< Align text to the reference point's bottom. */
        AlignBaseline = 3U << 2 /**< Align the text baseline to the reference point. */
    } TextAlignment;

    /** @brief Fills the custom-draw region with a color. */
    VOID Clear(_In_ COLORREF crColor) noexcept;

    /** @brief Draws a filled point in device-independent pixels. */
    VOID DrawPoint(_In_ FLOAT fX, _In_ FLOAT fY, _In_ COLORREF crColor, _In_ FLOAT fRadius = 1.0f) noexcept;
    /** @brief Draws a line in device-independent pixels. */
    VOID DrawLine(_In_ FLOAT fX1, _In_ FLOAT fY1, _In_ FLOAT fX2, _In_ FLOAT fY2, _In_ COLORREF crColor,
                  _In_ FLOAT fStrokeWidth = 1.0f) noexcept;

    /** @brief Draws a rectangle in device-independent pixels. */
    VOID DrawRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth = 1.0f) noexcept;
    /** @brief Fills a rectangle in device-independent pixels. */
    VOID FillRectangle(_In_ const D2D1_RECT_F &rcRectangle, _In_ COLORREF crColor) noexcept;
    /** @brief Draws a rounded rectangle in device-independent pixels. */
    VOID DrawRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth = 1.0f) noexcept;
    /** @brief Fills a rounded rectangle in device-independent pixels. */
    VOID FillRoundedRectangle(_In_ const D2D1_ROUNDED_RECT &rcRectangle, _In_ COLORREF crColor) noexcept;

    /** @brief Draws an ellipse in device-independent pixels. */
    VOID DrawEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor, _In_ FLOAT fStrokeWidth = 1.0f) noexcept;
    /** @brief Fills an ellipse in device-independent pixels. */
    VOID FillEllipse(_In_ const D2D1_ELLIPSE &ellipse, _In_ COLORREF crColor) noexcept;

    /**
     * @brief Draws styled text at a reference point, optionally rotated.
     * @param pointReference Anchor point interpreted according to @p textAlignment.
     * @param szFontFamilyW Optional font family; nullptr selects the region's terminal font.
     * @param fFontSize Optional font size in DIPs; zero selects the terminal font size.
     * @param fRotationDegrees Clockwise rotation around @p pointReference, in degrees.
     */
    VOID Write(_In_z_ LPCWSTR szTextW, _In_ const D2D1_POINT_2F &pointReference, _In_ COLORREF crColor,
               _In_opt_z_ LPCWSTR szFontFamilyW = nullptr, _In_ FLOAT fFontSize = 0.0f, _In_ DWORD dwStyleFlags = 0U,
               _In_ TextAlignment textAlignment = static_cast<TextAlignment>(AlignTop | AlignLeft),
               _In_ FLOAT fRotationDegrees = 0.0f) noexcept;

    /**
     * @brief Starts a new path and discards any path currently being built.
     * @remarks Call this before adding path segments.
     */
    VOID BeginPath() noexcept;
    /** @brief Starts the current path figure at a point. */
    VOID MoveTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept;
    /** @brief Adds a straight segment to the current path figure. */
    VOID LineTo(_In_ FLOAT fX, _In_ FLOAT fY) noexcept;
    /** @brief Adds a quadratic Bezier segment to the current path figure. */
    VOID QuadraticBezierTo(_In_ FLOAT fControlX, _In_ FLOAT fControlY, _In_ FLOAT fEndX, _In_ FLOAT fEndY) noexcept;
    /** @brief Adds a cubic Bezier segment to the current path figure. */
    VOID CubicBezierTo(_In_ FLOAT fControl1X, _In_ FLOAT fControl1Y, _In_ FLOAT fControl2X, _In_ FLOAT fControl2Y, _In_ FLOAT fEndX,
                       _In_ FLOAT fEndY) noexcept;
    /** @brief Adds an arc segment to the current path figure. */
    VOID ArcTo(_In_ FLOAT fEndX, _In_ FLOAT fEndY, _In_ FLOAT fRadiusX, _In_ FLOAT fRadiusY, _In_ FLOAT fRotationDegrees,
               _In_ D2D1_SWEEP_DIRECTION sweepDirection, _In_ D2D1_ARC_SIZE arcSize) noexcept;
    /** @brief Closes the current path figure. */
    VOID ClosePath() noexcept;
    /**
     * @brief Strokes the completed path.
     * @remarks Call after finishing the path with the path-building methods.
     */
    VOID StrokePath(_In_ COLORREF crColor, _In_ FLOAT fStrokeWidth = 1.0f) noexcept;
    /**
     * @brief Fills the completed path.
     * @remarks Call after finishing the path with the path-building methods.
     */
    VOID FillPath(_In_ COLORREF crColor) noexcept;

    /** @brief Returns the custom-draw region width in device-independent pixels. */
    INT GetWidth() const noexcept;
    /** @brief Returns the custom-draw region height in device-independent pixels. */
    INT GetHeight() const noexcept;

    /**
     * @brief Returns the underlying Direct2D render target for advanced drawing.
     * @return The render target valid only on the renderer worker for the duration of the custom draw callback, or nullptr when
     *         no target is available.
     */
    ID2D1RenderTarget *GetDirect2DRenderTarget() const noexcept;

  private:
    friend class Internals::Renderer;

    DrawContext(_In_ Internals::Renderer *lpRenderer, _In_ INT iWidth, _In_ INT iHeight) noexcept;

    Internals::Renderer *m_lpRenderer{};
    INT m_iWidth{};
    INT m_iHeight{};
};

} // namespace GuiTerminal
