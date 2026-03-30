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
    (0xCB06,"rsb"), # RLC [HL]
    (0xCB07,"rsb"), # RLC A
    (0xCB08,"rsb"), # RRC B
    (0xCB09,"rsb"), # RRC C
    (0xCB0A,"rsb"), # RRC D
    (0xCB0B,"rsb"), # RRC E
    (0xCB0C,"rsb"), # RRC H
    (0xCB0D,"rsb"), # RRC L
    (0xCB0E,"rsb"), # RRC [HL]
    (0xCB0F,"rsb"), # RRC A
    (0xCB10,"rsb"), # RL B
    (0xCB11,"rsb"), # RL C
    (0xCB12,"rsb"), # RL D
    (0xCB13,"rsb"), # RL E
    (0xCB14,"rsb"), # RL H
    (0xCB15,"rsb"), # RL L
    (0xCB16,"rsb"), # RL [HL]
    (0xCB17,"rsb"), # RL A
    (0xCB18,"rsb"), # RR B
    (0xCB19,"rsb"), # RR C
    (0xCB1A,"rsb"), # RR D
    (0xCB1B,"rsb"), # RR E
    (0xCB1C,"rsb"), # RR H
    (0xCB1D,"rsb"), # RR L
    (0xCB1E,"rsb"), # RR [HL]
    (0xCB1F,"rsb"), # RR A
    (0xCB20,"rsb"), # SLA B
    (0xCB21,"rsb"), # SLA C
    (0xCB22,"rsb"), # SLA D
    (0xCB23,"rsb"), # SLA E
    (0xCB24,"rsb"), # SLA H
    (0xCB25,"rsb"), # SLA L
    (0xCB26,"rsb"), # SLA [HL]
    (0xCB27,"rsb"), # SLA A
    (0xCB28,"rsb"), # SRA B
    (0xCB29,"rsb"), # SRA C
    (0xCB2A,"rsb"), # SRA D
    (0xCB2B,"rsb"), # SRA E
    (0xCB2C,"rsb"), # SRA H
    (0xCB2D,"rsb"), # SRA L
    (0xCB2E,"rsb"), # SRA [HL]
    (0xCB2F,"rsb"), # SRA A
    (0xCB30,"rsb"), # SWAP B
    (0xCB31,"rsb"), # SWAP C
    (0xCB32,"rsb"), # SWAP D
    (0xCB33,"rsb"), # SWAP E
    (0xCB34,"rsb"), # SWAP H
    (0xCB35,"rsb"), # SWAP L
    (0xCB36,"rsb"), # SWAP [HL]
    (0xCB37,"rsb"), # SWAP A
    (0xCB38,"rsb"), # SRL B
    (0xCB39,"rsb"), # SRL C
    (0xCB3A,"rsb"), # SRL D
    (0xCB3B,"rsb"), # SRL E
    (0xCB3C,"rsb"), # SRL H
    (0xCB3D,"rsb"), # SRL L
    (0xCB3E,"rsb"), # SRL [HL]
    (0xCB3F,"rsb"), # SRL A
    (0xCB40,"rsb"), # BIT 0, B
    (0xCB41,"rsb"), # BIT 0, C
    (0xCB42,"rsb"), # BIT 0, D
    (0xCB43,"rsb"), # BIT 0, E
    (0xCB44,"rsb"), # BIT 0, H
    (0xCB45,"rsb"), # BIT 0, L
    (0xCB46,"rsb"), # BIT 0, [HL]
    (0xCB47,"rsb"), # BIT 0, A
    (0xCB48,"rsb"), # BIT 1, B
    (0xCB49,"rsb"), # BIT 1, C
    (0xCB4A,"rsb"), # BIT 1, D
    (0xCB4B,"rsb"), # BIT 1, E
    (0xCB4C,"rsb"), # BIT 1, H
    (0xCB4D,"rsb"), # BIT 1, L
    (0xCB4E,"rsb"), # BIT 1, [HL]
    (0xCB4F,"rsb"), # BIT 1, A
    (0xCB50,"rsb"), # BIT 2, B
    (0xCB51,"rsb"), # BIT 2, C
    (0xCB52,"rsb"), # BIT 2, D
    (0xCB53,"rsb"), # BIT 2, E
    (0xCB54,"rsb"), # BIT 2, H
    (0xCB55,"rsb"), # BIT 2, L
    (0xCB56,"rsb"), # BIT 2, [HL]
    (0xCB57,"rsb"), # BIT 2, A
    (0xCB58,"rsb"), # BIT 3, B
    (0xCB59,"rsb"), # BIT 3, C
    (0xCB5A,"rsb"), # BIT 3, D
    (0xCB5B,"rsb"), # BIT 3, E
    (0xCB5C,"rsb"), # BIT 3, H
    (0xCB5D,"rsb"), # BIT 3, L
    (0xCB5E,"rsb"), # BIT 3, [HL]
    (0xCB5F,"rsb"), # BIT 3, A
    (0xCB60,"rsb"), # BIT 4, B
    (0xCB61,"rsb"), # BIT 4, C
    (0xCB62,"rsb"), # BIT 4, D
    (0xCB63,"rsb"), # BIT 4, E
    (0xCB64,"rsb"), # BIT 4, H
    (0xCB65,"rsb"), # BIT 4, L
    (0xCB66,"rsb"), # BIT 4, [HL]
    (0xCB67,"rsb"), # BIT 4, A
    (0xCB68,"rsb"), # BIT 5, B
    (0xCB69,"rsb"), # BIT 5, C
    (0xCB6A,"rsb"), # BIT 5, D
    (0xCB6B,"rsb"), # BIT 5, E
    (0xCB6C,"rsb"), # BIT 5, H
    (0xCB6D,"rsb"), # BIT 5, L
    (0xCB6E,"rsb"), # BIT 5, [HL]
    (0xCB6F,"rsb"), # BIT 5, A
    (0xCB70,"rsb"), # BIT 6, B
    (0xCB71,"rsb"), # BIT 6, C
    (0xCB72,"rsb"), # BIT 6, D
    (0xCB73,"rsb"), # BIT 6, E
    (0xCB74,"rsb"), # BIT 6, H
    (0xCB75,"rsb"), # BIT 6, L
    (0xCB76,"rsb"), # BIT 6, [HL]
    (0xCB77,"rsb"), # BIT 6, A
    (0xCB78,"rsb"), # BIT 7, B
    (0xCB79,"rsb"), # BIT 7, C
    (0xCB7A,"rsb"), # BIT 7, D
    (0xCB7B,"rsb"), # BIT 7, E
    (0xCB7C,"rsb"), # BIT 7, H
    (0xCB7D,"rsb"), # BIT 7, L
    (0xCB7E,"rsb"), # BIT 7, [HL]
    (0xCB7F,"rsb"), # BIT 7, A

    (0xCB80,"rsb"), # RES 0, B
    (0xCB81,"rsb"), # RES 0, C
    (0xCB82,"rsb"), # RES 0, D
    (0xCB83,"rsb"), # RES 0, E
    (0xCB84,"rsb"), # RES 0, H
    (0xCB85,"rsb"), # RES 0, L
    (0xCB86,"rsb"), # RES 0, [HL]
    (0xCB87,"rsb"), # RES 0, A
    (0xCB88,"rsb"), # RES 1, B
    (0xCB89,"rsb"), # RES 1, C
    (0xCB8A,"rsb"), # RES 1, D
    (0xCB8B,"rsb"), # RES 1, E
    (0xCB8C,"rsb"), # RES 1, H
    (0xCB8D,"rsb"), # RES 1, L
    (0xCB8E,"rsb"), # RES 1, [HL]
    (0xCB8F,"rsb"), # RES 1, A
    (0xCB90,"rsb"), # RES 2, B
    (0xCB91,"rsb"), # RES 2, C
    (0xCB92,"rsb"), # RES 2, D
    (0xCB93,"rsb"), # RES 2, E
    (0xCB94,"rsb"), # RES 2, H
    (0xCB95,"rsb"), # RES 2, L
    (0xCB96,"rsb"), # RES 2, [HL]
    (0xCB97,"rsb"), # RES 2, A
    (0xCB98,"rsb"), # RES 3, B
    (0xCB99,"rsb"), # RES 3, C
    (0xCB9A,"rsb"), # RES 3, D
    (0xCB9B,"rsb"), # RES 3, E
    (0xCB9C,"rsb"), # RES 3, H
    (0xCB9D,"rsb"), # RES 3, L
    (0xCB9E,"rsb"), # RES 3, [HL]
    (0xCB9F,"rsb"), # RES 3, A
    (0xCBA0,"rsb"), # RES 4, B
    (0xCBA1,"rsb"), # RES 4, C
    (0xCBA2,"rsb"), # RES 4, D
    (0xCBA3,"rsb"), # RES 4, E
    (0xCBA4,"rsb"), # RES 4, H
    (0xCBA5,"rsb"), # RES 4, L
    (0xCBA6,"rsb"), # RES 4, [HL]
    (0xCBA7,"rsb"), # RES 4, A
    (0xCBA8,"rsb"), # RES 5, B
    (0xCBA9,"rsb"), # RES 5, C
    (0xCBAA,"rsb"), # RES 5, D
    (0xCBAB,"rsb"), # RES 5, E
    (0xCBAC,"rsb"), # RES 5, H
    (0xCBAD,"rsb"), # RES 5, L
    (0xCBAE,"rsb"), # RES 5, [HL]
    (0xCBAF,"rsb"), # RES 5, A
    (0xCBB0,"rsb"), # RES 6, B
    (0xCBB1,"rsb"), # RES 6, C
    (0xCBB2,"rsb"), # RES 6, D
    (0xCBB3,"rsb"), # RES 6, E
    (0xCBB4,"rsb"), # RES 6, H
    (0xCBB5,"rsb"), # RES 6, L
    (0xCBB6,"rsb"), # RES 6, [HL]
    (0xCBB7,"rsb"), # RES 6, A
    (0xCBB8,"rsb"), # RES 7, B
    (0xCBB9,"rsb"), # RES 7, C
    (0xCBBA,"rsb"), # RES 7, D
    (0xCBBB,"rsb"), # RES 7, E
    (0xCBBC,"rsb"), # RES 7, H
    (0xCBBD,"rsb"), # RES 7, L
    (0xCBBE,"rsb"), # RES 7, [HL]
    (0xCBBF,"rsb"), # RES 7, A
    (0xCBC0,"rsb"), # SET 0, B
    (0xCBC1,"rsb"), # SET 0, C
    (0xCBC2,"rsb"), # SET 0, D
    (0xCBC3,"rsb"), # SET 0, E
    (0xCBC4,"rsb"), # SET 0, H
    (0xCBC5,"rsb"), # SET 0, L
    (0xCBC6,"rsb"), # SET 0, [HL]
    (0xCBC7,"rsb"), # SET 0, A
    (0xCBC8,"rsb"), # SET 1, B
    (0xCBC9,"rsb"), # SET 1, C
    (0xCBCA,"rsb"), # SET 1, D
    (0xCBCB,"rsb"), # SET 1, E
    (0xCBCC,"rsb"), # SET 1, H
    (0xCBCD,"rsb"), # SET 1, L
    (0xCBCE,"rsb"), # SET 1, [HL]
    (0xCBCF,"rsb"), # SET 1, A
    (0xCBD0,"rsb"), # SET 2, B
    (0xCBD1,"rsb"), # SET 2, C
    (0xCBD2,"rsb"), # SET 2, D
    (0xCBD3,"rsb"), # SET 2, E
    (0xCBD4,"rsb"), # SET 2, H
    (0xCBD5,"rsb"), # SET 2, L
    (0xCBD6,"rsb"), # SET 2, [HL]
    (0xCBD7,"rsb"), # SET 2, A
    (0xCBD8,"rsb"), # SET 3, B
    (0xCBD9,"rsb"), # SET 3, C
    (0xCBDA,"rsb"), # SET 3, D
    (0xCBDB,"rsb"), # SET 3, E
    (0xCBDC,"rsb"), # SET 3, H
    (0xCBDD,"rsb"), # SET 3, L
    (0xCBDE,"rsb"), # SET 3, [HL]
    (0xCBDF,"rsb"), # SET 3, A
    (0xCBE0,"rsb"), # SET 4, B
    (0xCBE1,"rsb"), # SET 4, C
    (0xCBE2,"rsb"), # SET 4, D
    (0xCBE3,"rsb"), # SET 4, E
    (0xCBE4,"rsb"), # SET 4, H
    (0xCBE5,"rsb"), # SET 4, L
    (0xCBE6,"rsb"), # SET 4, [HL]
    (0xCBE7,"rsb"), # SET 4, A
    (0xCBE8,"rsb"), # SET 5, B
    (0xCBE9,"rsb"), # SET 5, C
    (0xCBEA,"rsb"), # SET 5, D
    (0xCBEB,"rsb"), # SET 5, E
    (0xCBEC,"rsb"), # SET 5, H
    (0xCBED,"rsb"), # SET 5, L
    (0xCBEE,"rsb"), # SET 5, [HL]
    (0xCBEF,"rsb"), # SET 5, A
    (0xCBF0,"rsb"), # SET 6, B
    (0xCBF1,"rsb"), # SET 6, C
    (0xCBF2,"rsb"), # SET 6, D
    (0xCBF3,"rsb"), # SET 6, E
    (0xCBF4,"rsb"), # SET 6, H
    (0xCBF5,"rsb"), # SET 6, L
    (0xCBF6,"rsb"), # SET 6, [HL]
    (0xCBF7,"rsb"), # SET 6, A
    (0xCBF8,"rsb"), # SET 7, B
    (0xCBF9,"rsb"), # SET 7, C
    (0xCBFA,"rsb"), # SET 7, D
    (0xCBFB,"rsb"), # SET 7, E
    (0xCBFC,"rsb"), # SET 7, H
    (0xCBFD,"rsb"), # SET 7, L
    (0xCBFE,"rsb"), # SET 7, [HL]
    (0xCBFF,"rsb"), # SET 7, A
])

def check_illegal_address(address):
    # Accesses go through the memory map, so a 'flat map' doesn't really work
    # TODO: Maybe consider creating a virtual memory map class that supports the 'flat map' structure
    assert address <= 0xFFFF
    if (address >= 0xFEA0 and address <= 0xFEFF) or (address >= 0xE000 and address <= 0xFDFF):
        return True
    return False

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
        output_fn.write(f"#include <cpu/include/instructions.hpp>\n")
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

#define CheckRegisterWord(register, value) do {{ auto result = mmap.ReadWord(register); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); TEST_ASSERT_(static_cast<const Word&>(result) == value, "0x%X != 0x%X", static_cast<const Word&>(result).data, value); }} while(0)
#define CheckRegisterByte(register, value) do {{ auto result = mmap.ReadByte(register); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); TEST_ASSERT_(static_cast<const Byte&>(result) == value, "0x%X != 0x%X", static_cast<const Byte&>(result).data, value); }} while(0)
#define CheckRegisterFlag(value) do {{ TEST_ASSERT_(static_cast<const Byte>(mmap.ReadFlagByte()) == value, "0x%hhx != 0x%hhx", static_cast<const Byte>(mmap.ReadFlagByte()).data, value); }} while(0)
#define CheckMemory(address, value) do {{ auto result = mmap.ReadByte(address); TEST_ASSERT_(result.IsSuccess(), "%s", result.GetStatusDescription()); TEST_ASSERT_(static_cast<const Byte&>(result) == value, "%hhu != %hhu", static_cast<const Byte&>(result).data, value); }} while(0)

void test_check_implementation()
{{
    TEST_ASSERT(GetInstructionCallback(0x{opcode:02X}, 0x{prefix:02X}) != nullptr);
    TEST_ASSERT(GetInstructionTicks(0x{opcode:02X}, 0x{prefix:02X}) != 0);
}}

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
                    if not skip_test and check_illegal_address(address):
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
                        if not skip_test and check_illegal_address(address):
                            # Don't bother with tests that write to illegal locations
                            skip_test = True
                        test_content.append(f"    WriteMemory(0x{ram[0]:04X}, 0x{ram[1]:02X});")

            test_content.append(f"""
    // Execute step
    {{
        auto resultByte = mmap.ReadByte(mmap.ReadPC());
        TEST_ASSERT_(resultByte.IsSuccess(), "%s", resultByte.GetStatusDescription());
        auto ticks = ExecuteActiveDecoder(static_cast<const Byte&>(resultByte), mmap);
        TEST_ASSERT(ticks == GetInstructionTicks(static_cast<const Byte&>(resultByte), 0x00));""")
            if prefix == 0x00:
                test_content.append(f"""        TEST_ASSERT(ticks == {len(test['cycles']) * 4});""")
            else:
                test_content.append(f"""
        // Execute additional step for prefix
        auto resultByte2 = mmap.ReadByte(mmap.ReadPC());
        TEST_ASSERT_(resultByte2.IsSuccess(), "%s", resultByte2.GetStatusDescription());
        auto ticks2 = ExecuteActiveDecoder(static_cast<const Byte&>(resultByte2), mmap);
        TEST_ASSERT(ticks2 == GetInstructionTicks(static_cast<const Byte&>(resultByte2), 0x{prefix:02X}));
        TEST_ASSERT(ticks + ticks2 == {len(test['cycles']) * 4});""")

            test_content.append(f"""    }}

    // Final state
            """)

            for val in test['final']:
                if val in ("pc", "sp"):
                    address = test['final'][val]
                    if not skip_test and check_illegal_address(address):
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
                        if not skip_test and check_illegal_address(address):
                            # Don't bother with tests that write to illegal locations
                            skip_test = True
                        test_content.append(f"    CheckMemory(0x{ram[0]:04X}, 0x{ram[1]:02X});")

            test_content.append(f"}}")
            output_fn.write(f"const bool skip_test_{test_name} = {'true' if skip_test else 'false'};\n")

        for name in test_names:
            output_fn.write(f"void test_{name}(void);\n")

        output_fn.write(f"TEST_LIST = {{\n")
        output_fn.write(f"    {{ \"Check Implemented\", test_check_implementation }},\n")
        for name in test_names:
            output_fn.write(f"    {{ \"{name}\", test_{name} }},\n")
        output_fn.write(f"    {{NULL, NULL}}\n")
        output_fn.write(f"}};\n")

        for content in test_content:
            output_fn.write(content)
            output_fn.write("\n")
