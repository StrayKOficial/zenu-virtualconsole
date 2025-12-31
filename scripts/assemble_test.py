import struct

# Simple RISC-V Assembler for Zenu
def assemble_op_imm(rd, rs1, imm, funct3):
    # opcode 0010011 (0x13)
    return struct.pack("<I", (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0x13)

def assemble_lui(rd, imm):
    # opcode 0110111 (0x37)
    return struct.pack("<I", (imm << 12) | (rd << 7) | 0x37)

def assemble_store(rs2, rs1, imm, funct3):
    # opcode 0100011 (0x23)
    imm_low = imm & 0x1F
    imm_high = (imm >> 5) & 0x7F
    return struct.pack("<I", (imm_high << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_low << 7) | 0x23)

def assemble_load(rd, rs1, imm, funct3):
    # opcode 0000011 (0x03)
    return struct.pack("<I", (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0x03)

def assemble_jal(rd, imm):
    # opcode 1101111 (0x6F)
    imm_20 = (imm >> 20) & 0x1
    imm_10_1 = (imm >> 1) & 0x3FF
    imm_11 = (imm >> 11) & 0x1
    imm_19_12 = (imm >> 12) & 0xFF
    val = (imm_20 << 31) | (imm_10_1 << 21) | (imm_11 << 20) | (imm_19_12 << 12) | (rd << 7) | 0x6F
    return struct.pack("<I", val)

# Create a simple program that draws a moving bar based on input
rom = b""
rom += assemble_lui(10, 0x03000)      # x10 = VRAM (0x03000000)
rom += assemble_lui(11, 0x02000)      # x11 = INPUT (0x02000000)
rom += assemble_lui(12, 0x00FFF)      # x12 = WHITE (0x00FFF000... wait LUI is shift 12)
rom += assemble_op_imm(12, 0, 0xFFF, 0) # x12 = 0xFFF (Blueish)

# Loop:
# Read input
rom += assemble_load(13, 11, 0, 0)    # x13 = lbu(x11)
# Draw at 0,0 for now
rom += assemble_store(12, 10, 0, 2)   # sw x12, 0(x10)
# Simple loop
rom += assemble_jal(0, -8)            # j Loop (8 bytes back = Load + Store)

with open("rom.bin", "wb") as f:
    f.write(rom)
