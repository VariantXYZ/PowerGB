#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>

using namespace pgb;
using namespace pgb::memory;

namespace pgb::memory
{

class MemoryMapTestSM83 : public MemoryMap
{
private:
    Block<0xFFFF * Byte::TypeWidth, Byte::TypeWidth> _rom;

public:
    constexpr MemoryMapTestSM83(cpu::RegisterFile& registers) noexcept : MemoryMap(registers)
    {
        MemoryMap::Initialize(MinRomBankCount, MinVramBankCount, MinWramBankCount, MinWramBankCount);
    }

    void Reset() noexcept override
    {
        MemoryMap::Reset();
        _rom.Reset();
    }

    AccessResultSet ReadByte(const MemoryAddress& addr) const noexcept override
    {
        return AccessResultSet::DefaultResultSuccess(_rom[addr.address]);
    }

    WriteAccessResultSet WriteByte(const MemoryAddress& addr, const Byte& value) noexcept override
    {
        auto oldValue      = _rom[addr.address];
        _rom[addr.address] = value;
        return WriteAccessResultSet::DefaultResultSuccess(oldValue);
    }

    WordAccessResultSet ReadWordLE(const MemoryAddress& addr) const noexcept override
    {
        return WordAccessResultSet::DefaultResultSuccess({_rom[addr.address] + 1, _rom[addr.address]});
    }

    WordAccessResultSet WriteWordLE(const MemoryAddress& addr, const Word& value) noexcept override
    {
        auto oldValue          = Word(_rom[addr.address + 1], _rom[addr.address]);
        _rom[addr.address]     = value.LowByte();
        _rom[addr.address + 1] = value.HighByte();
        return WordAccessResultSet::DefaultResultSuccess(oldValue);
    }

    AccessResultSet      ReadByte(const std::uint_fast16_t addr) const noexcept override { return ReadByte({0, addr}); }
    WriteAccessResultSet WriteByte(const std::uint_fast16_t addr, const Byte& value) noexcept override { return WriteByte({0, addr}, value); }
    WordAccessResultSet  ReadWordLE(const std::uint_fast16_t addr) const noexcept override { return ReadWordLE({0, addr}); }
    WordAccessResultSet  WriteWordLE(const std::uint_fast16_t addr, const Word& value) noexcept override { return WriteWordLE({0, addr}, value); }
};

} // namespace pgb::memory
