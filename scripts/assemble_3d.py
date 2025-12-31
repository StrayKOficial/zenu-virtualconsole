import struct

# Zenu LLE Assembler v2
# Registers:
# x10: GPU_CTRL_BASE (0x03FF0000)
# x11: VRAM_BASE (0x03000000)
# x12: Frame Counter / Angle
# x13-x20: Temp math

def i_type(opcode, rd, funct3, rs1, imm):
    return struct.pack("<I", (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode)

def s_type(opcode, funct3, rs1, rs2, imm):
    imm_low = imm & 0x1F
    imm_high = (imm >> 5) & 0x7F
    return struct.pack("<I", (imm_high << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_low << 7) | opcode)

def b_type(opcode, funct3, rs1, rs2, imm):
    imm_12 = (imm >> 12) & 0x1
    imm_10_5 = (imm >> 5) & 0x3F
    imm_4_1 = (imm >> 1) & 0xF
    imm_11 = (imm >> 11) & 0x1
    return struct.pack("<I", (imm_12 << 31) | (imm_10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_11 << 7) | (imm_4_1 << 8) | opcode)

def u_type(opcode, rd, imm):
    return struct.pack("<I", (imm << 12) | (rd << 7) | opcode)

def j_type(opcode, rd, imm):
    imm_20 = (imm >> 20) & 0x1
    imm_10_1 = (imm >> 1) & 0x3FF
    imm_11 = (imm >> 11) & 0x1
    imm_19_12 = (imm >> 12) & 0xFF
    return struct.pack("<I", (imm_20 << 31) | (imm_10_1 << 21) | (imm_11 << 20) | (imm_19_12 << 12) | (rd << 7) | opcode)

rom = b""

# 1. Setup GPU Mode 2 (Primitive Mode)
rom += u_type(0x37, 10, 0x03FF0) # x10 = 0x03FF0000 (GPU_CTRL)
rom += i_type(0x13, 11, 0, 0, 2)    # x11 = 2 (Mode 2)
rom += s_type(0x23, 2, 10, 11, 12) # sw x11, 12(x10) -> Set Mode to 2

# 2. Main Loop
# x12 = X position (starts at 160)
rom += i_type(0x13, 12, 0, 0, 160)

# Loop:
# Draw a triangle
# Cmd (0x20) = 2 (Triangle)
rom += i_type(0x13, 13, 0, 0, 2)
rom += s_type(0x23, 2, 10, 13, 0x20)

# Vertices (Simplified test - a fixed triangle that moves)
# x1, y1 (160, 50)
rom += i_type(0x13, 13, 0, 0, 160)
rom += s_type(0x23, 1, 10, 13, 0x24) # sh x13, 0x24(x10)
rom += i_type(0x13, 13, 0, 0, 50)
rom += s_type(0x23, 1, 10, 13, 0x26) # sh x13, 0x26(x10)

# x2, y2 (100, 150)
rom += i_type(0x13, 13, 0, 0, 100)
rom += s_type(0x23, 1, 10, 13, 0x28) 
rom += i_type(0x13, 13, 0, 0, 150)
rom += s_type(0x23, 1, 10, 13, 0x2A)

# x3, y3 (220, 150)
rom += i_type(0x13, 13, 0, 0, 220)
rom += s_type(0x23, 1, 10, 13, 0x2C)
rom += i_type(0x13, 13, 0, 0, 150)
rom += s_type(0x23, 1, 10, 13, 0x2E)

# Color (0x00FF00FF - Magenta)
rom += u_type(0x37, 13, 0x00FF0)
rom += i_type(0x13, 13, 0, 13, 0x0FF)
rom += s_type(0x23, 2, 10, 13, 0x30)

# Jump back to Loop (Infinite loop)
rom += j_type(0x6F, 0, -56) # Adjust offset carefully

with open("rom.bin", "wb") as f:
    f.write(rom)
