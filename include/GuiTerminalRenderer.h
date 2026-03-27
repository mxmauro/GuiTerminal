#pragma once

#include "GuiTerminalBuffer.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <wrl\client.h>

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    namespace Internals
    {
        class Renderer
        {
        public:
            Renderer() noexcept = default;
            Renderer(const Renderer&) = delete;
            Renderer(Renderer&&) = delete;
            ~Renderer() noexcept = default;

            Renderer& operator=(const Renderer&) = delete;
            Renderer& operator=(Renderer&&) = delete;

            HRESULT Initialize(_In_ HWND hWnd, _In_z_ LPCWSTR szFontFamilyW, _In_ FLOAT fFontSize) noexcept;
            HRESULT Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;
            HRESULT Render(_In_ const Buffer& bufferGuiTerminal) noexcept;
            HRESULT GetCellSize(_Out_ LPSIZE lpSize) const noexcept;
            HRESULT GetPreferredClientSize(_In_ INT iCols, _In_ INT iRows, _Out_ LPSIZE lpSize) const noexcept;
            VOID SetContentSize(_In_ INT iCols, _In_ INT iRows) noexcept;
            VOID UpdateScrollBars() noexcept;
            BOOL HasVisibleScrollBars() const noexcept;
            BOOL HandleMouseMove(_In_ INT iX, _In_ INT iY) noexcept;
            BOOL HandleMouseLeave() noexcept;
            BOOL HitTestScrollBars(_In_ INT iX, _In_ INT iY, _Out_opt_ PBOOL lpbVertical, _Out_opt_ PBOOL lpbThumb) const noexcept;
            BOOL ScrollByTrackClick(_In_ BOOL bVertical, _In_ INT iPointerCoordinate) noexcept;
            BOOL ScrollByWheelDelta(_In_ SHORT iDelta) noexcept;
            BOOL ScrollByPage(_In_ BOOL bVertical, _In_ BOOL bForward) noexcept;
            BOOL SetScrollOffset(_In_ INT iOffsetX, _In_ INT iOffsetY) noexcept;
            INT GetScrollOffsetX() const noexcept;
            INT GetScrollOffsetY() const noexcept;
            BOOL ScrollFromThumbDrag(_In_ BOOL bVertical, _In_ INT iPointerCoordinate, _In_ INT iPointerOrigin,
                                     _In_ INT iOffsetOrigin) noexcept;
            VOID RefreshDpi() noexcept;

        private:
            struct FontMetrics
            {
                FLOAT fCellWidth{};
                FLOAT fCellHeight{};
                FLOAT fBaseline{};
                FLOAT fUnderlineOffset{};
                FLOAT fUnderlineThickness{};
                std::wstring strFontFamilyW;
                FLOAT fFontSize{ 18.0f };
            };

            struct ScrollBarMetrics
            {
                BOOL bVisible{ FALSE };
                BOOL bHot{ FALSE };
                D2D1_RECT_F rcTrack{};
                D2D1_RECT_F rcThumb{};
                FLOAT fThumbTravel{};
                INT iViewportSize{};
                INT iContentSize{};
                INT iOffset{};
                INT iMaxOffset{};
            };

        private:
            HRESULT CreateDeviceIndependentResources() noexcept;
            HRESULT CreateDeviceResources() noexcept;
            HRESULT CreateTextFormatAndMetrics() noexcept;
            VOID UpdateViewportLayout() noexcept;
            VOID UpdateScrollBarMetrics(_Inout_ ScrollBarMetrics& scrollBarMetrics, _In_ BOOL bVertical) noexcept;
            VOID DrawScrollBars(_In_ COLORREF crDefaultBackground) noexcept;
            VOID DrawCells(_In_ const Buffer::Snapshot& sSnapshotBuffer) noexcept;
            VOID DrawCell(_In_ const Buffer::Cell& sCellCurrent, _In_ INT iCol, _In_ INT iRow,
                          _In_ const Buffer::Snapshot& sSnapshotBuffer) noexcept;

        private:
            HWND m_hWnd{};
            FLOAT m_fDpiX{ 96.0f };
            FLOAT m_fDpiY{ 96.0f };
            FLOAT m_fGridOffsetX{};
            FLOAT m_fGridOffsetY{};
            D2D1_RECT_F m_rcViewport{};
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
            Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat[4];
        };
    }
}
