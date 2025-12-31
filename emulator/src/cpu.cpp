#include "cpu.hpp"
#include <iostream>

CPU::CPU(Bus& bus) : bus(bus) {
    reset();
}

CPU::~CPU() {}

void CPU::reset() {
    pc = 0x00010000; // Entry point in ROM
    for (int i = 0; i < 32; i++) regs[i] = 0;
}

void CPU::step(bool debug) {
    uint32_t instr = bus.read32(pc);
    if (debug) {
        std::cout << "0x" << std::hex << pc << ": " << disassemble(instr) << std::dec << std::endl;
    }
    uint32_t current_pc = pc;
    pc += 4;
    execute(instr);
    regs[0] = 0;
}

void CPU::execute(uint32_t instr) {
    uint32_t opcode = instr & 0x7F;
    uint32_t rd = (instr >> 7) & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x07;
    uint32_t rs1 = (instr >> 15) & 0x1F;
    uint32_t rs2 = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    int32_t imm_i = (int32_t)instr >> 20;
    
    switch (opcode) {
        case 0x37: // LUI
            regs[rd] = instr & 0xFFFFF000;
            break;
        case 0x17: // AUIPC
            regs[rd] = (pc - 4) + (instr & 0xFFFFF000);
            break;
        case 0x6F: // JAL
            {
                int32_t j_imm = (((instr >> 31) & 0x1) << 20) |
                                (((instr >> 21) & 0x3FF) << 1) |
                                (((instr >> 20) & 0x1) << 11) |
                                (((instr >> 12) & 0xFF) << 12);
                if (j_imm & (1 << 20)) j_imm |= 0xFFF00000;
                regs[rd] = pc;
                pc = (pc - 4) + j_imm;
            }
            break;
        case 0x67: // JALR
            {
                uint32_t target = (regs[rs1] + imm_i) & ~1;
                regs[rd] = pc;
                pc = target;
            }
            break;
        case 0x63: // BRANCH
            {
                int32_t imm_b = (((instr >> 31) & 0x1) << 12) |
                                (((instr >> 25) & 0x3F) << 5) |
                                (((instr >> 8) & 0xF) << 1) |
                                (((instr >> 7) & 0x1) << 11);
                if (imm_b & 0x1000) imm_b |= 0xFFFFE000;
                bool take = false;
                switch (funct3) {
                    case 0x0: if (regs[rs1] == regs[rs2]) take = true; break; // BEQ
                    case 0x1: if (regs[rs1] != regs[rs2]) take = true; break; // BNE
                    case 0x4: if ((int32_t)regs[rs1] < (int32_t)regs[rs2]) take = true; break; // BLT
                    case 0x5: if ((int32_t)regs[rs1] >= (int32_t)regs[rs2]) take = true; break; // BGE
                    case 0x6: if (regs[rs1] < regs[rs2]) take = true; break; // BLTU
                    case 0x7: if (regs[rs1] >= regs[rs2]) take = true; break; // BGEU
                }
                if (take) pc = (pc - 4) + imm_b;
            }
            break;
        case 0x03: // LOAD
            {
                uint32_t addr = regs[rs1] + imm_i;
                switch (funct3) {
                    case 0x0: regs[rd] = (int32_t)(int8_t)bus.read8(addr); break; // LB
                    case 0x1: regs[rd] = (int32_t)(int16_t)bus.read32(addr); break; // LH (dummy)
                    case 0x2: regs[rd] = bus.read32(addr); break; // LW
                    case 0x4: regs[rd] = bus.read8(addr); break; // LBU
                    case 0x5: regs[rd] = (uint16_t)bus.read32(addr); break; // LHU
                }
            }
            break;
        case 0x23: // STORE
            {
                int32_t imm_s = (((instr >> 25) & 0x7F) << 5) | ((instr >> 7) & 0x1F);
                if (imm_s & 0x800) imm_s |= 0xFFFFF000;
                uint32_t addr = regs[rs1] + imm_s;
                switch (funct3) {
                    case 0x0: bus.write8(addr, (uint8_t)regs[rs2]); break; // SB
                    case 0x1: bus.write8(addr, (uint8_t)regs[rs2]); bus.write8(addr+1, (uint8_t)(regs[rs2]>>8)); break; // SH
                    case 0x2: bus.write32(addr, regs[rs2]); break; // SW
                }
            }
            break;
        case 0x13: // OP-IMM
            switch (funct3) {
                case 0x0: regs[rd] = regs[rs1] + imm_i; break; // ADDI
                case 0x2: regs[rd] = ((int32_t)regs[rs1] < imm_i) ? 1 : 0; break; // SLTI
                case 0x3: regs[rd] = (regs[rs1] < (uint32_t)imm_i) ? 1 : 0; break; // SLTIU
                case 0x4: regs[rd] = regs[rs1] ^ imm_i; break; // XORI
                case 0x6: regs[rd] = regs[rs1] | imm_i; break; // ORI
                case 0x7: regs[rd] = regs[rs1] & imm_i; break; // ANDI
                case 0x1: regs[rd] = regs[rs1] << (rs2 & 0x1F); break; // SLLI
                case 0x5: 
                    if (funct7 == 0x00) regs[rd] = regs[rs1] >> (rs2 & 0x1F); // SRLI
                    else regs[rd] = (int32_t)regs[rs1] >> (rs2 & 0x1F); // SRAI
                    break;
            }
            break;
        case 0x33: // OP
            switch (funct3) {
                case 0x0: 
                    if (funct7 == 0x00) regs[rd] = regs[rs1] + regs[rs2]; // ADD
                    else if (funct7 == 0x20) regs[rd] = regs[rs1] - regs[rs2]; // SUB
                    else if (funct7 == 0x01) regs[rd] = (int32_t)regs[rs1] * (int32_t)regs[rs2]; // MUL (RV32M shortcut)
                    break;
                case 0x1: regs[rd] = regs[rs1] << (regs[rs2] & 0x1F); break; // SLL
                case 0x2: regs[rd] = ((int32_t)regs[rs1] < (int32_t)regs[rs2]) ? 1 : 0; break; // SLT
                case 0x3: regs[rd] = (regs[rs1] < regs[rs2]) ? 1 : 0; break; // SLTU
                case 0x4: 
                    if (funct7 == 0x00) regs[rd] = regs[rs1] ^ regs[rs2]; // XOR
                    else if (funct7 == 0x01) regs[rd] = (int32_t)regs[rs1] / (int32_t)regs[rs2]; // DIV
                    break;
                case 0x5:
                    if (funct7 == 0x00) regs[rd] = regs[rs1] >> (regs[rs2] & 0x1F); // SRL
                    else if (funct7 == 0x20) regs[rd] = (int32_t)regs[rs1] >> (regs[rs2] & 0x1F); // SRA
                    else if (funct7 == 0x01) regs[rd] = regs[rs1] / regs[rs2]; // DIVU
                    break;
                case 0x6: 
                    if (funct7 == 0x00) regs[rd] = regs[rs1] | regs[rs2]; // OR
                    else if (funct7 == 0x01) regs[rd] = (int32_t)regs[rs1] % (int32_t)regs[rs2]; // REM
                    break;
                case 0x7: 
                    if (funct7 == 0x00) regs[rd] = regs[rs1] & regs[rs2]; // AND
                    else if (funct7 == 0x01) regs[rd] = regs[rs1] % regs[rs2]; // REMU
                    break;
            }
            break;
    }
}

std::string CPU::disassemble(uint32_t instr) {
    uint32_t opcode = instr & 0x7F;
    uint32_t rd = (instr >> 7) & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x07;
    uint32_t rs1 = (instr >> 15) & 0x1F;
    uint32_t rs2 = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;
    int32_t imm_i = (int32_t)instr >> 20;

    char buf[128];
    switch (opcode) {
        case 0x37: sprintf(buf, "lui x%d, 0x%x", rd, (instr >> 12)); break;
        case 0x17: sprintf(buf, "auipc x%d, 0x%x", rd, (instr >> 12)); break;
        case 0x6F: sprintf(buf, "jal x%d, offset", rd); break;
        case 0x67: sprintf(buf, "jalr x%d, x%d, %d", rd, rs1, imm_i); break;
        case 0x63: sprintf(buf, "beq/bne x%d, x%d, offset", rs1, rs2); break;
        case 0x03: sprintf(buf, "l%c x%d, %d(x%d)", "bhw??u"[funct3], rd, imm_i, rs1); break;
        case 0x23: sprintf(buf, "s%c x%d, %d(x%d)", "bhw"[funct3], rs2, 0, rs1); break; // Simplified
        case 0x13: 
            if (funct3 == 0) sprintf(buf, "addi x%d, x%d, %d", rd, rs1, imm_i);
            else sprintf(buf, "op-imm x%d, x%d, %d", rd, rs1, funct3);
            break;
        case 0x33: 
            if (funct7 == 0x01) sprintf(buf, "mul/div x%d, x%d, x%d", rd, rs1, rs2);
            else sprintf(buf, "op x%d, x%d, x%d", rd, rs1, rs2);
            break;
        default: sprintf(buf, "unknown (0x%x)", instr); break;
    }
    return std::string(buf);
}
