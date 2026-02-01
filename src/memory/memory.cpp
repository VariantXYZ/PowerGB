#include "cpu/include/registers.hpp"
#include <cstdint>
#include <memory/include/memory.hpp>

namespace pgb::memory
{

namespace
{
template <std::size_t N>
constexpr inline static bool IsValidRange(const std::size_t (&range)[N], std::size_t value)
{
    return !(std::find(std::begin(range), std::end(range), value) == std::end(range));
}

} // namespace

// TODO: CPU should be separate and manage register states, including IME
MemoryMap::InitializeResultSet MemoryMap::Initialize(const Byte (&rom)[], const std::size_t size) noexcept
{
    // TODO: Parse cartridge header
    InitializeResultSet result = Initialize(1, 1, 1, 1);
    if (result.IsFailure())
    {
        return result;
    }

    if ((size % RomBankSize) > 0)
    {
        return ResultInitializeInvalidAlignment(false);
    }

    std::size_t offset = 0;
    std::size_t bank   = 0;
    while (offset < size)
    {
        std::copy(&rom[offset], &rom[offset + RomBankSize], &_rom[bank++][0]);
        offset += RomBankSize;
    }

    return InitializeResultSet::DefaultResultSuccess();
}

MemoryMap::InitializeResultSet MemoryMap::Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept
{
    if (_isInitialized)
    {
        return ResultInitializeAlreadyInitialized(false);
    }

    if (!IsValidRange(ValidRomBankCount, romBankCount))
    {
        return ResultInitializeInvalidRomBankCount(false);
    }

    if (!IsValidRange(ValidVramBankCount, vramBankCount))
    {
        // Either 1 in GB or 2 in CGB
        return ResultInitializeInvalidVramBankCount(false);
    }

    if (!IsValidRange(ValidExternalRamBankCount, eramBankCount))
    {
        return ResultInitializeInvalidEramBankCount(false);
    }

    if (!IsValidRange(ValidWramBankCount, wramBankCount))
    {
        // Either 2 in GB or 8 in CGB
        return ResultInitializeInvalidWramBankCount(false);
    }

    _romBankCount  = romBankCount;
    _vramBankCount = vramBankCount;
    _eramBankCount = eramBankCount;
    _wramBankCount = wramBankCount;

    _isInitialized = true;
    return InitializeResultSet::DefaultResultSuccess();
}

void MemoryMap::Reset() noexcept
{
    // Reset all memory regions
    for (auto& b : _rom)
    {
        b.Reset();
    }

    for (auto& b : _vram)
    {
        b.Reset();
    }

    for (auto& b : _eram)
    {
        b.Reset();
    }

    for (auto& b : _wram)
    {
        b.Reset();
    }

    _oam.Reset();
    _hram.Reset();
    _io.Reset();
    _registers.Reset();

    _isInitialized = false;
}

MemoryMap::BankSetResultSet MemoryMap::SetRomBank(std::uint_fast16_t bank) noexcept
{
    if (bank >= _romBankCount)
    {
        return ResultAccessInvalidBank(false);
    }

    _romBankSelect = bank;

    return BankSetResultSet::DefaultResultSuccess();
}

MemoryMap::BankSetResultSet MemoryMap::SetEramBank(std::uint_fast16_t bank) noexcept
{
    if (bank >= _eramBankCount)
    {
        return ResultAccessInvalidBank(false);
    }

    _eramBankSelect = bank;

    return BankSetResultSet::DefaultResultSuccess();
}

MemoryMap::AccessResultSet MemoryMap::ReadByte(const MemoryAddress& maddr, bool useCurrentBank) const noexcept
{
    std::uint_fast16_t address = maddr.address;

    if (address <= 0x3FFF)
    {
        return AccessResultSet::DefaultResultSuccess(_rom[0][address]);
    }
    else if (address <= 0x7FFF)
    {
        std::uint_fast16_t bank = useCurrentBank ? GetRomBank() : maddr.bank;
        return (bank >= _romBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_rom[bank][address - 0x4000]);
    }
    else if (address <= 0x9FFF)
    {
        std::uint_fast16_t bank = useCurrentBank ? _io[0x4F].data : maddr.bank;
        return (bank >= _vramBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_vram[bank][address - 0x8000]);
    }
    else if (address <= 0xBFFF)
    {
        std::uint_fast16_t bank = useCurrentBank ? GetEramBank() : maddr.bank;
        return bank >= _eramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_eram[bank][address - 0xA000]);
    }
    else if (address <= 0xCFFF)
    {
        return AccessResultSet::DefaultResultSuccess(_wram[0][address - 0xC000]);
    }
    else if (address <= 0xDFFF)
    {
        std::uint_fast16_t bank = useCurrentBank ? _io[0x70].data : maddr.bank;
        return bank >= _wramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_wram[bank][address - 0xD000]);
    }
    else if (address <= 0xFDFF)
    {
        std::uint_fast16_t bank = useCurrentBank ? _io[0x70].data : maddr.bank;
        // Echo RAM
        // From pandocs: "The range E000-FDFF is mapped to WRAM, but only the lower 13 bits of the address lines are connected"
        // Return a prohibited address warning, but otherwise the value is still returned
        address -= 0x2000;
        if (bank >= _wramBankCount)
        {
            return AccessResultSet(ResultAccessInvalidBank(false), 0);
        }

        auto value = ReadByte({bank, address});
        if (value.IsSuccess())
        {
            return AccessResultSet(ResultAccessProhibitedAddress(true), value);
        }
        else
        {
            return AccessResultSet(ResultAccessProhibitedAddress(false), value);
        }
    }
    else if (address <= 0xFE9F)
    {
        return AccessResultSet::DefaultResultSuccess(_oam[address - 0xFE00]);
    }
    else if (address <= 0xFEFF)
    {
        // From pandocs: "Use of this area is prohibited. This area returns $FF when OAM is blocked, and otherwise the behavior depends on the hardware revision."
        // On DMG, MGB, SGB, and SGB2, reads during OAM block trigger OAM corruption. Reads otherwise return $00.
        // On CGB revisions 0-D, this area is a unique RAM area, but is masked with a revision-specific value.
        // On CGB revision E, AGB, AGS, and GBP, it returns the high nibble of the lower address byte twice, e.g. FFAx returns $AA, FFBx returns $BB, and so forth.

        // For ease of implementation, just use CGB Revision E that returns the high nibble of the lower address byte twice
        // TODO: This area should be treated similarly to IO registers with a special handler that relies on OAM state
        return AccessResultSet(ResultAccessReadOnlyProhibitedAddress(true), _FEA0_FEFF[((address & 0x00F0) >> 4) - 0xA]);
    }
    else if (address <= 0xFF7F)
    {
        // IO registers are controlled via the CPU layer, but all have some memory representation underneath
        return AccessResultSet::DefaultResultSuccess(_io[address - 0xFF00]);
    }
    else if (address <= 0xFFFE)
    {
        return AccessResultSet::DefaultResultSuccess(_hram[address - 0xFF80]);
    }
    else if (address == 0xFFFF)
    {
        return AccessResultSet::DefaultResultSuccess(_registers.IE());
    }
    else
    {
        return AccessResultSet(ResultAccessInvalidAddress(false), 0);
    }
}

MemoryMap::AccessResultSet MemoryMap::ReadByte(const MemoryAddress& maddr) const noexcept { return ReadByte(maddr, false); }
MemoryMap::AccessResultSet MemoryMap::ReadByte(const std::uint_fast16_t address) const noexcept { return ReadByte({0, address}, true); }

MemoryMap::WriteAccessResultSet MemoryMap::WriteByte(const MemoryAddress& maddr, const Byte& value, bool useCurrentBank) noexcept
{
    AccessResultSet accessResult = ReadByte(maddr, useCurrentBank);
    if (accessResult.IsFailure())
    {
        return accessResult;
    }

    if (accessResult.IsResult<ResultAccessReadOnlyProhibitedAddress>())
    {
        return WriteAccessResultSet(ResultAccessReadOnlyProhibitedAddress(false), static_cast<const Byte&>(accessResult));
    }

    auto& valueRef = const_cast<Byte&>(static_cast<const Byte&>(accessResult));
    auto  oldValue = valueRef;
    valueRef       = value;

    // Make sure we propagate prohibited access issues up
    if (accessResult.IsResult<ResultAccessProhibitedAddress>())
    {
        return WriteAccessResultSet(ResultAccessProhibitedAddress(true), oldValue);
    }
    else
    {
        return WriteAccessResultSet::DefaultResultSuccess(oldValue);
    }
}

MemoryMap::WriteAccessResultSet MemoryMap::WriteByte(const MemoryAddress& maddr, const Byte& value) noexcept { return WriteByte(maddr, value, false); }
MemoryMap::WriteAccessResultSet MemoryMap::WriteByte(const std::uint_fast16_t address, const Byte& value) noexcept { return WriteByte({0, address}, value, true); }

MemoryMap::WordAccessResultSet MemoryMap::ReadWordLE(const MemoryAddress& maddr, bool useCurrentBank) const noexcept
{
    uint_fast16_t bank    = maddr.bank;
    uint_fast16_t address = maddr.address;

    if (address == 0xFFFF)
    {
        return WordAccessResultSet(ResultAccessInvalidAddress(false), 0);
    }

    auto low = ReadByte(maddr);
    if (low.IsFailure())
    {

        return static_cast<WordAccessResultSet>(low);
    }

    auto high = ReadByte({bank, ++address}, useCurrentBank);
    if (high.IsFailure())
    {
        return static_cast<WordAccessResultSet>(high);
    }

    Word w(high, low);

    if (!IsValidRange({
                          RomXBoundary,
                          VramBoundary,
                          EramBoundary,
                          WramXBoundary,
                          OamBoundary,
                          IOBoundary,
                          HramBoundary,
                      },
                      address))
    {
        return WordAccessResultSet(ResultAccessCrossesRegionBoundary(true), w);
    }

    return WordAccessResultSet::DefaultResultSuccess(w);
}
MemoryMap::WordAccessResultSet MemoryMap::ReadWordLE(const MemoryAddress& maddr) const noexcept { return ReadWordLE(maddr, false); }
MemoryMap::WordAccessResultSet MemoryMap::ReadWordLE(const std::uint_fast16_t address) const noexcept { return ReadWordLE({0, address}, true); }

MemoryMap::WordAccessResultSet MemoryMap::WriteWordLE(const MemoryAddress& maddr, const Word& value, bool useCurrentBank) noexcept
{
    uint_fast16_t bank    = maddr.bank;
    uint_fast16_t address = maddr.address;

    if (address == 0xFFFF)
    {
        return WordAccessResultSet(ResultAccessInvalidAddress(false), 0);
    }

    auto low = WriteByte(maddr, value.LowByte());
    if (low.IsFailure())
    {
        return static_cast<WordAccessResultSet>(low);
    }

    auto high = WriteByte({bank, ++address}, value.HighByte(), useCurrentBank);
    if (high.IsFailure())
    {
        return static_cast<WordAccessResultSet>(high);
    }

    Word w(high, low);

    if (!IsValidRange({
                          RomXBoundary,
                          VramBoundary,
                          EramBoundary,
                          WramXBoundary,
                          OamBoundary,
                          IOBoundary,
                          HramBoundary,
                      },
                      address))
    {
        return WordAccessResultSet(ResultAccessCrossesRegionBoundary(true), w);
    }

    return WordAccessResultSet::DefaultResultSuccess(w);
}

MemoryMap::WordAccessResultSet MemoryMap::WriteWordLE(const MemoryAddress& maddr, const Word& value) noexcept { return WriteWordLE(maddr, value, false); }
MemoryMap::WordAccessResultSet MemoryMap::WriteWordLE(const std::uint_fast16_t address, const Word& value) noexcept { return WriteWordLE({0, address}, value, true); }

MemoryMap::Register8AccessResultSet MemoryMap::ReadByte(const cpu::RegisterType& type) const noexcept
{
    switch (type)
    {
    case cpu::RegisterType::A:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.A());
    case cpu::RegisterType::B:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.B());
    case cpu::RegisterType::C:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.C());
    case cpu::RegisterType::D:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.D());
    case cpu::RegisterType::E:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.E());
    case cpu::RegisterType::H:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.H());
    case cpu::RegisterType::L:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.L());
    case cpu::RegisterType::IE:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.IE());
    case cpu::RegisterType::IR:
        return Register8AccessResultSet::DefaultResultSuccess(_registers.IR());
    default:
        return Register8AccessResultSet(ResultAccessRegisterInvalidWidth(false), 0);
    }
}

MemoryMap::Register8AccessResultSet MemoryMap::WriteByte(const cpu::RegisterType& type, const Byte& value) noexcept
{
    switch (type)
    {
    case cpu::RegisterType::A:
    {
        auto oldValue  = _registers.A();
        _registers.A() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::B:
    {
        auto oldValue  = _registers.B();
        _registers.B() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::C:
    {
        auto oldValue  = _registers.C();
        _registers.C() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::D:
    {
        auto oldValue  = _registers.D();
        _registers.D() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::E:
    {
        auto oldValue  = _registers.E();
        _registers.E() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::H:
    {
        auto oldValue  = _registers.H();
        _registers.H() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::L:
    {
        auto oldValue  = _registers.L();
        _registers.L() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::IE:
    {
        auto oldValue   = _registers.IE();
        _registers.IE() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::IR:
    {
        auto oldValue   = _registers.IR();
        _registers.IR() = value;
        return Register8AccessResultSet::DefaultResultSuccess(oldValue);
    }
    default:
        return Register8AccessResultSet(ResultAccessRegisterInvalidWidth(false), 0);
    }
}

// Read byte stored in 16-bit register, the stored result is a value and only valid if it is marked successful.
// Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
MemoryMap::Register16AccessResultSet MemoryMap::ReadWord(const cpu::RegisterType& type) const noexcept
{
    switch (type)
    {
    case cpu::RegisterType::AF:
        return Register16AccessResultSet::DefaultResultSuccess(_registers.AF());
    case cpu::RegisterType::BC:
        return Register16AccessResultSet::DefaultResultSuccess(_registers.BC());
    case cpu::RegisterType::DE:
        return Register16AccessResultSet::DefaultResultSuccess(_registers.DE());
    case cpu::RegisterType::HL:
        return Register16AccessResultSet::DefaultResultSuccess(_registers.HL());
    case cpu::RegisterType::SP:
        return Register16AccessResultSet::DefaultResultSuccess(_registers.SP());
    case cpu::RegisterType::PC:
        return Register16AccessResultSet::DefaultResultSuccess(_registers.PC());
    default:
        return Register16AccessResultSet(ResultAccessRegisterInvalidWidth(false), 0);
    }
}

MemoryMap::Register16AccessResultSet MemoryMap::WriteWord(const cpu::RegisterType& type, const Word& value) noexcept
{
    switch (type)
    {
    case cpu::RegisterType::AF:
    {
        auto oldValue  = _registers.AF();
        _registers.A() = value.HighByte();
        _registers.F() = value.LowByte().HighNibble();
        return Register16AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::BC:
    {
        auto oldValue  = _registers.BC();
        _registers.B() = value.HighByte();
        _registers.C() = value.LowByte();
        return Register16AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::DE:
    {
        auto oldValue  = _registers.DE();
        _registers.D() = value.HighByte();
        _registers.E() = value.LowByte();
        return Register16AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::HL:
    {
        auto oldValue  = _registers.HL();
        _registers.H() = value.HighByte();
        _registers.L() = value.LowByte();
        return Register16AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::SP:
    {
        auto oldValue   = _registers.SP();
        _registers.SP() = value;
        return Register16AccessResultSet::DefaultResultSuccess(oldValue);
    }
    case cpu::RegisterType::PC:
    {
        auto oldValue   = _registers.PC();
        _registers.PC() = value;
        return Register16AccessResultSet::DefaultResultSuccess(oldValue);
    }
    default:
        return Register16AccessResultSet(ResultAccessRegisterInvalidWidth(false), 0);
    }
}

const Nibble& MemoryMap::ReadFlag() const noexcept
{
    return _registers.F();
}

Nibble MemoryMap::WriteFlag(const Nibble& value) noexcept
{
    auto oldValue  = _registers.F();
    _registers.F() = value;
    return oldValue;
}

const Word& MemoryMap::ReadPC() const noexcept
{
    return _registers.PC();
}

const Byte& MemoryMap::ReadIR() const noexcept
{
    return _registers.IR();
}

MemoryMap::ModifyStateRegisterResultSet MemoryMap::IncrementPC() noexcept
{
    ++_registers.PC();
    if (_registers.PC() > 0x7FFF)
    {
        // We still return a 'success', but just note that the result is an overflow
        return ModifyStateRegisterResultSet(ResultRegisterOverflow(true), _registers.PC());
    }
    return ModifyStateRegisterResultSet::DefaultResultSuccess(_registers.PC());
}

MemoryMap::ModifyStateRegisterResultSet MemoryMap::DecrementPC() noexcept
{
    --_registers.PC();
    if (_registers.PC() > 0x7FFF)
    {
        // We still return a 'success', but just note that the result is an overflow
        return ModifyStateRegisterResultSet(ResultRegisterOverflow(true), _registers.PC());
    }
    return ModifyStateRegisterResultSet::DefaultResultSuccess(_registers.PC());
}

} // namespace pgb::memory
