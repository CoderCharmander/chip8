#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include <cstdint>

#include <bit>

struct Instruction {
  // const uint8_t h2 : 4;
  // const uint8_t h3 : 4;
  // const uint8_t h1 : 4;
  // const uint8_t h0 : 4;

  // const uint8_t b0;
  // const uint8_t b1;
  // const uint8_t b2;

  // const uint16_t a0 : 12;

  // const uint16_t w0;

  Instruction(int16_t data)
      : // : h0(data & halfMask), h1(data >> 4 & halfMask), h2(data >> 8 &
        // halfMask),
        //   h3(data >> 12 & halfMask),

        //   b0(data & byteMask), b1(data >> 4 & byteMask), b2(data >> 8 &
        //   byteMask),

        //   a0(data & addressMask),

        //   w0(data),
        data(data) {}

public:
  uint16_t inst() const { return data; }
  uint16_t address() const { return data & addressMask; }
  uint16_t byte() const { return data & byteMask; }
  uint8_t x() const { return data >> 8 & halfMask; }
  uint8_t y() const { return data >> 4 & halfMask; }
  uint8_t nibble() const { return data & halfMask; }
  uint8_t hnibble() const { return data >> 12 & halfMask; }

private:
  static constexpr int16_t halfMask = 0x000f;
  static constexpr int16_t byteMask = 0x00ff;
  static constexpr int16_t addressMask = 0x0fff;
  const uint16_t data;
};

#endif