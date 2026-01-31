#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include <common/registry.hpp>
#include <common/util.hpp>
#include <cpu/include/instruction.hpp>
#include <memory/include/memory.hpp>

// There are only a few types of instructions to decode:
// * 0 parameters
// * 1 parameter (16 bit)
// * 1 parameter (8 bit)
// * 2 parameters (8, 8)
// * 2 parameters (8, Reg16)
// * 2 parameters (Reg16, Reg16)

using namespace pgb::common;

namespace pgb::cpu::instruction
{

namespace detail
{
template <typename List>
struct DecoderHelper;

template <typename... Decoders>
struct DecoderHelper<registry::List<Decoders...>>
{
    static constexpr auto CreateCallbackMap() noexcept
    {
        // For opcode decoding, we only care about going from an 8-bit integer to a specific Decoder class, so just create a 256 element array
        // Doing this also lets us flag errors for unsupported opcodes
        constexpr std::size_t Size = 0xFF + 1;

        // Initialize all elements to nullptr by default
        std::array<std::size_t (*)(memory::MemoryMap&) noexcept, Size> callbacks{nullptr};

        ((callbacks[Decoders::Opcode] = &Decoders::Execute), ...);

        return callbacks;
    }
};

// Tags to group instructions by opcode prefix
struct InstructionRegistryTagNoPrefix;
struct InstructionRegistryTagPrefixCB;

template <typename RegistryType>
    requires(std::is_same_v<RegistryType, InstructionRegistryTagNoPrefix> || std::is_same_v<RegistryType, InstructionRegistryTagPrefixCB>)
struct InstructionRegistry
{
private:
    static constexpr auto Callbacks = detail::DecoderHelper<decltype(registry::Registry<RegistryType>())>::CreateCallbackMap();

public:
    static constexpr std::size_t Execute(std::uint_fast8_t opCode, memory::MemoryMap& mmap) noexcept
    {
        return Callbacks[opCode](mmap);
    }
};

} // namespace detail

using InstructionRegistryNoPrefix = detail::InstructionRegistry<detail::InstructionRegistryTagNoPrefix>;
using InstructionRegistryPrefixCB = detail::InstructionRegistry<detail::InstructionRegistryTagPrefixCB>;

// 0 parameter instructions
template <util::StringLiteral Name, std::uint_fast8_t OpCode, InstructionType InstructionHandler, bool Prefixed = false>
class InstructionDecoder
{
public:
    InstructionDecoder()              = delete;
    constexpr static auto Instruction = Name.value;
    constexpr static auto Opcode      = OpCode;

    constexpr static std::size_t Execute(memory::MemoryMap& mmap)
    {
        return InstructionHandler::ExecuteAll(mmap);
    }

    using RegistryType = std::conditional_t<Prefixed, InstructionRegistryPrefixCB, InstructionRegistryNoPrefix>;

private:
    inline static constexpr bool _registered = registry::Append<InstructionDecoder, RegistryType>();
    // The static assert is required to add it to the list
    static_assert(_registered);
};

// Helper function to facilitate ODR-use within an alias statement to guarantee it's added to the registry'
// e.g., `using X_Y_Decoder = Instantiate<InstructionDecoder<...>>::Type`
template <class T>
struct Instantiate
{
    using Type = T;
    static_assert(sizeof(Type));
};

} // namespace pgb::cpu::instruction
