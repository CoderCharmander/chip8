#include <ncurses.h>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <clocale>
#include <iostream>
#include "CursesInterface.h"

static const wchar_t blocks[] {L' ', L'\u2584', L'\u2580', L'\u2588'};

CursesInterface::CursesInterface(Chip8& emu, int argc, char* args[]): Interface(emu, argc, args) {
	setlocale(LC_ALL, "");
	std::cout << argc << ' ' << args << std::endl;
	for (int i = 0; i < argc; ++i) {
		std::cout << args[i] << std::endl;
	}
	initscr();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_BLACK, COLOR_WHITE);
	init_pair(4, COLOR_WHITE, COLOR_WHITE);
	if (argc > 0 && !strcmp(args[0], "-d")) {
		debug = true;
	}
	else debug=false;
}

CursesInterface::~CursesInterface() {
	endwin();
}

bool CursesInterface::update() {
	emulator.cycle();
	return true;
}

void CursesInterface::update_screen() {
	move(0, 0);
	for (uint8_t y = 0; y < 32; y += 2) {
		for (uint8_t x = 0; x < 64; ++x) {
			short pair = int(emulator.get_pixel(x, y)) << 1 | int(emulator.get_pixel(x, y + 1));
			printw("%lc", blocks[pair]);
		}
		printw("\n");
	}
	if (debug) {
		uint16_t& pc = emulator.refI(Chip8::Chip8PC);
		printw("Pointer: %03X\n", pc);
		printw("Executing: %04X\n",
				emulator.mem(pc-2)<<8 | emulator.mem(pc-1));
		printw("Registers: ");
		for (int i = 0; i < 16; ++i) {
			printw("%02X ", emulator.V(i));
		}
		printw("\nI: %03X\n", emulator.refI(Chip8::Chip8I));
	}
	refresh();
	if (debug) getch();
}
