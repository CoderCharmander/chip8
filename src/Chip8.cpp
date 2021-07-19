#include "Chip8.h"
#include <array>
#include <bitset>
#include <cstring>
#include <iostream>
#include <netinet/in.h>

Chip8::Chip8(uint8_t *rom, uint16_t romSize)
    : pc(0x200), I(0), delay_timer(0), waiting_for_key(-1),
      distribution(0, 255) {
  static const uint8_t fontset[80] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  memset(memory, 0, 0x1000);
  memset(screen, 0, 64 * 32);

  memcpy(memory, fontset, sizeof(fontset));
  if (romSize >= 0x1000 - 0x200) {
    return;
  }
  memcpy(memory + 0x200, rom, romSize);
}

const std::array<std::function<void(Chip8 &, Instruction)>, 16>
    Chip8::opcode_map{
        &Chip8::op_system,   &Chip8::op_goto,      &Chip8::op_call,
        &Chip8::op_skip_ceq, &Chip8::op_skip_cneq, &Chip8::op_skip_eq,
        &Chip8::op_set,      &Chip8::op_inc,       &Chip8::op_arithmetic,
        &Chip8::op_skip_neq, &Chip8::op_set_i,     &Chip8::op_goto_plus_v0,
        &Chip8::op_random,   &Chip8::op_draw,      &Chip8::op_key,
        &Chip8::op_reg,
    };

Instruction Chip8::cycle() {
  if (delay_timer)
    --delay_timer;
  if (waiting_for_key == -1) {
    Instruction instr(memory[pc] << 8 | memory[pc + 1]);
    pc += 2;
    // (opcode_map[instr.hnibble()])(*this, instr);
    switch (instr.hnibble()) {
    case 0x0:
      op_system(instr);
      break;
    case 0x1:
      op_goto(instr);
      break;
    case 0x2:
      op_call(instr);
      break;
    case 0x3:
      op_skip_ceq(instr);
      break;
    case 0x4:
      op_skip_cneq(instr);
      break;
    case 0x5:
      op_skip_eq(instr);
      break;
    case 0x6:
      op_set(instr);
      break;
    case 0x7:
      op_inc(instr);
      break;
    case 0x8:
      op_arithmetic(instr);
      break;
    case 0x9:
      op_skip_neq(instr);
      break;
    case 0xA:
      op_set_i(instr);
      break;
    case 0xB:
      op_goto_plus_v0(instr);
      break;
    case 0xC:
      op_random(instr);
      break;
    case 0xD:
      op_draw(instr);
      break;
    case 0xE:
      op_key(instr);
      break;
    case 0xF:
      op_reg(instr);
      break;
    }
    return instr;
  }
  return Instruction(0);
}

void Chip8::op_system(Instruction inst) {
  if (inst.inst() == 0x00EE) {
    if (!stack.empty()) {
      pc = stack.top();
      stack.pop();
    } else {
      pc = 0x200;
    }
  } else if (inst.inst() == 0x00E0) {
    memset(screen, 0, sizeof(screen));
  }
}

void Chip8::op_goto(Instruction inst) { pc = inst.address(); }

void Chip8::op_call(Instruction inst) {
  stack.push(pc);
  pc = inst.address();
}

void Chip8::op_skip_ceq(Instruction inst) {
  if (v[inst.x()] == inst.byte())
    pc += 2;
}

void Chip8::op_skip_cneq(Instruction inst) {
  if (v[inst.x()] != inst.byte())
    pc += 2;
}

void Chip8::op_skip_eq(Instruction inst) {
  if (v[inst.x()] == v[inst.y()])
    pc += 2;
}

void Chip8::op_set(Instruction inst) { v[inst.x()] = inst.byte(); }

void Chip8::op_inc(Instruction inst) { v[inst.x()] += inst.byte(); }

void Chip8::op_arithmetic(Instruction inst) {
  uint8_t &vx = v[inst.x()];
  uint8_t vy = v[inst.y()];
  uint8_t store = vx;

  switch (inst.nibble()) {
  case 0: // assign
    vx = vy;
    break;
  case 1: // or
    vx |= vy;
    break;
  case 2: // and
    vx &= vy;
    break;
  case 3: // xor
    vx ^= vy;
    break;
  case 4: // add
    vx += vy;
    v[0xF] = store > vx;
    break;
  case 5: // sub
    vx -= vy;
    v[0xF] = store > vx;
    break;
  case 6: // shift right
    v[0xF] = vx & 1;
    vx >>= 1;
    break;
  case 0xE: // shift left
    v[0xF] = vx >> 7;
    vx <<= 1;
    break;
  case 7: // rev sub
    vx = vy - vx;
    v[0xF] = vy >= vx;
    break;
  }
}

void Chip8::op_skip_neq(Instruction inst) {
  if (v[inst.x()] != v[inst.y()]) {
    pc += 2;
  }
}

void Chip8::op_set_i(Instruction inst) { I = inst.address(); }

void Chip8::op_goto_plus_v0(Instruction inst) { pc = inst.address() + v[0]; }

void Chip8::op_random(Instruction inst) {
  v[inst.x()] = distribution(random_engine) & inst.byte();
}

void Chip8::op_draw(Instruction inst) {
  is_screen_updated = true;
  uint8_t x = v[inst.x()];
  uint8_t y = v[inst.y()];
  uint8_t height = inst.nibble();
  v[0xF] = 0;
  for (int i = 0; i < height; i++) {
    uint8_t line = memory[I + i];
    for (int j = 0; j < 8; ++j, line >>= 1) {
      uint8_t pos_x = (x + 7 - j) % 64; // % 64
      uint8_t pos_y = (y + i) % 32;     // % 32
      bool prev = screen[pos_x][pos_y];
      screen[pos_x][pos_y] ^= line & 1;
      if (prev && (line & 1)) {
        v[0xF] = 1;
      }
    }
  }
}

void Chip8::op_key(Instruction inst) {
  switch (inst.byte()) {
  case 0x9E:
    if (keys[v[inst.x()]])
      pc += 2;
    break;
  case 0xA1:
    if (!keys[v[inst.x()]])
      pc += 2;
    break;
  }
}

void Chip8::op_reg(Instruction inst) {
  uint8_t &vx = v[inst.x()];
  uint8_t x = inst.x();

  uint8_t hundreds, tenths, ones;
  switch (inst.byte()) {
  case 0x07: // get delay_timer
    vx = delay_timer;
    break;
  case 0x0A: // wait for key press
    waiting_for_key = x;
    break;
  case 0x15: // set delay_timer
    delay_timer = vx;
    break;
  case 0x18: // sound timer
    break;   // TODO
  case 0x1E:
    I += vx;
    break;
  case 0x29:
    I = vx * 5;
    break;
  case 0x33:
    hundreds = vx / 100;
    tenths = vx % 100 / 10;
    ones = vx % 10;
    memory[I] = hundreds;
    memory[I + 1] = tenths;
    memory[I + 2] = ones;
    break;
  case 0x55:
    for (int i = 0; i <= x; ++i) {
      memory[I + i] = v[i];
    }
    break;
  case 0x65:
    for (int i = 0; i <= x; ++i) {
      v[i] = memory[I + i];
    }
    break;
  }
}

void Chip8::press_key(uint8_t key) {
  if (waiting_for_key != -1) {
    v[waiting_for_key] = key;
    waiting_for_key = -1;
  }
  keys[key] = true;
}

void Chip8::release_key(uint8_t key) { keys[key] = false; }

bool Chip8::get_pixel(uint8_t x, uint8_t y) { return screen[x][y]; }

uint8_t &Chip8::V(uint8_t idx) { return v[idx]; }

uint8_t &Chip8::mem(uint16_t address) {
  if (address >= 0x1000)
    return memory[0xFFF];
  return memory[address];
}

uint16_t &Chip8::refI(Chip8::Internal ref) {
  switch (ref) {
  case Chip8I:
    return I;
  case Chip8PC:
    return pc;
  }
  return I;
}

bool Chip8::screen_updated() { return is_screen_updated; }

void Chip8::screen_update() { is_screen_updated = false; }

decltype(Chip8::screen) &Chip8::get_display() { return screen; }