import struct
import json
import os

# Zenu ROM Assembler (Improved with Labels)
def assemble(instructions):
    bin_data = b""
    for i in instructions:
        bin_data += i
    return bin_data

def i_type(opcode, rd, funct3, rs1, imm):
    return struct.pack("<I", ((imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode)

def s_type(opcode, funct3, rs1, rs2, imm):
    imm_low = imm & 0x1F
    imm_high = (imm >> 5) & 0x7F
    return struct.pack("<I", (imm_high << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_low << 7) | opcode)

def u_type(opcode, rd, imm):
    return struct.pack("<I", ((imm & 0xFFFFF) << 12) | (rd << 7) | opcode)

def j_type(opcode, rd, imm):
    imm = imm >> 1
    imm_20 = (imm >> 19) & 0x1
    imm_10_1 = imm & 0x3FF
    imm_11 = (imm >> 10) & 0x1
    imm_19_12 = (imm >> 11) & 0xFF
    return struct.pack("<I", (imm_20 << 31) | (imm_10_1 << 21) | (imm_11 << 20) | (imm_19_12 << 12) | (rd << 7) | opcode)

def b_type(opcode, funct3, rs1, rs2, imm):
    imm = imm >> 1
    imm_12 = (imm >> 11) & 0x1
    imm_11 = (imm >> 10) & 0x1
    imm_10_5 = (imm >> 4) & 0x3F
    imm_4_1 = imm & 0xF
    return struct.pack("<I", (imm_12 << 31) | (imm_10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_11 << 7) | (imm_4_1 << 8) | opcode)

def add(rd, rs1, rs2): return struct.pack("<I", (0 << 25) | (rs2 << 20) | (rs1 << 15) | (0 << 12) | (rd << 7) | 0x33)
def sltu(rd, rs1, rs2): return struct.pack("<I", (0 << 25) | (rs2 << 20) | (rs1 << 15) | (3 << 12) | (rd << 7) | 0x33)
def and_op(rd, rs1, rs2): return struct.pack("<I", (0 << 25) | (rs2 << 20) | (rs1 << 15) | (7 << 12) | (rd << 7) | 0x33)

def make_hello():
    rom = []
    # 0: Init
    rom.append(u_type(0x37, 10, 0x03000)) # x10 = VRAM (0x03000000)
    rom.append(u_type(0x37, 11, 0x02000)) # x11 = HW Base (0x02000000)
    rom.append(i_type(0x13, 12, 0, 0, 0))   # x12 = X Counter
    rom.append(u_type(0x37, 15, 0x000FF))   # x15 = Pure Red Color (0x000FF000)
    rom.append(i_type(0x13, 15, 0, 15, 0xFFF)) # x15 = Red-ish White

    # 20: Loop Start (X movement)
    # Clear previous line (simple: clear the whole first row)
    # But for a demo, let's just draw a trail
    rom.append(i_type(0x13, 13, 1, 12, 2))  # x13 = X << 2
    rom.append(add(14, 10, 13))            # x14 = VRAM + offset
    rom.append(s_type(0x23, 2, 14, 15, 0)) # Store pixel

    # Update X
    rom.append(i_type(0x13, 12, 0, 12, 1)) # X++
    rom.append(i_type(0x13, 16, 0, 0, 160)) # Limit 160
    rom.append(sltu(17, 12, 16))           # x17 = (X < 160)
    
    # Branch: if (X < 160) jump back to Loop Start (offset -12)
    # PC is at 44. Target is 20. Offset is -24.
    rom.append(b_type(0x63, 1, 17, 0, -24)) # bne x17, 0, Loop (PC 20)
    
    # Reset X and loop forever
    rom.append(i_type(0x13, 12, 0, 0, 0))
    rom.append(j_type(0x6F, 0, -32)) # Jump back to Loop Start (PC 20)

    os.makedirs("roms/hello.boc", exist_ok=True)
    with open("roms/hello.boc/rom.bin", "wb") as f:
        f.write(assemble(rom))
    with open("roms/hello.boc/manifest.json", "w") as f:
        json.dump({"name": "Zenu Moving Line", "version": "1.1"}, f)

def make_tetris():
    rom = []
    # This ROM uses the "HLE Bridge" at 0x0200FFF0
    # 0x02010000 - 16 = 0x0200FFF0
    rom.append(u_type(0x37, 10, 0x02010)) # x10 = 0x02010000
    rom.append(i_type(0x13, 11, 0, 0, 1))
    
    # Loop: write 1 to 0x0200FFF0
    # 8:
    rom.append(s_type(0x23, 0, 10, 11, -16)) # sb x11, -16(x10) -> 0x0200FFF0
    rom.append(j_type(0x6F, 0, -4)) # Jump back to sb (PC 8)

    os.makedirs("roms/tetris.boc", exist_ok=True)
    with open("roms/tetris.boc/rom.bin", "wb") as f:
        f.write(assemble(rom))
    with open("roms/tetris.boc/manifest.json", "w") as f:
        json.dump({"name": "Zenu Tetris (Pocket)", "version": "2.0"}, f)

make_hello()
make_tetris()
print("ROMs generated: hello.boc and tetris.boc")
