#include "Chip8.h"
#include <cstring>
#include <bitset>
#include <iostream>

Chip8::Chip8(uint8_t *rom, uint16_t romSize) : pc(0x200),
                                               I(0),
                                               distribution(0, 255),
                                               delay_timer(0),
                                               waiting_for_key(-1),
                                               opcodes{
                                                   {0, &Chip8::op_system},
                                                   {1, &Chip8::op_goto},
                                                   {2, &Chip8::op_call},
                                                   {3, &Chip8::op_skip_ceq},
                                                   {4, &Chip8::op_skip_cneq},
                                                   {5, &Chip8::op_skip_eq},
                                                   {6, &Chip8::op_set},
                                                   {7, &Chip8::op_inc},
                                                   {8, &Chip8::op_arithmetic},
                                                   {9, &Chip8::op_skip_neq},
                                                   {10, &Chip8::op_set_i},
                                                   {11, &Chip8::op_goto_plus_v0},
                                                   {12, &Chip8::op_random},
                                                   {13, &Chip8::op_draw},
                                                   {14, &Chip8::op_key},
                                                   {15, &Chip8::op_reg}}
{
    static const uint8_t fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, //0
        0x20, 0x60, 0x20, 0x20, 0x70, //1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
        0x90, 0x90, 0xF0, 0x10, 0x10, //4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
        0xF0, 0x10, 0x20, 0x40, 0x40, //7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
        0xF0, 0x90, 0xF0, 0x90, 0x90, //A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
        0xF0, 0x80, 0x80, 0x80, 0xF0, //C
        0xE0, 0x90, 0x90, 0x90, 0xE0, //D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
        0xF0, 0x80, 0xF0, 0x80, 0x80  //F
    };
    memset(memory, 0, 0x1000);
    memset(screen, 0, 64 * 32);

    memcpy(memory, fontset, sizeof(fontset));
    if (romSize >= 0x1000 - 0x200)
    {
        return;
    }
    memcpy(memory + 0x200, rom, romSize);
}

Instruction Chip8::cycle()
{
    if (delay_timer)
        --delay_timer;
    if (waiting_for_key == -1)
    {
        Instruction instr(memory[pc] << 8 | memory[pc + 1]);
        pc += 2;
        (opcodes[instr.h3])(*this, instr);
		return instr;
    }
	return Instruction(0);
}

void Chip8::op_system(Instruction inst)
{
    if (inst.w0 == 0x00EE)
    {
        if (!stack.empty())
        {
            pc = stack.top();
            stack.pop();
        }
        else
        {
            pc = 0x200;
        }
    }
    else if(inst.w0 == 0x00E0)
    {
        memset(screen, 0, sizeof(screen));
    }
}

void Chip8::op_goto(Instruction inst)
{
    pc = inst.a0;
}

void Chip8::op_call(Instruction inst)
{
    stack.push(pc);
    pc = inst.a0;
}

void Chip8::op_skip_ceq(Instruction inst)
{
    if (v[inst.h2] == inst.b0)
        pc += 2;
}

void Chip8::op_skip_cneq(Instruction inst)
{
    if (v[inst.h2] != inst.b0)
        pc += 2;
}

void Chip8::op_skip_eq(Instruction inst)
{
    if (v[inst.h2] == v[inst.h1])
        pc += 2;
}

void Chip8::op_set(Instruction inst)
{
    v[inst.h2] = inst.b0;
}

void Chip8::op_inc(Instruction inst)
{
    v[inst.h2] += inst.b0;
}

void Chip8::op_arithmetic(Instruction inst)
{
    uint8_t *vx = v + inst.h2;
    uint8_t vy = v[inst.h1];
    uint8_t store = *vx;

    switch (inst.h0)
    {
    case 0: //assign
        *vx = vy;
        break;
    case 1: //or
        *vx |= vy;
        break;
    case 2: //and
        *vx &= vy;
        break;
    case 3: //xor
        *vx ^= vy;
        break;
    case 4: //add
        *vx += vy;
        v[0xF] = store > *vx;
        break;
    case 5: //sub
        *vx -= vy;
        v[0xF] = store > *vx;
        break;
    case 6: //shift right
        v[0xF] = *vx & 1;
        *vx >>= 1;
        break;
    case 0xE: //shift left
        v[0xF] = *v & 0b10000000;
        *vx <<= 1;
        break;
    case 7: // rev sub
        *vx = vy - *vx;
        v[0xF] = vy > *vx;
        break;
    }
}

void Chip8::op_skip_neq(Instruction inst)
{
    if (v[inst.h1] != v[inst.h2])
    {
        pc += 2;
    }
}

void Chip8::op_set_i(Instruction inst)
{
    I = inst.a0;
}

void Chip8::op_goto_plus_v0(Instruction inst)
{
    pc = inst.a0 + v[0];
}

void Chip8::op_random(Instruction inst)
{
    v[inst.h2] = distribution(random_engine) & inst.b0;
}

void Chip8::op_draw(Instruction inst)
{
    uint8_t x = v[inst.h2];
    uint8_t y = v[inst.h1];
    uint8_t height = inst.h0;
    v[0xF] = 0;
    for (int i = 0; i < height; i++)
    {
        std::bitset<8> line(memory[I + i]);
        for (int j = 7; j >= 0; --j)
        {
            bool prev = screen[x + i][y + j];
            screen[x + i][y + j] ^= line[j];
            if (prev && !(screen[x + i][y + j]))
            {
                v[0xF] = 1;
            }
        }
    }
}

void Chip8::op_key(Instruction inst)
{
    switch (inst.b0)
    {
    case 0x9E:
        if (keys[v[inst.h2]])
            pc += 2;
        break;
    case 0xA1:
        if (!keys[v[inst.h2]])
            pc += 2;
        break;
    }
}

void Chip8::op_reg(Instruction inst)
{
    uint8_t *vx = v + inst.h2;

    uint8_t hundreds, tenths, ones;
    switch (inst.b0)
    {
    case 0x07: // get delay_timer
        *vx = delay_timer;
        break;
    case 0x0A: // wait for key press
        waiting_for_key = inst.h2;
        break;
    case 0x15: // set delay_timer
        delay_timer = *vx;
        break;
    case 0x18: // sound timer
        break; // TODO
    case 0x1E:
        I += *vx;
        break;
    case 0x29:
        I = *vx * 5;
        break;
    case 0x33:
        hundreds = *vx / 100;
        tenths = *vx % 100 / 10;
        ones = *vx % 10;
        memory[I] = hundreds;
        memory[I + 1] = tenths;
        memory[I + 2] = ones;
        break;
    case 0x55:
        for (int i = 0; i <= inst.h2; ++i)
        {
            memory[I + i] = v[i];
        }
        break;
    case 0x65:
        for (int i = 0; i <= inst.h2; ++i)
        {
            v[i] = memory[I + i];
        }
        break;
    }
}

void Chip8::press_key(uint8_t key)
{
    if (waiting_for_key != -1)
    {
        v[waiting_for_key] = key;
        waiting_for_key = -1;
    }
    keys[key] = true;
}

void Chip8::release_key(uint8_t key)
{
    keys[key] = false;
}

bool Chip8::get_pixel(uint8_t x, uint8_t y)
{
    return screen[x][y];
}
