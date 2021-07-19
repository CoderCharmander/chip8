#include "Interface.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <mutex>

#define INTERFACE SdlInterface

class SdlInterface : public Interface {
public:
  SdlInterface(Chip8 &, int, char *args[]);
  bool update();
  void update_screen();
  bool error_occurred() const;
  std::string error_message() const;

private:
  SDL_Window *window;
  SDL_GLContext ctx;
  GLuint program_id;

  std::string err;
  bool error;
  GLuint buffer;
  GLuint texture;
  GLubyte screenTex[32][64][3];
  bool debug;
  float scale;
  Uint32 render_time;
  Uint32 tick_time;
  std::mutex emu_mtx, sdl_mtx;
  bool closing;

  static int8_t translate_key(const SDL_Keycode);
  void guiFrame();

  GLuint load_shaders();
  void gen_screentex();
};