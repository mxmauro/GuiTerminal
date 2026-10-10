#pragma once

#include "GuiTerminalBuffer.h"

// -----------------------------------------------------------------------------

namespace GuiTerminal {

namespace Internals {

/**
 * @brief Parses GuiTerminal's supported ANSI text sequences into Buffer operations.
 * @remarks This is a lower-level library contract. Applications normally use Control::Write or Control::Print instead.
 */
class Parser
{
  public:
    /** @brief Creates a parser that writes into a root or child region of a buffer. */
    explicit Parser(_Inout_ Buffer &sBuffer, _In_opt_ RegionHandle hRegion) noexcept;
    /** @brief Prevents copying parser state. */
    Parser(const Parser &) = delete;
    /** @brief Prevents moving parser state. */
    Parser(Parser &&) = delete;
    /** @brief Destroys the parser. */
    ~Parser() noexcept = default;

    /** @brief Prevents copy assignment. */
    Parser &operator=(const Parser &) = delete;
    /** @brief Prevents move assignment. */
    Parser &operator=(Parser &&) = delete;

    /** @brief Consumes null-terminated text and applies supported ANSI sequences to the selected region. */
    VOID Feed(_In_z_ LPCWSTR szTextW) noexcept;

  private:
    typedef enum class State_e {
        Ground,
        Escape,
        Csi
    } State;

  private:
    VOID HandleGround(_In_ WCHAR chCodepointW) noexcept;
    VOID HandleEscape(_In_ WCHAR chCodepointW) noexcept;
    VOID HandleCsi(_In_ WCHAR chCodepointW) noexcept;

    VOID DispatchCsi(_In_ WCHAR chFinalCodepointW) noexcept;

    VOID PushCurrentNumber() noexcept;
    VOID ResetCsi() noexcept;

  private:
    Buffer &m_sBuffer;
    RegionHandle m_hRegion{nullptr};

    State m_sState{State::Ground};
    INT m_iParams[16]{};
    size_t m_cParams{0};
    BOOL m_bHasCurrentNumber{FALSE};
    INT m_iCurrentNumber{};
};

} // namespace Internals

} // namespace GuiTerminal
