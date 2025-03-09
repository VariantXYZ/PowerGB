#pragma once

#include <common/block.hpp>
#include <common/datatypes.hpp>

namespace pgb::memory
{
class MemoryMap;
}

namespace pgb::cpu
{

using namespace pgb::common::block;
using namespace pgb::common::datatypes;

class RegisterFile
{
private:
    // CPU registers

    //// Instruction Register
    Block<8, 8> _IR;

    //// Interrupt Enable
    Block<8, 8> _IE;

    //// Accumulator
    Block<8, 8> _A;

    //// Flag
    Block<8, 4> _F;

    //// General purpose
    Block<16, 8> _BC;
    Block<16, 8> _DE;
    Block<16, 8> _HL;

    // Program counter
    Block<16, 16> _PC;

    // Stack pointer
    Block<16, 16> _SP;

protected:
    friend memory::MemoryMap;

    // Flag reference
    constexpr Nibble& F() { return _F[0]; }

    // 8-bit reference
    constexpr Byte& IR() { return _IR[0]; }
    constexpr Byte& IE() { return _IE[0]; }
    constexpr Byte& A() { return _A[0]; }
    constexpr Byte& B() { return _BC[0]; }
    constexpr Byte& C() { return _BC[1]; }
    constexpr Byte& D() { return _DE[0]; }
    constexpr Byte& E() { return _DE[1]; }
    constexpr Byte& H() { return _HL[0]; }
    constexpr Byte& L() { return _HL[1]; }

    // 16-bit reference
    constexpr Word& PC() { return _PC[0]; }
    constexpr Word& SP() { return _SP[0]; }

public:
    // Flag read
    constexpr const Nibble& F() const { return _F[0]; }

    // 8-bit reads
    constexpr const Byte& IR() const { return _IR[0]; }
    constexpr const Byte& IE() const { return _IE[0]; }
    constexpr const Byte& A() const { return _A[0]; }
    constexpr const Byte& B() const { return _BC[0]; }
    constexpr const Byte& C() const { return _BC[1]; }
    constexpr const Byte& D() const { return _DE[0]; }
    constexpr const Byte& E() const { return _DE[1]; }
    constexpr const Byte& H() const { return _HL[0]; }
    constexpr const Byte& L() const { return _HL[1]; }

    // 16-bit reads
    constexpr const Word  AF() const { return Word(_A[0], Byte(_F[0], 0)); }
    constexpr const Word  BC() const { return _BC.Word<0>(); }
    constexpr const Word  DE() const { return _DE.Word<0>(); }
    constexpr const Word  HL() const { return _HL.Word<0>(); }
    constexpr const Word& PC() const { return _PC[0]; }
    constexpr const Word& SP() const { return _SP[0]; }
};

} // namespace pgb::cpu