#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "Chip8.h"
#include "SdlInterface.h"

float scale;

void usage(std::string progname) {
  std::cerr << "Usage: " << progname
            << " [-c cyclespersec] ROMFILE [displaysize]" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <rom> [display size]" << std::endl;
    return 1;
  }
  char *rom_filename = nullptr;
  uint32_t cycle_time = 0;
  for (int i = 1; i < argc; ++i) {
    std::string curr_arg = argv[i];
    if (curr_arg == "-c" || curr_arg == "--cycle") {
      if (i < argc - 1) {
        cycle_time = ((double)1000.0) / std::atof(argv[++i]);
        std::cerr << "Set cycle time to " << cycle_time << std::endl;
      } else {
        usage(argv[0]);
        return 1;
      }
    } else {
      rom_filename = argv[i];
    }
  }

  if (!rom_filename) {
    usage(argv[0]);
    return 1;
  }

  std::ifstream rom_file(rom_filename, std::ios::binary | std::ios::ate);
  std::streampos begin, end;
  if (!rom_file.is_open()) {
    std::cerr << "Could not open ROM file " << argv[1] << std::endl;
    return 3;
  }
  end = rom_file.tellg();
  rom_file.seekg(0, std::ios::beg);
  begin = rom_file.tellg();

  size_t size = end - begin;
  if (size > 0x1000 - 0x200) {
    std::cerr << "The file is too large!" << std::endl;
    return 4;
  }

  std::cout << "Loading " << size << " bytes from " << rom_filename
            << std::endl;
  uint8_t *rom = new uint8_t[size];
  rom_file.read(reinterpret_cast<char *>(rom), size);
  rom_file.close();

  Chip8 emulator(rom, size);

  delete[] rom;
  INTERFACE iface(emulator, argc - 2, argv + 2);
  if (iface.error_occurred()) {
    std::cerr << "An error occurred while trying to initialize the interface: "
              << iface.error_message() << std::endl;
    return 1;
  }

  bool running = true;
  std::chrono::milliseconds cycle_sleep_duration(cycle_time);
  std::thread th_cycle([&]() {
    while (iface.update()) {
      std::this_thread::sleep_for(cycle_sleep_duration);
    }
    running = false;
  });

  while (running) {
    iface.update_screen();
  }
  th_cycle.join();

  return 0;
}
