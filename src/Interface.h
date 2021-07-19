#include "Chip8.h"
#include <string>
class Interface {
	public:
		Interface(Chip8&, int, char* args[]);
		virtual bool update() = 0;
		virtual void update_screen() = 0;
		virtual bool error_occurred() const;
		virtual std::string error_message() const;
	protected:
		Chip8 &emulator;
};
