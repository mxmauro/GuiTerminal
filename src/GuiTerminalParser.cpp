#include "..\include\GuiTerminalParser.h"

// -----------------------------------------------------------------------------

static BOOL IsC0Control(_In_ WCHAR chCodepointW) noexcept;

// -----------------------------------------------------------------------------

namespace GuiTerminal::Internals
{
    Parser::Parser(_Inout_ Buffer &sBuffer, _In_opt_ RegionHandle hRegion) noexcept : m_sBuffer(sBuffer), m_hRegion(hRegion)
    {
    };

    VOID Parser::Feed(_In_z_ LPCWSTR szTextW) noexcept
    {
        while (*szTextW != L'\0')
        {
            if (m_sState == State::Ground)
            {
                HandleGround(*szTextW);
            }
            else if (m_sState == State::Escape)
            {
                HandleEscape(*szTextW);
            }
            else
            {
                HandleCsi(*szTextW);
            }
            szTextW += 1;
        }
    }

    VOID Parser::HandleGround(_In_ WCHAR chCodepointW) noexcept
    {
        if (chCodepointW == 0x1BU)
        {
            m_sState = State::Escape;
        }
        else if (IsC0Control(chCodepointW))
        {
            m_sBuffer.ProcessControl(m_hRegion, chCodepointW);
        }
        else
        {
            m_sBuffer.PutCodepoint(m_hRegion, chCodepointW);
        }
    }

    VOID Parser::HandleEscape(_In_ WCHAR chCodepointW) noexcept
    {
        if (chCodepointW == L'[')
        {
            ResetCsi();
            m_sState = State::Csi;
            return;
        }
        if (chCodepointW == L'7')
        {
            m_sBuffer.SaveCursor(m_hRegion);
        }
        else if (chCodepointW == L'8')
        {
            m_sBuffer.RestoreCursor(m_hRegion);
        }
        m_sState = State::Ground;
    }

    VOID Parser::HandleCsi(_In_ WCHAR chCodepointW) noexcept
    {
        if (chCodepointW >= L'0' && chCodepointW <= L'9')
        {
            m_bHasCurrentNumber = TRUE;
            m_iCurrentNumber = (m_iCurrentNumber * 10) + static_cast<INT>(chCodepointW - L'0');
            return;
        }
        if (chCodepointW == L';')
        {
            PushCurrentNumber();
            return;
        }
        if (chCodepointW >= 0x40 && chCodepointW <= 0x7E)
        {
            PushCurrentNumber();
            DispatchCsi(chCodepointW);
            m_sState = State::Ground;
            return;
        }
        ResetCsi();
        m_sState = State::Ground;
    }

    VOID Parser::DispatchCsi(_In_ WCHAR chFinalCodepointW) noexcept
    {
        INT iFirst;
        INT iSecond;

        iFirst = 1;
        iSecond = 1;
        if (m_cParams > 0)
        {
            iFirst = (m_iParams[0] == 0) ? 1 : m_iParams[0];
        }
        if (m_cParams > 1)
        {
            iSecond = (m_iParams[1] == 0) ? 1 : m_iParams[1];
        }

        switch (chFinalCodepointW)
        {
            case L'A':
                m_sBuffer.MoveCursorRelative(m_hRegion, 0, -iFirst);
                break;
            case L'B':
                m_sBuffer.MoveCursorRelative(m_hRegion, 0, iFirst);
                break;
            case L'C':
                m_sBuffer.MoveCursorRelative(m_hRegion, iFirst, 0);
                break;
            case L'D':
                m_sBuffer.MoveCursorRelative(m_hRegion, -iFirst, 0);
                break;
            case L'G':
                m_sBuffer.SetCursorColumn(m_hRegion, iFirst);
                break;
            case L'd':
                m_sBuffer.SetCursorRow(m_hRegion, iFirst);
                break;
            case L'H':
            case L'f':
                m_sBuffer.SetCursorPosition(m_hRegion, iFirst, iSecond);
                break;
            case L'J':
                m_sBuffer.EraseInDisplay(m_hRegion, (m_cParams > 0) ? m_iParams[0] : 0);
                break;
            case L'K':
                m_sBuffer.EraseInLine(m_hRegion, (m_cParams > 0) ? m_iParams[0] : 0);
                break;
            case L'm':
                m_sBuffer.SetGraphicsRendition(m_hRegion, m_iParams, m_cParams);
                break;
            case L'S':
                m_sBuffer.ScrollRegion(m_hRegion, iFirst);
                break;
            case L'T':
                m_sBuffer.ScrollRegion(m_hRegion, -iFirst);
                break;
            case L's':
                m_sBuffer.SaveCursor(m_hRegion);
                break;
            case L'u':
                m_sBuffer.RestoreCursor(m_hRegion);
                break;
        }
        ResetCsi();
    }

    VOID Parser::PushCurrentNumber() noexcept
    {
        if (m_cParams < (sizeof(m_iParams) / sizeof(m_iParams[0])))
        {
            m_iParams[m_cParams] = (m_bHasCurrentNumber != FALSE) ? m_iCurrentNumber : 0;
            m_cParams += 1;
        }
        m_bHasCurrentNumber = FALSE;
        m_iCurrentNumber = 0;
    }

    VOID Parser::ResetCsi() noexcept
    {
        m_cParams = 0;
        m_bHasCurrentNumber = FALSE;
        m_iCurrentNumber = 0;
    }
}

// -----------------------------------------------------------------------------

static BOOL IsC0Control(_In_ WCHAR chCodepointW) noexcept
{
    return ((chCodepointW < 0x20U) || (chCodepointW == 0x7FU)) ? TRUE : FALSE;
}
