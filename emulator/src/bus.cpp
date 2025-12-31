#include "bus.hpp"
#include <cstring>

Bus::Bus() : hle_bridge_data(0) {
    ram.resize(RAM_SIZE, 0);
    rom_area.resize(ROM_SIZE, 0);
    vram.resize(VRAM_SIZE, 0);
}

Bus::~Bus() {}

void Bus::load_rom(const std::vector<uint8_t>& data) {
    size_t size = std::min(data.size(), (size_t)ROM_SIZE);
    std::memcpy(rom_area.data(), data.data(), size);
}

uint8_t Bus::read8(uint32_t addr) {
    if (addr >= ROM_START && addr < ROM_START + ROM_SIZE) {
        return rom_area[addr - ROM_START];
    } else if (addr >= RAM_START && addr < RAM_START + RAM_SIZE) {
        return ram[addr - RAM_START];
    } else if (addr >= VRAM_START && addr < VRAM_START + VRAM_SIZE) {
        return vram[addr - VRAM_START];
    } else if (addr >= JOY_START && addr < JOY_START + JOY_SIZE) {
        return joy_state[addr - JOY_START];
    } else if (addr == HLE_BRIDGE) {
        return hle_bridge_data;
    } else if (addr >= APU_START && addr < APU_START + APU_SIZE) {
        if (apu) return apu->read8(addr - APU_START);
    }
    return 0;
}

void Bus::write8(uint32_t addr, uint8_t data) {
    if (addr >= RAM_START && addr < RAM_START + RAM_SIZE) {
        ram[addr - RAM_START] = data;
    } else if (addr >= VRAM_START && addr < VRAM_START + VRAM_SIZE) {
        vram[addr - VRAM_START] = data;
    } else if (addr >= JOY_START && addr < JOY_START + JOY_SIZE) {
        joy_state[addr - JOY_START] = data;
    } else if (addr == HLE_BRIDGE) {
        hle_bridge_data = data;
    } else if (addr >= APU_START && addr < APU_START + APU_SIZE) {
        if (apu) apu->write8(addr - APU_START, data);
    }
}

uint32_t Bus::read32(uint32_t addr) {
    // Basic little-endian read
    return (uint32_t)read8(addr) | 
           ((uint32_t)read8(addr + 1) << 8) | 
           ((uint32_t)read8(addr + 2) << 16) | 
           ((uint32_t)read8(addr + 3) << 24);
}

void Bus::write32(uint32_t addr, uint32_t data) {
    write8(addr,     (uint8_t)(data & 0xFF));
    write8(addr + 1, (uint8_t)((data >> 8) & 0xFF));
    write8(addr + 2, (uint8_t)((data >> 16) & 0xFF));
    write8(addr + 3, (uint8_t)((data >> 24) & 0xFF));
}
