#pragma once

#include <common/block.hpp>

namespace pgb::cpu
{

using namespace pgb::common::block;

class CPU
{
private:
    // CPU registers
    Block<8, 8>   _A;
    Block<8, 4>   _F;
    Block<16, 8>  _BC;
    Block<16, 8>  _DE;
    Block<16, 8>  _HL;
    Block<16, 16> _SP;
    Block<16, 16> _PC;

public:
};

} // namespace pgb::cpu