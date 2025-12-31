import struct

# Zenu LLE Assembler - Moving Pixel Demo
# Registers:
# x10: VRAM_BASE (0x03000000)
# x11: X pos
# x12: Y pos
# x13: DX
# x14: DY
# x15: Temp / Color

def i_type(opcode, rd, funct3, rs1, imm):
    return struct.pack("<I", ((imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode)

def s_type(opcode, funct3, rs1, rs2, imm):
    imm_low = imm & 0x1F
    imm_high = (imm >> 5) & 0x7F
    return struct.pack("<I", (imm_high << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_low << 7) | opcode)

def b_type(opcode, funct3, rs1, rs2, imm):
    imm = imm >> 1
    imm_12 = (imm >> 11) & 0x1
    imm_11 = (imm >> 10) & 0x1
    imm_10_5 = (imm >> 4) & 0x3F
    imm_4_1 = imm & 0xF
    return struct.pack("<I", (imm_12 << 31) | (imm_10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_11 << 7) | (imm_4_1 << 8) | opcode)

def u_type(opcode, rd, imm):
    return struct.pack("<I", ((imm & 0xFFFFF) << 12) | (rd << 7) | opcode)

def j_type(opcode, rd, imm):
    imm = imm >> 1
    imm_20 = (imm >> 19) & 0x1
    imm_10_1 = imm & 0x3FF
    imm_11 = (imm >> 10) & 0x1
    imm_19_12 = (imm >> 11) & 0xFF
    return struct.pack("<I", (imm_20 << 31) | (imm_10_1 << 21) | (imm_11 << 20) | (imm_19_12 << 12) | (rd << 7) | opcode)

rom = b""

# Init
rom += u_type(0x37, 10, 0x03000) # x10 = 0x03000000 (VRAM)
rom += i_type(0x13, 11, 0, 0, 160) # x11 = X = 160
rom += i_type(0x13, 12, 0, 0, 120) # x12 = Y = 120
rom += i_type(0x13, 13, 0, 0, 1)   # x13 = DX = 1
rom += i_type(0x13, 14, 0, 0, 1)   # x14 = DY = 1
rom += u_type(0x37, 15, 0xFFFFF) # x15 = Color White

# Loop Start
# Draw pixel: offset = (Y * 320 + X) * 4
# We don't have MUL, so we use shift + add (320 = 256 + 64)
# Y * 256 = Y << 8
# Y * 64 = Y << 6
rom += i_type(0x13, 16, 1, 12, 8)  # x16 = Y << 8
rom += i_type(0x13, 17, 1, 12, 6)  # x17 = Y << 6
rom += struct.pack("<I", (0x00 << 25) | (17 << 20) | (16 << 15) | (0 << 12) | (18 << 7) | 0x33) # x18 = x16 + x17 (Y*320)
rom += struct.pack("<I", (0x00 << 25) | (11 << 20) | (18 << 15) | (0 << 12) | (18 << 7) | 0x33) # x18 = x18 + X
rom += i_type(0x13, 18, 1, 18, 2)  # x18 = x18 << 2 (offset in bytes)
rom += struct.pack("<I", (0x00 << 25) | (18 << 20) | (10 << 15) | (0 << 12) | (19 << 7) | 0x33) # x19 = VRAM + offset

rom += s_type(0x23, 2, 19, 15, 0) # sw x15, 0(x19)

# Update positions
rom += struct.pack("<I", (0x00 << 25) | (13 << 20) | (11 << 15) | (0 << 12) | (11 << 7) | 0x33) # X += DX
rom += struct.pack("<I", (0x00 << 25) | (14 << 20) | (12 << 15) | (0 << 12) | (12 << 7) | 0x33) # Y += DY

# Bounce X
rom += i_type(0x13, 20, 2, 11, 0)  # slt x20, x11, 0
rom += b_type(0x63, 1, 20, 0, 12)  # bne x20, x0, NegX
rom += i_type(0x13, 21, 0, 0, 319)
rom += struct.pack("<I", (0x00 << 25) | (11 << 20) | (21 << 15) | (2 << 12) | (20 << 7) | 0x33) # slt x20, x21, X (X > 319)
rom += b_type(0x63, 0, 20, 0, 8)   # beq x20, x0, SkipX
# NegX:
rom += struct.pack("<I", (0x20 << 25) | (13 << 20) | (0 << 15) | (0 << 12) | (13 << 7) | 0x33) # DX = -DX
# SkipX:

# Bounce Y
rom += i_type(0x13, 20, 2, 12, 0)  # slt x20, x12, 0
rom += b_type(0x63, 1, 20, 0, 12)  # bne x20, x0, NegY
rom += i_type(0x13, 21, 0, 0, 239)
rom += struct.pack("<I", (0x00 << 25) | (12 << 20) | (21 << 15) | (2 << 12) | (20 << 7) | 0x33) # slt x20, x21, Y (Y > 239)
rom += b_type(0x63, 0, 20, 0, 8)   # beq x20, x0, SkipY
# NegY:
rom += struct.pack("<I", (0x20 << 25) | (14 << 20) | (0 << 15) | (0 << 12) | (14 << 7) | 0x33) # DY = -DY
# SkipY:

# Loop forever
rom += j_type(0x6F, 0, -84) # 21 instructions back

with open("rom.bin", "wb") as f:
    f.write(rom)
print("ROM generated successfully.")
