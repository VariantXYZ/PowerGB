#include <cstddef>
#include <initializer_list>
#include <memory>

#include <cpu/include/cpu.hpp>
#include <memory/include/memory.hpp>
#include <test/include/acutest.h>

using namespace pgb::memory;

void test_initialize(void);

TEST_LIST = {
    {"Initialize", test_initialize},

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

void test_initialize(void)
{
    // Test initialization failure cases
    {
        auto cpu  = std::make_unique<pgb::cpu::CPU>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidRomBankCount, 0>(mmap, {0, 1, 3, 513});
        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidVramBankCount, 1>(mmap, {0, 3});
        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidEramBankCount, 2>(mmap, {2, 3, 15});
        test_initialize_failure_helper<MemoryMap::ResultInitializeInvalidWramBankCount, 3>(mmap, {0, 7, 9});
    }

    // Test valid initialization with minimal banks
    {
        auto cpu  = std::make_unique<pgb::cpu::CPU>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        TEST_ASSERT(!mmap->IsInitialized());
        auto result = mmap->Initialize(2, 1, 0, 2);
        TEST_ASSERT(result.IsSuccess());
        TEST_ASSERT(mmap->IsInitialized());
    }

    // Test valid initialization with max banks
    {
        auto cpu  = std::make_unique<pgb::cpu::CPU>();
        auto mmap = std::make_unique<MemoryMap>(*cpu);

        TEST_ASSERT(!mmap->IsInitialized());
        auto result = mmap->Initialize(MaxRomBankCount, MaxVramBankCount, MaxEramBankCount, MaxWramBankCount);
        TEST_ASSERT(result.IsSuccess());
        TEST_ASSERT(mmap->IsInitialized());
    }

    // Verify reinitialization fails without a reset
    {
        auto cpu  = std::make_unique<pgb::cpu::CPU>();
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