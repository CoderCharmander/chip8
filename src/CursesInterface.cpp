#include <ncurses.h>
#include <csignal>
#include <cstdint>
#include <clocale>
#include <iostream>
#include "CursesInterface.h"

CursesInterface::CursesInterface(Chip8& emu): Interface(emu) {
	setlocale(LC_ALL, "");
	initscr();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_BLACK, COLOR_WHITE);
	init_pair(4, COLOR_WHITE, COLOR_WHITE);
}

CursesInterface::~CursesInterface() {
	endwin();
}

bool CursesInterface::update() {
	emulator.cycle();
	return true;
}

void CursesInterface::update_screen() {
	for (uint8_t x = 0; x < 64; ++x) {
		for (uint8_t y = 0; y < 32; y += 2) {
			short pair = int(emulator.get_pixel(x, y)) << 1 | int(emulator.get_pixel(x, y + 1));
			//printw("%d", pair);
			move(y/2, x);
			attron(COLOR_PAIR(pair + 1));
			//addch(L'\u2584');
			printw("%lc", L'\u2584');
			attroff(COLOR_PAIR(pair));
		}
	}
	refresh();
}
