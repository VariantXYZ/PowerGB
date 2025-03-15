#pragma once

#include <bit>
#include <cstdint>

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>
#include <cpu/include/registers.hpp>
#include <type_traits>

using namespace pgb::common::block;
using namespace pgb::common::datatypes;

namespace pgb::memory
{

constexpr static const std::size_t ValidRomBankCount[]         = {2, 4, 8, 16, 32, 64, 128, 256, 512, 72, 80, 96};
constexpr static const std::size_t ValidVramBankCount[]        = {1, 2};
constexpr static const std::size_t ValidExternalRamBankCount[] = {0, 1, 4, 16, 8};
constexpr static const std::size_t ValidWramBankCount[]        = {2, 8};

// Pre-emptively statically allocate all necessary buffers
constexpr static std::size_t MaxBankValue                      = 0x1FF;
constexpr static std::size_t MaxAddressValue                   = 0xFFFF;

constexpr static std::size_t MaxRomBankCount                   = 0x200;
constexpr static std::size_t MaxVramBankCount                  = 2;
constexpr static std::size_t MaxEramBankCount                  = 16;
constexpr static std::size_t MaxWramBankCount                  = 8;

constexpr static std::size_t RomBankSize                       = 0x4000;
constexpr static std::size_t VramBankSize                      = 0x2000;
constexpr static std::size_t EramBankSize                      = 0x2000;
constexpr static std::size_t WramBankSize                      = 0x1000;

// GB has a 16-bit address space to map IO, ROM, & RAM
// This class provides an interface to the raw underlying memory representation of this address space.
// Note that this class is generally too large to be allocated on the stack.
class MemoryMap
{
public:
    // Function results
    //// Access
    using ResultAccessInvalidBank               = common::Result<"Bank not in valid range">;
    using ResultAccessInvalidAddress            = common::Result<"Address not in valid range">;
    using ResultAccessProhibitedAddress         = common::Result<"Accessing prohibited address">;
    using ResultAccessReadOnlyProhibitedAddress = common::Result<"Accessing read-only prohibited address">;
    using ResultAccessCrossesRegionBoundary     = common::Result<"Access width would result in crossing region boundaries">;

    template <typename T>
    using BaseAccessResultSet =
        common::ResultSet<
            /* Type */ T,
            common::ResultSuccess,
            ResultAccessInvalidBank,
            ResultAccessInvalidAddress,
            ResultAccessProhibitedAddress,
            ResultAccessReadOnlyProhibitedAddress,
            ResultAccessCrossesRegionBoundary>;

    using AccessResultSet                  = BaseAccessResultSet<const Byte&>;
    using WriteAccessResultSet             = BaseAccessResultSet<const Byte>;
    using WordAccessResultSet              = BaseAccessResultSet<const Word>;

    using ResultAccessRegisterInvalidWidth = common::Result<"Register access does not match register width">;
    template <typename T>
    using BaseRegisterAccessResultSet =
        common::ResultSet<
            /* Type */ T,
            common::ResultSuccess,
            ResultAccessRegisterInvalidWidth>;

    using Register8AccessResultSet             = BaseRegisterAccessResultSet<const Byte>;
    using Register16AccessResultSet            = BaseRegisterAccessResultSet<const Word>;

    //// Initialization
    using ResultInitializeInvalidAlignment     = common::Result<"ROM size is not a multiple of 0x4000">;
    using ResultInitializeAlreadyInitialized   = common::Result<"Memory has already been initialized">;
    using ResultInitializeInvalidRomBankCount  = common::Result<"ROM bank count is invalid">;
    using ResultInitializeInvalidVramBankCount = common::Result<"VRAM bank count is invalid">;
    using ResultInitializeInvalidEramBankCount = common::Result<"ERAM bank count is invalid">;
    using ResultInitializeInvalidWramBankCount = common::Result<"WRAM bank count is invalid">;
    using InitializeResultSet =
        common::ResultSet<
            /* Type */ void,
            common::ResultSuccess,
            ResultInitializeInvalidAlignment,
            ResultInitializeAlreadyInitialized,
            ResultInitializeInvalidRomBankCount,
            ResultInitializeInvalidVramBankCount,
            ResultInitializeInvalidEramBankCount,
            ResultInitializeInvalidWramBankCount>;

private:
    // The bus maps references to the CPU IE
    cpu::RegisterFile& _registers;

    std::size_t _romBankCount;
    std::size_t _vramBankCount;
    std::size_t _eramBankCount;
    std::size_t _wramBankCount;

    // IO Registers should be handled as individual registers
    // We could be more memory efficient, but doing this allows for statically allocating this entire class
    Block<RomBankSize * Byte::TypeWidth, Byte::TypeWidth>  _rom[MaxRomBankCount];
    Block<VramBankSize * Byte::TypeWidth, Byte::TypeWidth> _vram[MaxVramBankCount];
    Block<EramBankSize * Byte::TypeWidth, Byte::TypeWidth> _eram[MaxEramBankCount];
    Block<WramBankSize * Byte::TypeWidth, Byte::TypeWidth> _wram[MaxWramBankCount];
    Block<0xA0 * Byte::TypeWidth, Byte::TypeWidth>         _oam;
    Block<0x80 * Byte::TypeWidth, Byte::TypeWidth>         _hram;
    Block<0x80 * Byte::TypeWidth, Byte::TypeWidth>         _io;

    bool _isInitialized                      = false;

    constexpr static const Byte _FEA0_FEFF[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

public:
    struct MemoryAddress
    {
        uint_fast16_t bank : std::bit_width(static_cast<uint_fast16_t>(MaxBankValue));
        uint_fast16_t address : std::bit_width(static_cast<uint_fast16_t>(MaxAddressValue));
    };

    constexpr MemoryMap(cpu::RegisterFile& registers) noexcept
        : _registers(registers) {}

    constexpr MemoryMap(
        cpu::RegisterFile& registers,
        std::size_t        romBankCount,
        std::size_t        vramBankCount,
        std::size_t        eramBankCount,
        std::size_t        wramBankCount
    )
        : _registers(registers),
          _romBankCount(romBankCount),
          _vramBankCount(vramBankCount),
          _eramBankCount(eramBankCount),
          _wramBankCount(wramBankCount),
          _isInitialized(true)
    {
    }

    // Initialize memory based on an input ROM, will handle parsing the cartridge header to pull necessary metadata
    // All non-default results (anything that is not ResultSuccess) are considered failure cases and will not initialize the map
    constexpr InitializeResultSet Initialize(const Byte (&rom)[], const std::size_t size) noexcept;
    constexpr InitializeResultSet Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept;

    // Sets all memory regions to 0
    constexpr void Reset() noexcept;

    constexpr bool IsInitialized() { return _isInitialized; }

    // Access a byte at a specific address, the stored result is a reference and is only valid if it is marked successful.
    // ResultAccessInvalidBank is always a failure case.
    // ResultAccessInvalidAddress is always a failure case.
    // ResultAccessProhibitedAddress is sometimes returned as a failure case.
    // ResultAccessReadOnlyProhibitedAddress is never a failure case.
    constexpr AccessResultSet ReadByte(const MemoryAddress&) const noexcept;

    // Write a byte at a specific address if it is accessible and returns the previous value.
    // If the address is not accessible, the function will propagate the AccessByte error.
    // If access returns ResultAccessReadOnlyProhibitedAddress, the result of this function is a failure and the read value is in the result.
    // Note that this function will write as long as the address is within a valid range and the address is not ReadOnlyProhibited.
    constexpr WriteAccessResultSet WriteByte(const MemoryAddress&, const Byte& value) noexcept;

    // Access a word at a specific address, the stored result is a value and only valid if it is marked successful.
    // Treats the value in memory as being stored as little endian, so a byteswap will happen prior to returning.
    // The result behavior is the same as ReadByte except it can also return ResultAccessCrossesRegionBoundary which will never be a failure (consider it a warning).
    constexpr WordAccessResultSet ReadWordLE(const MemoryAddress&) const noexcept;

    // Write a word at a specific address if it is accessible and returns the previous value.
    // Treats the value in memory as being stored as little endian, so a byteswap will happen prior to storing.
    // The result behavior is the same as WriteByte, except ReadWordLE is used instead of ReadByte.
    constexpr WordAccessResultSet WriteWordLE(const MemoryAddress&, const Word& value) noexcept;

    // Read byte stored in 8-bit register, the stored result is a value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    constexpr Register8AccessResultSet ReadByte(const cpu::RegisterType&) const noexcept;

    // Write byte into 8-bit register, the stored result is the previous value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    constexpr Register8AccessResultSet WriteByte(const cpu::RegisterType&, const Byte&) noexcept;

    // Read byte stored in 16-bit register, the stored result is a value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    constexpr Register16AccessResultSet ReadWord(const cpu::RegisterType&) const noexcept;

    // Write byte into 8-bit register, the stored result is the previous value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    constexpr Register16AccessResultSet WriteWord(const cpu::RegisterType&, const Word&) noexcept;

    constexpr const Nibble& ReadFlag() const noexcept;
    constexpr Nibble        WriteFlag(const Nibble&) noexcept;
};

namespace
{
template <std::size_t N>
constexpr static bool IsValidRange(const std::size_t (&range)[N], std::size_t value)
{
    return !(std::find(std::begin(range), std::end(range), value) == std::end(range));
}

} // namespace

constexpr MemoryMap::InitializeResultSet MemoryMap::Initialize(const Byte (&rom)[], const std::size_t size) noexcept
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

constexpr MemoryMap::InitializeResultSet MemoryMap::Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept
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

constexpr void MemoryMap::Reset() noexcept
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

    _isInitialized = false;
}

constexpr MemoryMap::AccessResultSet MemoryMap::ReadByte(const MemoryAddress& maddr) const noexcept
{
    auto bank    = maddr.bank;
    auto address = maddr.address;

    if (address <= 0x3FFF)
    {
        return AccessResultSet::DefaultResultSuccess(_rom[0][address]);
    }
    else if (address <= 0x7FFF)
    {
        return (bank >= _romBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_rom[bank][address - 0x4000]);
    }
    else if (address <= 0x9FFF)
    {
        return (bank >= _vramBankCount) ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_vram[bank][address - 0x8000]);
    }
    else if (address <= 0xBFFF)
    {
        return bank >= _eramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_eram[bank][address - 0xA000]);
    }
    else if (address <= 0xCFFF)
    {
        return AccessResultSet::DefaultResultSuccess(_wram[0][address - 0xC000]);
    }
    else if (address <= 0xDFFF)
    {
        return bank >= _wramBankCount ? AccessResultSet(ResultAccessInvalidBank(false), 0) : AccessResultSet::DefaultResultSuccess(_wram[bank][address - 0xD000]);
    }
    else if (address <= 0xFDFF)
    {
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

constexpr MemoryMap::WriteAccessResultSet MemoryMap::WriteByte(const MemoryAddress& maddr, const Byte& value) noexcept
{
    AccessResultSet accessResult = ReadByte(maddr);
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

constexpr MemoryMap::WordAccessResultSet MemoryMap::ReadWordLE(const MemoryAddress& maddr) const noexcept
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

    auto high = ReadByte({bank, ++address});
    if (high.IsFailure())
    {
        return static_cast<WordAccessResultSet>(high);
    }

    Word w(high, low);

    if (!IsValidRange({
                          0x7FFF, // ROM Boundary
                          0x9FFF, // VRAM Boundary
                          0xBFFF, // ERAM Boundary
                          0xDFFF, // WRAM Boundary
                          0xFE9F, // OAM Boundary
                          0xFF7F, // IO Boundary
                          0xFFFE, // HRAM Boundary
                      },
                      address))
    {
        return WordAccessResultSet(ResultAccessCrossesRegionBoundary(true), w);
    }

    return WordAccessResultSet::DefaultResultSuccess(w);
}

constexpr MemoryMap::WordAccessResultSet MemoryMap::WriteWordLE(const MemoryAddress& maddr, const Word& value) noexcept
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

    auto high = WriteByte({bank, ++address}, value.HighByte());
    if (high.IsFailure())
    {
        return static_cast<WordAccessResultSet>(high);
    }

    Word w(high, low);

    if (!IsValidRange({
                          0x7FFF, // ROM Boundary
                          0x9FFF, // VRAM Boundary
                          0xBFFF, // ERAM Boundary
                          0xDFFF, // WRAM Boundary
                          0xFE9F, // OAM Boundary
                          0xFF7F, // IO Boundary
                          0xFFFE, // HRAM Boundary
                      },
                      address))
    {
        return WordAccessResultSet(ResultAccessCrossesRegionBoundary(true), w);
    }

    return WordAccessResultSet::DefaultResultSuccess(w);
}

constexpr MemoryMap::Register8AccessResultSet MemoryMap::ReadByte(const cpu::RegisterType& type) const noexcept
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

constexpr MemoryMap::Register8AccessResultSet MemoryMap::WriteByte(const cpu::RegisterType& type, const Byte& value) noexcept
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
constexpr MemoryMap::Register16AccessResultSet MemoryMap::ReadWord(const cpu::RegisterType& type) const noexcept
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

constexpr MemoryMap::Register16AccessResultSet MemoryMap::WriteWord(const cpu::RegisterType& type, const Word& value) noexcept
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

constexpr const Nibble& MemoryMap::ReadFlag() const noexcept
{
    return _registers.F();
}

constexpr Nibble MemoryMap::WriteFlag(const Nibble& value) noexcept
{
    auto oldValue  = _registers.F();
    _registers.F() = value;
    return oldValue;
}

} // namespace pgb::memory