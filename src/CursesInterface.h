#include <vector>
#include "Interface.h"

#define INTERFACE CursesInterface
class CursesInterface : Interface {
	public:
		CursesInterface(Chip8&, int, char* args[]);
		bool update();
		void update_screen();
		~CursesInterface();
	private:
		bool debug;
};
