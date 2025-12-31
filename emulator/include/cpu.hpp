#ifndef CPU_HPP
#define CPU_HPP

#include "bus.hpp"
#include <cstdint>
#include <vector>

class CPU {
public:
    CPU(Bus& bus);
    ~CPU();

    void reset();
    void step(bool debug = false);
    std::string disassemble(uint32_t instr);

    // Registers
    uint32_t pc;
    uint32_t regs[32];

private:
    Bus& bus;

    void execute(uint32_t instr);
};

#endif
