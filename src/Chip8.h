#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <functional>
#include <random>
#include <stack>
#include <map>

#include "Instruction.h"

class Chip8
{
private:
    uint8_t v[16];
    std::stack<uint16_t> stack;
    uint8_t memory[0x1000];
    uint16_t pc;
    bool screen[64][32];
    uint16_t I;
    uint16_t delay_timer;
    bool keys[16];
    int8_t waiting_for_key;

    std::default_random_engine random_engine;
    std::uniform_int_distribution<uint8_t> distribution;

    void op_system(Instruction);
    void op_goto(Instruction);
    void op_call(Instruction);
    void op_skip_ceq(Instruction);
    void op_skip_cneq(Instruction);
    void op_skip_eq(Instruction);
    void op_set(Instruction);
    void op_inc(Instruction);
    void op_arithmetic(Instruction);
    void op_skip_neq(Instruction);
    void op_set_i(Instruction);
    void op_goto_plus_v0(Instruction);
    void op_random(Instruction);
    void op_draw(Instruction);
    void op_key(Instruction);
    void op_reg(Instruction);
    std::map<uint8_t, std::function<void(Chip8&, Instruction)>> opcodes;

public:
    Chip8(uint8_t *, uint16_t);
    Instruction cycle();
    bool get_pixel(uint8_t x, uint8_t y);

    enum Internal {
		Chip8I,
		Chip8PC
	};

    void press_key(uint8_t);
    void release_key(uint8_t);
	uint8_t& V(uint8_t);
	uint8_t& mem(uint16_t);
	uint16_t& refI(Chip8::Internal);
};
#endif
