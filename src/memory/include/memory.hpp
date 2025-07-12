#pragma once

#include <bit>
#include <cstdint>

#include <common/block.hpp>
#include <common/datatypes.hpp>
#include <common/result.hpp>
#include <cpu/include/registers.hpp>

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
constexpr static std::size_t OamSize                           = 0xA0;
constexpr static std::size_t IOSize                            = 0x80;
constexpr static std::size_t HramSize                          = 0x7F;

constexpr static std::size_t Rom0Boundary                      = 0x3FFF;
constexpr static std::size_t RomXBoundary                      = Rom0Boundary + RomBankSize;
constexpr static std::size_t VramBoundary                      = RomXBoundary + VramBankSize;
constexpr static std::size_t EramBoundary                      = VramBoundary + EramBankSize;
constexpr static std::size_t Wram0Boundary                     = EramBoundary + WramBankSize;
constexpr static std::size_t WramXBoundary                     = Wram0Boundary + WramBankSize;
constexpr static std::size_t OamBoundary                       = 0xFE00 + OamSize - 1;
// FEA0 to FEFF is prohibited
constexpr static std::size_t IOBoundary                        = 0xFF00 + IOSize - 1;
constexpr static std::size_t HramBoundary                      = IOBoundary + HramSize;

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

    using BankSetResultSet =
        common::ResultSet<
            void,
            common::ResultSuccess,
            ResultAccessInvalidBank>;

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

    struct MemoryAddress
    {
        uint_fast16_t bank : std::bit_width(static_cast<uint_fast16_t>(MaxBankValue));
        uint_fast16_t address : std::bit_width(static_cast<uint_fast16_t>(MaxAddressValue));
    };

private:
    // The bus maps references to the CPU IE
    cpu::RegisterFile& _registers;

    std::size_t _romBankCount;
    std::size_t _vramBankCount;
    std::size_t _eramBankCount;
    std::size_t _wramBankCount;

    // VRAM/WRAM bank select is managed by IO registers
    std::uint_fast16_t _romBankSelect : std::bit_width(static_cast<uint_fast16_t>(MaxRomBankCount));
    std::uint_fast16_t _eramBankSelect : std::bit_width(static_cast<uint_fast16_t>(MaxEramBankCount));

    // IO Registers should be handled as individual registers
    // We could be more memory efficient, but doing this allows for statically allocating this entire class
    Block<RomBankSize * Byte::TypeWidth, Byte::TypeWidth>  _rom[MaxRomBankCount];
    Block<VramBankSize * Byte::TypeWidth, Byte::TypeWidth> _vram[MaxVramBankCount];
    Block<EramBankSize * Byte::TypeWidth, Byte::TypeWidth> _eram[MaxEramBankCount];
    Block<WramBankSize * Byte::TypeWidth, Byte::TypeWidth> _wram[MaxWramBankCount];
    Block<OamSize * Byte::TypeWidth, Byte::TypeWidth>      _oam;
    Block<HramSize * Byte::TypeWidth, Byte::TypeWidth>     _hram;
    Block<IOSize * Byte::TypeWidth, Byte::TypeWidth>       _io;

    bool _isInitialized                      = false;

    constexpr static const Byte _FEA0_FEFF[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    AccessResultSet      ReadByte(const MemoryAddress&, bool) const noexcept;
    WriteAccessResultSet WriteByte(const MemoryAddress&, const Byte&, bool) noexcept;
    WordAccessResultSet  ReadWordLE(const MemoryAddress&, bool) const noexcept;
    WordAccessResultSet  WriteWordLE(const MemoryAddress&, const Word&, bool) noexcept;

public:
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
    InitializeResultSet Initialize(const Byte (&rom)[], const std::size_t size) noexcept;
    InitializeResultSet Initialize(std::size_t romBankCount, std::size_t vramBankCount, std::size_t eramBankCount, std::size_t wramBankCount) noexcept;

    // Sets all memory regions to 0
    void Reset() noexcept;

    bool IsInitialized() { return _isInitialized; }

    // Sets the active ROM Bank
    // ResultAccessInvalidBank is always a failure case.
    BankSetResultSet SetRomBank(std::uint_fast16_t) noexcept;

    // Sets the active ERAM Bank
    // ResultAccessInvalidBank is always a failure case.
    BankSetResultSet SetEramBank(std::uint_fast16_t) noexcept;

    auto GetEramBank() const noexcept { return static_cast<std::uint_fast16_t>(_eramBankSelect); }
    auto GetRomBank() const noexcept { return static_cast<std::uint_fast16_t>(_romBankSelect); };

    // Access a byte at a specific address, the stored result is a reference and is only valid if it is marked successful.
    // ResultAccessInvalidBank is always a failure case.
    // ResultAccessInvalidAddress is always a failure case.
    // ResultAccessProhibitedAddress is sometimes returned as a failure case.
    // ResultAccessReadOnlyProhibitedAddress is never a failure case.
    AccessResultSet ReadByte(const MemoryAddress&) const noexcept;
    // Same as above, but use the active bank
    AccessResultSet ReadByte(const std::uint_fast16_t) const noexcept;

    // Write a byte at a specific address if it is accessible and returns the previous value.
    // If the address is not accessible, the function will propagate the AccessByte error.
    // If access returns ResultAccessReadOnlyProhibitedAddress, the result of this function is a failure and the read value is in the result.
    // Note that this function will write as long as the address is within a valid range and the address is not ReadOnlyProhibited.
    WriteAccessResultSet WriteByte(const MemoryAddress&, const Byte&) noexcept;
    // Same as above, but use the active bank
    WriteAccessResultSet WriteByte(const std::uint_fast16_t, const Byte&) noexcept;

    // Access a word at a specific address, the stored result is a value and only valid if it is marked successful.
    // Treats the value in memory as being stored as little endian, so a byteswap will happen prior to returning.
    // The result behavior is the same as ReadByte except it can also return ResultAccessCrossesRegionBoundary which will never be a failure (consider it a warning).
    WordAccessResultSet ReadWordLE(const MemoryAddress&) const noexcept;
    // Same as above, but use the active bank
    WordAccessResultSet ReadWordLE(const std::uint_fast16_t) const noexcept;

    // Write a word at a specific address if it is accessible and returns the previous value.
    // Treats the value in memory as being stored as little endian, so a byteswap will happen prior to storing.
    // The result behavior is the same as WriteByte, except ReadWordLE is used instead of ReadByte.
    WordAccessResultSet WriteWordLE(const MemoryAddress&, const Word&) noexcept;
    // Same as above, but use the active bank
    WordAccessResultSet WriteWordLE(const std::uint_fast16_t, const Word&) noexcept;

    // Read byte stored in 8-bit register, the stored result is a value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    Register8AccessResultSet ReadByte(const cpu::RegisterType&) const noexcept;

    // Write byte into 8-bit register, the stored result is the previous value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    Register8AccessResultSet WriteByte(const cpu::RegisterType&, const Byte&) noexcept;

    // Read byte stored in 16-bit register, the stored result is a value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    Register16AccessResultSet ReadWord(const cpu::RegisterType&) const noexcept;

    // Write word into a 16-bit register, the stored result is the previous value and only valid if it is marked successful.
    // Returns ResultAccessRegisterInvalidWidth if this register is not accessible at that width.
    Register16AccessResultSet WriteWord(const cpu::RegisterType&, const Word&) noexcept;

    const Nibble& ReadFlag() const noexcept;
    Nibble        WriteFlag(const Nibble&) noexcept;
};

} // namespace pgb::memory