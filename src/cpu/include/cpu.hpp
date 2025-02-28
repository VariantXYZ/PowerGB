#pragma once

#include <common/block.hpp>

namespace pgb::cpu
{

class CPU
{
private:
    // CPU registers
    pgb::common::block::Block<8, 8>   _A;
    pgb::common::block::Block<8, 4>   _F;
    pgb::common::block::Block<16, 8>  _BC;
    pgb::common::block::Block<16, 8>  _DE;
    pgb::common::block::Block<16, 8>  _HL;
    pgb::common::block::Block<16, 16> _SP;
    pgb::common::block::Block<16, 16> _PC;

public:
};

} // namespace pgb::cpu