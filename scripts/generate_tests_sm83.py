import os, sys
import json

script_path = sys.argv[0]
tests_path = sys.argv[1]
repository_path = sys.argv[2]

assert os.path.exists(f"{repository_path}/v1/00.json"), "sm83 directory is not checked out"

SUPPORTED_OPCODES = ([
    (0x00,"nop"), # NOP
    (0x01,"ld"), # LD BC, nnnn
    (0x02,"ld"), # LD [BC], A
    (0x03,"alu"), # INC BC
    (0x04,"alu"), # INC B
    (0x05,"alu"), # DEC B
    (0x06,"ld"), # LD B, nn
    (0x07,"rsb"), # RLCA
    (0x08,"ld"), # LD [nnnn], sp
    (0x09,"alu"), # ADD HL, BC
    (0x0A,"ld"), # LD A, [BC]
    (0x0B,"alu"), # DEC BC
    (0x0C,"alu"), # INC C
    (0x0D,"alu"), # DEC C
    (0x0E,"ld"), # LD C, nn
    (0x0F,"rsb"), # RRCA
    (0x11,"ld"), # LD DE, nnnn
    (0x12,"ld"), # LD [DE], A
    (0x13,"alu"), # INC DE
    (0x14,"alu"), # INC D
    (0x15,"alu"), # DEC D
    (0x16,"ld"), # LD D, nn
    (0x17,"rsb"), # RLA
    (0x19,"alu"), # ADD HL, DE
    (0x1A,"ld"), # LD A, [DE]
    (0x1B,"alu"), # DEC DE
    (0x1C,"alu"), # INC E
    (0x1D,"alu"), # DEC E
    (0x1E,"ld"), # LD E, nn
    (0x1F,"rsb"), # RRA
    (0x21,"ld"), # LD HL, nnnn
    (0x22,"ld"), # LD HL+, A
    (0x23,"alu"), # INC HL
    (0x24,"alu"), # INC H
    (0x25,"alu"), # DEC H
    (0x26,"ld"), # LD H, nn
    (0x29,"alu"), # ADD HL, HL
    (0x2A,"ld"), # LD A, [HL+]
    (0x2B,"alu"), # DEC HL
    (0x2C,"alu"), # INC L
    (0x2D,"alu"), # DEC L
    (0x2E,"ld"), # LD L, nn
    (0x31,"ld"), # LD SP, nnnn
    (0x32,"ld"), # LD [HL-], A
    (0x33,"alu"), # INC SP
    (0x34,"alu"), # INC [HL]
    (0x35,"alu"), # DEC [HL]
    (0x36,"ld"), # LD [HL], nn
    (0x39,"alu"), # ADD HL, SP
    (0x3A,"ld"), # LD A, [HL-]
    (0x3B,"alu"), # DEC SP
    (0x3C,"alu"), # INC A
    (0x3D,"alu"), # DEC A
    (0x3E,"ld"), # LD A, nn
    # 0x40 to 0x7F is the main load block
    (0x40,"ld"), # LD B, B
    (0x41,"ld"), # LD B, C
    (0x42,"ld"), # LD B, D
    (0x43,"ld"), # LD B, E
    (0x44,"ld"), # LD B, H
    (0x45,"ld"), # LD B, L
    (0x46,"ld"), # LD B, [HL]
    (0x47,"ld"), # LD B, A
    (0x48,"ld"), # LD C, B
    (0x49,"ld"), # LD C, C
    (0x4A,"ld"), # LD C, D
    (0x4B,"ld"), # LD C, E
    (0x4C,"ld"), # LD C, H
    (0x4D,"ld"), # LD C, L
    (0x4E,"ld"), # LD C, [HL]
    (0x4F,"ld"), # LD C, A
    (0x50,"ld"), # LD D, B
    (0x51,"ld"), # LD D, C
    (0x52,"ld"), # LD D, D
    (0x53,"ld"), # LD D, E
    (0x54,"ld"), # LD D, H
    (0x55,"ld"), # LD D, L
    (0x56,"ld"), # LD D, [HL]
    (0x57,"ld"), # LD D, A
    (0x58,"ld"), # LD E, B
    (0x59,"ld"), # LD E, C
    (0x5A,"ld"), # LD E, D
    (0x5B,"ld"), # LD E, E
    (0x5C,"ld"), # LD E, H
    (0x5D,"ld"), # LD E, L
    (0x5E,"ld"), # LD E, [HL]
    (0x5F,"ld"), # LD E, A
    (0x60,"ld"), # LD H, B
    (0x61,"ld"), # LD H, C
    (0x62,"ld"), # LD H, D
    (0x63,"ld"), # LD H, E
    (0x64,"ld"), # LD H, H
    (0x65,"ld"), # LD H, L
    (0x66,"ld"), # LD H, [HL]
    (0x67,"ld"), # LD H, A
    (0x68,"ld"), # LD L, B
    (0x69,"ld"), # LD L, C
    (0x6A,"ld"), # LD L, D
    (0x6B,"ld"), # LD L, E
    (0x6C,"ld"), # LD L, H
    (0x6D,"ld"), # LD L, L
    (0x6E,"ld"), # LD L, [HL]
    (0x6F,"ld"), # LD L, A
    (0x70,"ld"), # LD [HL], B
    (0x71,"ld"), # LD [HL], C
    (0x72,"ld"), # LD [HL], D
    (0x73,"ld"), # LD [HL], E
    (0x74,"ld"), # LD [HL], H
    (0x75,"ld"), # LD [HL], L
    # 0x76 is HALT ('ld [hl], [hl]')
    (0x77,"ld"), # LD [HL], A
    (0x78,"ld"), # LD A, B
    (0x79,"ld"), # LD A, C
    (0x7A,"ld"), # LD A, D
    (0x7B,"ld"), # LD A, E
    (0x7C,"ld"), # LD A, H
    (0x7D,"ld"), # LD A, L
    (0x7E,"ld"), # LD A, [HL]
    (0x7F,"ld"), # LD A, A
    (0x80,"alu"), # ADD A, B
    (0x81,"alu"), # ADD A, C
    (0x82,"alu"), # ADD A, D
    (0x83,"alu"), # ADD A, E
    (0x84,"alu"), # ADD A, H
    (0x85,"alu"), # ADD A, L
    (0x86,"alu"), # ADD A, [HL]
    (0x87,"alu"), # ADD A, A
    (0x88,"alu"), # ADC A, B
    (0x89,"alu"), # ADC A, C
    (0x8A,"alu"), # ADC A, D
    (0x8B,"alu"), # ADC A, E
    (0x8C,"alu"), # ADC A, H
    (0x8D,"alu"), # ADC A, L
    (0x8E,"alu"), # ADC A, [HL]
    (0x8F,"alu"), # ADC A, A
    (0x90,"alu"), # SUB A, B
    (0x91,"alu"), # SUB A, C
    (0x92,"alu"), # SUB A, D
    (0x93,"alu"), # SUB A, E
    (0x94,"alu"), # SUB A, H
    (0x95,"alu"), # SUB A, L
    (0x96,"alu"), # SUB A, [HL]
    (0x97,"alu"), # SUB A, A
    (0x98,"alu"), # SBC A, B
    (0x99,"alu"), # SBC A, C
    (0x9A,"alu"), # SBC A, D
    (0x9B,"alu"), # SBC A, E
    (0x9C,"alu"), # SBC A, H
    (0x9D,"alu"), # SBC A, L
    (0x9E,"alu"), # SBC A, [HL]
    (0x9F,"alu"), # SBC A, A
    (0xA0,"alu"), # AND A, B
    (0xA1,"alu"), # AND A, C
    (0xA2,"alu"), # AND A, D
    (0xA3,"alu"), # AND A, E
    (0xA4,"alu"), # AND A, H
    (0xA5,"alu"), # AND A, L
    (0xA6,"alu"), # AND A, [HL]
    (0xA7,"alu"), # AND A, A
    (0xA8,"alu"), # XOR A, B
    (0xA9,"alu"), # XOR A, C
    (0xAA,"alu"), # XOR A, D
    (0xAB,"alu"), # XOR A, E
    (0xAC,"alu"), # XOR A, H
    (0xAD,"alu"), # XOR A, L
    (0xAE,"alu"), # XOR A, [HL]
    (0xAF,"alu"), # XOR A, A
    (0xB0,"alu"), # OR A, B
    (0xB1,"alu"), # OR A, C
    (0xB2,"alu"), # OR A, D
    (0xB3,"alu"), # OR A, E
    (0xB4,"alu"), # OR A, H
    (0xB5,"alu"), # OR A, L
    (0xB6,"alu"), # OR A, [HL]
    (0xB7,"alu"), # OR A, A
    (0xB8,"alu"), # CP A, B
    (0xB9,"alu"), # CP A, C
    (0xBA,"alu"), # CP A, D
    (0xBB,"alu"), # CP A, E
    (0xBC,"alu"), # CP A, H
    (0xBD,"alu"), # CP A, L
    (0xBE,"alu"), # CP A, [HL]
    (0xBF,"alu"), # CP A, A
    (0xC1,"ld"), # POP BC
    (0xC5,"ld"), # PUSH BC
    (0xD1,"ld"), # POP DE
    (0xD5,"ld"), # PUSH DE
    (0xE0,"ld"), # LDH [nn], A
    (0xE1,"ld"), # POP HL
    (0xE5,"ld"), # PUSH HL
    (0xF0,"ld"), # LDH A, [nn]
    (0xE2,"ld"), # LDH [C], A
    (0xF2,"ld"), # LDH A, [C]
    (0xF1,"ld"), # POP AF
    (0xF5,"ld"), # PUSH AF
    (0xF9,"ld"), # LD SP, HL
    (0xEA,"ld"), # LD [nnnn], A
    (0xFA,"ld"), # LD A, [nnnn]

    # CB Prefix
    (0xCB00,"rsb"), # RLC B
    (0xCB01,"rsb"), # RLC C
    (0xCB02,"rsb"), # RLC D
    (0xCB03,"rsb"), # RLC E
    (0xCB04,"rsb"), # RLC H
    (0xCB05,"rsb"), # RLC L

    (0xCB10,"rsb"), # RL B
    (0xCB11,"rsb"), # RL C
    (0xCB12,"rsb"), # RL D
    (0xCB13,"rsb"), # RL E
    (0xCB14,"rsb"), # RL H
    (0xCB15,"rsb"), # RL L
])

for info in SUPPORTED_OPCODES:
    opcode = info[0]
    insn = info[1]
    prefix = (0xFF00 & opcode) >> 8
    if prefix != 0:
        opcode = opcode & 0xFF
    json_file = f"{opcode:02X}.json" if prefix == 0x00 else f"{prefix:02X} {opcode:02X}.json"
    input_filename = os.path.join(f"{repository_path}/v1", json_file.lower())
    output_filename = f"{tests_path}/sm83_{opcode:02X}.cpp" if prefix == 0x00 else f"{tests_path}/sm83_{prefix:02X}{opcode:02X}.cpp"
    registry_string = "InstructionRegistryNoPrefix" if prefix == 0x00 else f"InstructionRegistryPrefix{prefix:02X}"
    with open(input_filename, "r") as input_fn, open(output_filename, "w", encoding="utf-8") as output_fn:
        input_json = json.load(input_fn)

        output_fn.write(f"// File autogenerated by {script_path}\n")
        output_fn.write(f"#include <test/include/acutest.h>\n\n")
        output_fn.write(f"#include <cpu/include/cpu.hpp>\n")
        output_fn.write(f"#include <cpu/include/registers.hpp>\n")
        output_fn.write(f"#include <memory/include/memory.hpp>\n\n")

        output_fn.write(f"using namespace pgb::common;\n")
        output_fn.write(f"using namespace pgb::memory;\n")
        output_fn.write(f"using namespace pgb::cpu;\n\n")

        output_fn.write(f"static auto registers = RegisterFile();\n")
        output_fn.write(f"static auto mmap      = MemoryMap(registers, MinRomBankCount, MinVramBankCount, 1, MinWramBankCount);\n\n")

        output_fn.write(f"""
#define WriteRegisterWord(register, value) do {{ auto result = mmap.WriteWord(register, value); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); }} while(0)
#define WriteRegisterByte(register, value) do {{ auto result = mmap.WriteByte(register, value); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); }} while(0)
#define WriteRegisterFlag(value) do {{ TEST_ASSERT(static_cast<const Byte>(mmap.WriteFlagByte(static_cast<const Byte&>(value))) == 0x00); }} while(0)
#define WriteMemory(address, value) do {{ auto result = mmap.WriteByte(address, value); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); }} while(0)

#define CheckRegisterWord(register, value) do {{ auto result = mmap.ReadWord(register); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); TEST_ASSERT(static_cast<const Word&>(result) == value); }} while(0)
#define CheckRegisterByte(register, value) do {{ auto result = mmap.ReadByte(register); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); TEST_ASSERT(static_cast<const Byte&>(result) == value); }} while(0)
#define CheckRegisterFlag(value) do {{ TEST_ASSERT_(static_cast<const Byte>(mmap.ReadFlagByte()) == value, "0x%hhx != 0x%hhx", static_cast<const Byte>(mmap.ReadFlagByte()).data, value); }} while(0)
#define CheckMemory(address, value) do {{ auto result = mmap.ReadByte(address); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); TEST_ASSERT_(static_cast<const Byte&>(result) == value, "%hhu != %hhu", static_cast<const Byte&>(result).data, value); }} while(0)
static_assert(instruction::{registry_string}::Callbacks[0x{opcode:02X}] != nullptr);
static_assert(instruction::{registry_string}::Ticks[0x{opcode:02X}] != 0);
\n""")

        test_names = []
        test_content = []
        for test in input_json:
            skip_test = False
            test_name = test['name'].replace(' ', '_')
            test_names.append(test_name)
            test_content.append(f"""
void test_{test_name}()
{{
    if (skip_test_{test_name})
    {{
        TEST_SKIP("Skipping");
        return;
    }}
    mmap.Reset();
    auto mmapResult = mmap.Initialize(MinRomBankCount, MinVramBankCount, 1, MinWramBankCount);
    TEST_ASSERT_(mmapResult.IsSuccess(), "%s", mmapResult.GetStatusDescription());

    // Initial state
""")
            # These tests assume '64K of flat ram (Address range from 0x0000 to 0xFFFF)
            # No point bank swapping, just assume raw data
            for val in test['initial']:
                if val in ("pc", "sp"):
                    address = test['initial'][val]
                    assert address <= 0xFFFF
                    if address >= 0xFEA0 and address <= 0xFEFF:
                        # Don't bother with tests that write to illegal locations
                        skip_test = True
                    test_content.append(f"    WriteRegisterWord(RegisterType::{val.upper()}, 0x{address:04X});")
                elif val in ("a", "b", "c", "d", "e", "h", "l", "ie"):
                    test_content.append(f"    WriteRegisterByte(RegisterType::{val.upper()}, 0x{test['initial'][val]:02X});")
                elif val == 'f':
                    test_content.append(f"    WriteRegisterFlag(0x{test['initial'][val]:02X});")
                elif val == 'ime':
                    v = test['initial'][val]
                    assert v in (0, 1)
                    if v == 0:
                        test_content.append(f"    mmap.DisableIME();")
                    else:
                        test_content.append(f"    mmap.EnableIME();")
                elif val == "ram":
                    for ram in sorted(test['initial'][val], key=lambda x: x[0], reverse=True):
                        address = ram[0]
                        assert address <= 0xFFFF
                        if address >= 0xFEA0 and address <= 0xFEFF:
                            # Don't bother with tests that write to illegal locations
                            skip_test = True
                        test_content.append(f"    WriteMemory(0x{ram[0]:04X}, 0x{ram[1]:02X});")

            test_content.append(f"""
    // Execute step
    {{
        auto resultByte = mmap.ReadByte(mmap.ReadPC());
        TEST_ASSERT_(resultByte.IsSuccess(), "%s", resultByte.GetStatusDescription());
        auto ticks = ExecuteActiveDecoder(static_cast<const Byte&>(resultByte), mmap);
        TEST_ASSERT(ticks == instruction::InstructionRegistryNoPrefix::Ticks[static_cast<const Byte&>(resultByte)]);""")
            if prefix == 0x00:
                test_content.append(f"""        TEST_ASSERT(ticks == {len(test['cycles']) * 4});""")
            else:
                test_content.append(f"""
        // Execute additional step for prefix
        auto resultByte2 = mmap.ReadByte(mmap.ReadPC());
        TEST_ASSERT_(resultByte2.IsSuccess(), "%s", resultByte2.GetStatusDescription());
        auto ticks2 = ExecuteActiveDecoder(static_cast<const Byte&>(resultByte2), mmap);
        TEST_ASSERT(ticks2 == instruction::{registry_string}::Ticks[static_cast<const Byte&>(resultByte2)]);
        TEST_ASSERT(ticks + ticks2 == {len(test['cycles']) * 4});""")

            test_content.append(f"""    }}

    // Final state
            """)

            for val in test['final']:
                if val in ("pc", "sp"):
                    address = test['final'][val]
                    assert address <= 0xFFFF
                    if address >= 0xFEA0 and address <= 0xFEFF:
                        # Don't bother with tests that write to illegal locations
                        skip_test = True
                    test_content.append(f"    CheckRegisterWord(RegisterType::{val.upper()}, 0x{address:04X});")
                elif val in ("a", "b", "c", "d", "e", "h", "l", "ie"):
                    test_content.append(f"    CheckRegisterByte(RegisterType::{val.upper()}, 0x{test['final'][val]:02X});")
                elif val == 'f':
                    test_content.append(f"    CheckRegisterFlag(0x{test['final'][val]:02X});")
                elif val == 'ime':
                    v = test['final'][val]
                    assert v in (0, 1)
                    if v == 0:
                        test_content.append(f"    TEST_ASSERT(!mmap.IMEEnabled());")
                    else:
                        test_content.append(f"    TEST_ASSERT(mmap.IMEEnabled());")
                elif val == "ram":
                    for ram in sorted(test['final'][val], key=lambda x: x[0], reverse=False):
                        address = ram[0]
                        assert address <= 0xFFFF
                        if address >= 0xFEA0 and address <= 0xFEFF:
                            # Don't bother with tests that write to illegal locations
                            skip_test = True
                        test_content.append(f"    CheckMemory(0x{ram[0]:04X}, 0x{ram[1]:02X});")

            test_content.append(f"}}")
            output_fn.write(f"const bool skip_test_{test_name} = {'true' if skip_test else 'false'};\n")

        for name in test_names:
            output_fn.write(f"void test_{name}(void);\n")

        output_fn.write(f"TEST_LIST = {{\n")
        for name in test_names:
            output_fn.write(f"    {{ \"{name}\", test_{name} }},\n")
        output_fn.write(f"    {{NULL, NULL}}\n")
        output_fn.write(f"}};\n")

        for content in test_content:
            output_fn.write(content)
            output_fn.write("\n")
