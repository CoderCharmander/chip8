#include "Chip8.h"
class Interface {
	public:
		Interface(Chip8&, int, char* args[]);
		virtual bool update() = 0;
		virtual void update_screen() = 0;
	protected:
		Chip8 &emulator;
};
