#pragma once

#include <common/block.hpp>

namespace pgb::memory
{
using namespace pgb::common::block;

// GB has a 16-bit address space to map IO, ROM, & RAM
class MemoryMap
{
private:
    constexpr static std::size_t MaxBank         = 0x1FF;
    constexpr static std::size_t ByteAccessWidth = 0x8;

    Block<0x4000 * ByteAccessWidth, ByteAccessWidth>           _rom0;
    Block<0x4000 * ByteAccessWidth * MaxBank, ByteAccessWidth> _romX;
};

} // namespace pgb::memory