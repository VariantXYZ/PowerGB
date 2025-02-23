#pragma once

#include "cpu_register.hpp"
#include <cstdint>

namespace pgb::cpu
{

class CPU
{
protected:
    GBCPURegister16<true, true, true, true>     _af;
    GBCPURegister16<true, true, true, true>     _bc;
    GBCPURegister16<true, true, true, true>     _de;
    GBCPURegister16<true, true, true, true>     _hl;
    GBCPURegister16<false, false, false, false> _sp;
    GBCPURegister16<false, false, false, false> _pc;

public:
    // CPU Register Access
    // Generally straightforward with the caveat that all accesses to the lower 4 bits of F (and, consequently, AF) are 0, so we handle these separately

    // 16-bit general purpose
    uint16_t& BC = _bc;
    uint16_t& DE = _de;
    uint16_t& HL = _hl;

    // 16-bit state management
    uint16_t& SP = _sp;
    uint16_t& PC = _pc;

    // 8-bit
    uint8_t& A   = _af.Hi();
    uint8_t& B   = _bc.Hi();
    uint8_t& C   = _bc.Lo();
    uint8_t& D   = _de.Hi();
    uint8_t& E   = _de.Lo();
    uint8_t& H   = _hl.Hi();
    uint8_t& L   = _hl.Lo();

    // Flag manipulation
    void               FlagReset() { _af.Lo() = 0; }
    void               FlagSetZ() { _af.Lo() = _af.Lo() | (1 << 7); }
    void               FlagSetN() { _af.Lo() = _af.Lo() | (1 << 6); }
    void               FlagSetH() { _af.Lo() = _af.Lo() | (1 << 5); }
    void               FlagSetC() { _af.Lo() = _af.Lo() | (1 << 4); }
    [[nodiscard]] bool FlagZ() const { return ((_af.Lo() & (1 << 7)) != 0); } // Zero
    [[nodiscard]] bool FlagN() const { return ((_af.Lo() & (1 << 6)) != 0); } // Subtraction
    [[nodiscard]] bool FlagH() const { return ((_af.Lo() & (1 << 5)) != 0); } // Half-Carry
    [[nodiscard]] bool FlagC() const { return ((_af.Lo() & (1 << 4)) != 0); } // Carry
};

} // namespace pgb::cpu