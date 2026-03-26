#pragma once

#include "GuiTerminalBuffer.h"

// -----------------------------------------------------------------------------

namespace GuiTerminal
{
    namespace Internals
    {
        class Parser
        {
        public:
            explicit Parser(_Inout_ Buffer &sBuffer, _In_opt_ RegionHandle hRegion) noexcept;
            Parser(const Parser&) = delete;
            Parser(Parser&&) = delete;
            ~Parser() noexcept = default;

            Parser& operator=(const Parser&) = delete;
            Parser& operator=(Parser&&) = delete;

            VOID Feed(_In_z_ LPCWSTR szTextW) noexcept;

        private:
            enum class State
            {
                Ground,
                Escape,
                Csi
            };

        private:
            VOID HandleGround(_In_ WCHAR chCodepointW) noexcept;
            VOID HandleEscape(_In_ WCHAR chCodepointW) noexcept;
            VOID HandleCsi(_In_ WCHAR chCodepointW) noexcept;

            VOID DispatchCsi(_In_ WCHAR chFinalCodepointW) noexcept;

            VOID PushCurrentNumber() noexcept;
            VOID ResetCsi() noexcept;

        private:
            Buffer &m_sBuffer;
            RegionHandle m_hRegion{ nullptr };

            State m_sState{ State::Ground };
            INT m_iParams[16]{};
            size_t m_cParams{ 0 };
            BOOL m_bHasCurrentNumber{ FALSE };
            INT m_iCurrentNumber{};
        };
    }
}

