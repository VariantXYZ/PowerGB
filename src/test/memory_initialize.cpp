
#include <cstddef>
#include <initializer_list>
#include <memory>

#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

using namespace pgb::memory;

// Initialize tests take a while so we just move them here

void test_initialize_failures(void);
void test_initialize_success(void);

TEST_LIST = {
    {"Initialize Failure Cases", test_initialize_failures},
    {"Initialize Valid Cases", test_initialize_success},
    {NULL, NULL}
};

template <typename Result, std::size_t ArgIndex>
void test_initialize_failure_helper(std::unique_ptr<MemoryMap>& mmap, std::initializer_list<std::size_t> invalidBanks)
{
    static_assert(ArgIndex < 4);
    TEST_ASSERT(!mmap->IsInitialized());
    for (auto& b : invalidBanks)
    {

        auto result = (ArgIndex == 0)   ? mmap->Initialize(b, 1, 0, 2)
                      : (ArgIndex == 1) ? mmap->Initialize(2, b, 0, 2)
                      : (ArgIndex == 2) ? mmap->Initialize(2, 1, b, 2)
                                        : mmap->Initialize(2, 1, 0, b);
        TEST_ASSERT(!mmap->IsInitialized());
        TEST_ASSERT(result.IsFailure());
        TEST_ASSERT(result.IsResult<Result>());
    }
}

void test_initialize_failures(void)
{
    // Test initialization failure cases return expected results
    {
        auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidRomBankCount, 0>(mmap, {0, 1, 3, 513});
        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidVramBankCount, 1>(mmap, {0, 3});
        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidEramBankCount, 2>(mmap, {2, 3, 15});
        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidWramBankCount, 3>(mmap, {0, 7, 9});
    }

    // Verify reinitialization fails without a reset
    {
        auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        TEST_ASSERT(!mmap->IsInitialized());
        auto result = mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);
        TEST_ASSERT(result.IsSuccess());
        TEST_ASSERT(mmap->IsInitialized());

        auto result2 = mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);
        TEST_ASSERT(result2.IsFailure());
        TEST_ASSERT(result2.IsResult<MemoryMap::ResultInitializeAlreadyInitialized>());

        mmap->Reset();
        auto result3 = mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);
        TEST_ASSERT(result3.IsSuccess());
    }
}

void test_initialize_success(void)
{
    // Test valid initialization with max banks
    {
        auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        TEST_ASSERT(!mmap->IsInitialized());
        auto result = mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);
        TEST_ASSERT(result.IsSuccess());
        TEST_ASSERT(mmap->IsInitialized());
    }

    // Test all valid initialization patterns
    {
        auto cpu  = std::make_unique<pgb::cpu::RegisterFile>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        // We explicitly lay out the valid banks here instead of just pulling the constants from memory.hpp
        for (auto romBank : {2, 4, 8, 16, 32, 64, 128, 256, 512, 72, 80, 96})
        {
            for (auto vramBank : {1, 2})
            {
                for (auto eramBank : {0, 1, 4, 16, 8})
                {
                    for (auto wramBank : {2, 8})
                    {
                        TEST_ASSERT(!mmap->IsInitialized());
                        auto result = mmap->Initialize(romBank, vramBank, eramBank, wramBank);
                        TEST_ASSERT(result.IsSuccess());
                        TEST_ASSERT(mmap->IsInitialized());
                        mmap->Reset();
                        TEST_ASSERT(!mmap->IsInitialized());
                    }
                }
            }
        }
    }
}