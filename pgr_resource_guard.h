#pragma once
#include "cpu.h"
#include "loader.h"
#include <cstdio>

inline bool InstallRaceResourceGuard(Cpu& cpu) {
    const uint32_t load = G(0x4a066c04);
    const uint32_t caller = G(0x4a08a604);
    uint32_t instructions[3]{};
    uint32_t call = 0;
    if (cpu.MemRead(load, instructions, sizeof instructions) != Cpu::OK ||
        instructions[0] != 0xe5900024 || instructions[1] != 0xe3500000 ||
        instructions[2] != 0x11d011b8 ||
        cpu.MemRead(caller, &call, sizeof call) != Cpu::OK || call != 0xebff7164)
        return false;

    const bool installed = cpu.AddCodeHook([](Cpu* cpu, uint64_t address, uint32_t, void*) {
        uint32_t resource = 0, value = 0, stack = 0, returnAddress = 0;
        cpu->RegRead(Cpu::REG_R0, &resource);
        if (cpu->MemRead(uint64_t(resource) + 0x24, &value, sizeof value) != Cpu::OK || value == 0)
            return;
        uint16_t referenceCount = 0;
        const uint64_t countAddress = uint64_t(value) + 0x18;
        if ((countAddress & 1) == 0 &&
            cpu->MemRead(countAddress, &referenceCount, sizeof referenceCount) == Cpu::OK)
            return;
        cpu->RegRead(Cpu::REG_SP, &stack);
        // The cleanup routine saves six registers, with the caller's LR last.
        if (cpu->MemRead(uint64_t(stack) + 20, &returnAddress, sizeof returnAddress) != Cpu::OK ||
            returnAddress != G(0x4a08a608))
            return;

        // Follow the game's existing no-reference path for this malformed field.
        // The release callback and resource-table bookkeeping still run.
        const uint32_t absent = 0;
        const uint32_t next = static_cast<uint32_t>(address) + 4;
        cpu->RegWrite(Cpu::REG_R0, &absent);
        cpu->RegWrite(Cpu::REG_PC, &next);
        std::fprintf(stderr, "[RaceResourceGuard] Ignored invalid reference %08x on resource %08x\n",
                     static_cast<unsigned>(value), static_cast<unsigned>(resource));
    }, nullptr, load, load) == Cpu::OK;
    if (installed) std::fprintf(stderr, "[RaceResourceGuard] Test 2 installed\n");
    return installed;
}
