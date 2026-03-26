#pragma once

#include "GuiTerminalBuffer.h"
#include <d2d1.h>
#include <dwrite.h>
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

            HRESULT Initialize(_In_ HWND hWnd) noexcept;
            HRESULT Resize(_In_ UINT uiWidth, _In_ UINT uiHeight) noexcept;
            HRESULT Render(_In_ const Buffer& bufferGuiTerminal) noexcept;
            HRESULT GetPreferredClientSize(_In_ INT iCols, _In_ INT iRows, _Out_ LPSIZE lpSize) const noexcept;
            VOID RefreshDpi() noexcept;

        private:
            struct FontMetrics
            {
                FLOAT fCellWidth{};
                FLOAT fCellHeight{};
                FLOAT fBaseline{};
                FLOAT fUnderlineOffset{};
                FLOAT fUnderlineThickness{};
                LPCWSTR szFontFamilyW{ nullptr };
                FLOAT fFontSize{ 18.0f };
            };

        private:
            HRESULT CreateDeviceIndependentResources() noexcept;
            HRESULT CreateDeviceResources() noexcept;
            HRESULT CreateTextFormatAndMetrics() noexcept;
            HRESULT ChooseFontFamily(_Out_ LPCWSTR *lpszFontFamilyW) noexcept;
            VOID DrawCells(_In_ const Buffer::Snapshot& sSnapshotBuffer) noexcept;
            VOID DrawCell(_In_ const Buffer::Cell& sCellCurrent, _In_ INT iCol, _In_ INT iRow,
                          _In_ const Buffer::Snapshot& sSnapshotBuffer) noexcept;

        private:
            HWND m_hWnd{};
            FLOAT m_fDpiX{ 96.0f };
            FLOAT m_fDpiY{ 96.0f };
            FontMetrics m_metricsFont;
            Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
            Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
            Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
            Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat[4];
        };
    }
}

