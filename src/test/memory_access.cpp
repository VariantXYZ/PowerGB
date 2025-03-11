#include "common/result.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>

#include <cpu/include/registers.hpp>
#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

using namespace pgb::memory;

void test_access_basic(void);

template <std::uint_fast16_t BankStart, std::uint_fast16_t BankEnd, std::uint_fast16_t AddressStart, std::uint_fast16_t AddressEnd, std::uint_fast8_t Value>
void test_access_helper(void);
void test_access_echo_ram(void);
void test_access_FEA0_FEFF(void);
void test_access_IE(void);

TEST_LIST = {
    {"Access Basic Tests", test_access_basic},
    {"Access ROM (bank 0)", test_access_helper<0, 0, 0x0000, 0x3FFF, 0xFF>},
    {"Access ROM (bank > 0)", test_access_helper<1, MaxRomBankCount - 1, 0x4000, 0x7FFF, 0xFF>},
    {"Access VRAM", test_access_helper<0, MaxVramBankCount - 1, 0x8000, 0x9FFF, 0xFF>},
    {"Access ERAM", test_access_helper<0, MaxEramBankCount - 1, 0xA000, 0xBFFF, 0xFF>},
    {"Access WRAM (bank 0)", test_access_helper<0, 0, 0xC000, 0xCFFF, 0xFF>},
    {"Access WRAM (bank > 0)", test_access_helper<1, MaxWramBankCount - 1, 0xD000, 0xDFFF, 0xFF>},
    {"Access OAM", test_access_helper<0, 0, 0xFE00, 0xFE9F, 0xFF>},
    {"Access Echo RAM", test_access_echo_ram},
    {"Access FEA0-FEFF", test_access_FEA0_FEFF},
    {"Access I/O", test_access_helper<0, 0, 0xFF00, 0xFF7F, 0xFF>},
    {"Access HRAM", test_access_helper<0, 0, 0xFF80, 0xFFFE, 0xFF>},
    {"Access IE", test_access_IE},
    {NULL, NULL}
};

void test_access_basic(void)
{
    auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
    auto mmap = std::make_unique<MemoryMap>(*cpu);
    TEST_ASSERT(mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    // Invalid bank
    {
        // VRAM Bank 3 is invalid
        auto r = mmap->AccessByte({3, 0x9000});
        TEST_ASSERT(r.IsFailure());
        TEST_ASSERT(r.IsResult<MemoryMap::ResultAccessInvalidBank>());
    }

    // Simple R/W
    {
        auto r = mmap->AccessByte({0, 0x3000});
        TEST_ASSERT(r.IsSuccess());
        TEST_ASSERT(static_cast<const Byte&>(r) == 0);

        auto rW = mmap->WriteByte({0, 0x3000}, 26);
        TEST_ASSERT(rW.IsSuccess());

        auto rR = mmap->AccessByte({0, 0x3000});
        TEST_ASSERT(rR.IsSuccess());
        TEST_ASSERT(static_cast<const Byte&>(rR) == 26);
    }
}

template <std::uint_fast16_t BankStart, std::uint_fast16_t BankEnd, std::uint_fast16_t AddressStart, std::uint_fast16_t AddressEnd, std::uint_fast8_t Value>
void test_access_helper(void)
{
    auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
    auto mmap = std::make_unique<MemoryMap>(*cpu);
    TEST_ASSERT(mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    for (std::uint_fast16_t bank = BankStart; bank <= BankEnd; ++bank)
    {
        for (std::uint_fast16_t address = AddressStart; address <= AddressEnd; ++address)
        {
            auto r = mmap->AccessByte({bank, address});
            TEST_ASSERT(r.IsSuccess());
            TEST_ASSERT(r.IsResult<pgb::common::ResultSuccess>());
            TEST_ASSERT(static_cast<const Byte&>(r) == 0);

            auto rW = mmap->WriteByte({bank, address}, 0xFF);
            TEST_ASSERT(rW.IsSuccess());
            TEST_ASSERT(r.IsResult<pgb::common::ResultSuccess>());

            auto rR = mmap->AccessByte({bank, address});
            TEST_ASSERT(rR.IsSuccess());
            TEST_ASSERT(r.IsResult<pgb::common::ResultSuccess>());
            TEST_ASSERT(static_cast<const Byte&>(rR) == 0xFF);
        }
    }
}

void test_access_echo_ram(void)
{
    auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
    auto mmap = std::make_unique<MemoryMap>(*cpu);
    TEST_ASSERT(mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    // "The range E000-FDFF is mapped to WRAM...
    // This causes the address to effectively wrap around. All reads and writes to this range have the same effect as reads and writes to C000-DDFF."
    for (std::uint_fast16_t address = 0xC000; address < 0xD000; ++address)
    {
        auto echo_address = static_cast<uint_fast16_t>(address + 0x2000);

        auto r            = mmap->AccessByte({0, address});
        TEST_ASSERT(r.IsSuccess());
        TEST_ASSERT(static_cast<const Byte&>(r) == 0);

        auto rE = mmap->AccessByte({0, echo_address});
        TEST_ASSERT(rE.IsSuccess());
        TEST_ASSERT(rE.IsResult<MemoryMap::ResultAccessProhibitedAddress>());
        TEST_ASSERT(static_cast<const Byte&>(rE) == 0);

        auto rW = mmap->WriteByte({0, address}, 2);
        TEST_ASSERT(rW.IsSuccess());
        TEST_ASSERT(static_cast<const Byte&>(rW) == 2);

        auto rER = mmap->AccessByte({0, echo_address});
        TEST_ASSERT(rER.IsSuccess());
        TEST_ASSERT(rER.IsResult<MemoryMap::ResultAccessProhibitedAddress>());
        TEST_ASSERT(static_cast<const Byte&>(rER) == 2);

        auto rEW = mmap->WriteByte({0, echo_address}, 1);
        TEST_ASSERT(rEW.IsSuccess());
        TEST_ASSERT(rEW.IsResult<MemoryMap::ResultAccessProhibitedAddress>());
        TEST_ASSERT(static_cast<const Byte&>(rEW) == 1);

        auto rR = mmap->AccessByte({0, address});
        TEST_ASSERT(rR.IsSuccess());
        TEST_ASSERT(static_cast<const Byte&>(rR) == 1);
    }

    for (std::uint_fast16_t bank = 1; bank < MaxWramBankCount; ++bank)
    {
        for (std::uint_fast16_t address = 0xD000; address < 0xDE00; ++address)
        {
            auto echo_address = static_cast<uint_fast16_t>(address + 0x2000);

            auto r            = mmap->AccessByte({bank, address});
            TEST_ASSERT(r.IsSuccess());
            TEST_ASSERT(static_cast<const Byte&>(r) == 0);

            auto rE = mmap->AccessByte({bank, echo_address});
            TEST_ASSERT(rE.IsSuccess());
            TEST_ASSERT(rE.IsResult<MemoryMap::ResultAccessProhibitedAddress>());
            TEST_ASSERT(static_cast<const Byte&>(rE) == 0);

            auto rW = mmap->WriteByte({bank, address}, 2);
            TEST_ASSERT(rW.IsSuccess());
            TEST_ASSERT(static_cast<const Byte&>(rW) == 2);

            auto rER = mmap->AccessByte({bank, echo_address});
            TEST_ASSERT(rER.IsSuccess());
            TEST_ASSERT(rER.IsResult<MemoryMap::ResultAccessProhibitedAddress>());
            TEST_ASSERT(static_cast<const Byte&>(rER) == 2);

            auto rEW = mmap->WriteByte({bank, echo_address}, 1);
            TEST_ASSERT(rEW.IsSuccess());
            TEST_ASSERT(rEW.IsResult<MemoryMap::ResultAccessProhibitedAddress>());
            TEST_ASSERT(static_cast<const Byte&>(rEW) == 1);

            auto rR = mmap->AccessByte({bank, address});
            TEST_ASSERT(rR.IsSuccess());
            TEST_ASSERT(static_cast<const Byte&>(rR) == 1);
        }
    }
}

void test_access_FEA0_FEFF(void)
{
    auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
    auto mmap = std::make_unique<MemoryMap>(*cpu);
    TEST_ASSERT(mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    // On CGB revision E, AGB, AGS, and GBP, it returns the high nibble of the lower address byte twice, e.g. FFAx returns $AA, FFBx returns $BB, and so forth.
    // (Bank should be irrelevant)
    const std::uint_fast8_t _FEA0_FEFF[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    for (std::uint_fast16_t bank = 0; bank < MaxRomBankCount; ++bank)
    {
        for (std::uint_fast16_t address = 0xFEA0; address < 0xFF00; ++address)
        {
            auto value = _FEA0_FEFF[((address & 0x00F0) >> 4) - 0xA];
            auto r     = mmap->AccessByte({bank, address});
            TEST_ASSERT(r.IsSuccess());
            TEST_ASSERT(r.IsResult<MemoryMap::ResultAccessReadOnlyProhibitedAddress>());
            TEST_ASSERT(static_cast<const Byte&>(r) == value);
        }
    }
}

void test_access_IE(void)
{
    auto cpu       = std::make_unique<pgb::cpu::RegisterFile>();
    auto const_cpu = const_cast<const pgb::cpu::RegisterFile*>(cpu.get());
    auto mmap      = std::make_unique<MemoryMap>(*cpu);
    TEST_ASSERT(mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount).IsSuccess());

    auto r = mmap->AccessByte({0, 0xFFFF});
    TEST_ASSERT(r.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(r) == 0);
    TEST_ASSERT(static_cast<const Byte&>(r) == const_cpu->IE());

    auto rW = mmap->WriteByte({0, 0xFFFF}, 0xFF);
    TEST_ASSERT(rW.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(rW) == 0xFF);
    TEST_ASSERT(static_cast<const Byte&>(rW) == const_cpu->IE());
    
    auto rR = mmap->AccessByte({0, 0xFFFF});
    TEST_ASSERT(rR.IsSuccess());
    TEST_ASSERT(static_cast<const Byte&>(rR) == 0xFF);
}
