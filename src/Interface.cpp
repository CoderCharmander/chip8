#include "Interface.h"

Interface::Interface(Chip8& emu, int argc, char* args[]): emulator(emu) {
}

bool Interface::error_occurred() const {
    return false;
}

std::string Interface::error_message() const {
    return "";
}