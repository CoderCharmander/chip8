#include <vector>
#include "Interface.h"

#define INTERFACE CursesInterface
class CursesInterface : Interface {
	public:
		CursesInterface(Chip8&);
		bool update();
		void update_screen();
		~CursesInterface();
};
