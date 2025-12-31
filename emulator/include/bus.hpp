#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include <cstdint>
#include <iostream>
#include "apu.hpp"

class Bus {
public:
    Bus();
    ~Bus();

    void set_apu(APU* a) { apu = a; }

    // Memory Access
    uint32_t read32(uint32_t addr);
    void write32(uint32_t addr, uint32_t data);

    uint8_t read8(uint32_t addr);
    void write8(uint32_t addr, uint8_t data);

    // RAM Access
    void load_rom(const std::vector<uint8_t>& data);
    uint8_t* get_vram_ptr() { return vram.data(); }

private:
    std::vector<uint8_t> ram;      // 16 MB WRAM (0x01000000)
    std::vector<uint8_t> rom_area; // 16 MB ROM Area (0x00010000)
    std::vector<uint8_t> vram;     // 16 MB VRAM (0x03000000)
    uint8_t joy_state[256];        // Input (0x02000000)
    
    // Constants for memory mapping
    static constexpr uint32_t ROM_START = 0x00010000;
    static constexpr uint32_t ROM_SIZE  = 0x00FF0000; 
    static constexpr uint32_t RAM_START = 0x01000000;
    static constexpr uint32_t RAM_SIZE  = 0x01000000; 
    static constexpr uint32_t VRAM_START = 0x03000000;
    static constexpr uint32_t VRAM_SIZE  = 0x01000000;
    static constexpr uint32_t JOY_START = 0x02000000;
    static constexpr uint32_t JOY_SIZE  = 0x00000100;
    static constexpr uint32_t APU_START = 0x02000100;
    static constexpr uint32_t APU_SIZE  = 0x00000100;
    static constexpr uint32_t HLE_BRIDGE = 0x0200FFF0;

    APU* apu = nullptr;
    uint8_t hle_bridge_data = 0;
};

#endif
