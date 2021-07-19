#include "SdlInterface.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include <SDL2/SDL_timer.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

static const float vertices[] = {
    // position    textcoord
    -1, -1, 0.0, 0.0, 0.0,

    -1, 1,  0.0, 0.0, 1.0,

    1,  1,  0.0, 1.0, 1.0,

    1,  -1, 0.0, 1.0, 0.0,
};

SdlInterface::SdlInterface(Chip8 &emu, int argc, char *args[])
    : Interface(emu, argc, args), scale(10.0f), debug(false), emu_mtx(),
      sdl_mtx(), closing(false) {
  std::stringstream ss;

  error = false;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    error = true;
    ss << "SDL_Init failed: " << SDL_GetError();
    err = ss.str();
    SDL_Quit();
    return;
  }

  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  window = SDL_CreateWindow("Chip8 emulator", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 64 * scale, 32 * scale,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

  if (!(window)) {
    error = true;
    ss << "SDL_CreateWindow failed: " << SDL_GetError();
    err = ss.str();
    SDL_Quit();
    return;
  }

  ctx = SDL_GL_CreateContext(window);
  if (ctx == NULL) {
    error = true;
    ss << "SDL_GL_CreateContext failed: " << SDL_GetError();
    err = ss.str();
    return;
  }
  SDL_GL_MakeCurrent(window, ctx);
  SDL_GL_SetSwapInterval(1);

  if (glewInit() != GLEW_OK) {
    error = true;
    err = "glewInit failed!";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
  }

  std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl
            << "Version: " << glGetString(GL_VERSION) << std::endl
            << "Shader Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
            << std::endl
            << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

  IMGUI_CHECKVERSION();
  ImGui::SetCurrentContext(ImGui::CreateContext());
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, ctx);
  ImGui_ImplOpenGL3_Init(glsl_version);

  glGenBuffers(1, &buffer);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  program_id = load_shaders();

  if (error_occurred())
    return;

  glUseProgram(program_id);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
}

void SdlInterface::gen_screentex() {
  auto screen = emulator.get_display();
  for (int i = 0; i < 32; ++i) {
    for (int j = 0; j < 64; ++j) {
      if (screen[j][32 - i]) {
        screenTex[i][j][0] = 255;
        screenTex[i][j][1] = 255;
        screenTex[i][j][2] = 255;
      } else {
        screenTex[i][j][0] = 0;
        screenTex[i][j][1] = 0;
        screenTex[i][j][2] = 0;
      }
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 32, 0, GL_RGB, GL_UNSIGNED_BYTE,
               screenTex);
}

bool SdlInterface::error_occurred() const { return error; }

std::string SdlInterface::error_message() const { return err; }

bool SdlInterface::update() {
  auto start_time = std::chrono::steady_clock::now();
  emu_mtx.lock();
  emulator.cycle();
  emu_mtx.unlock();
  SDL_Event event;
  int8_t emukey;

  while (SDL_PollEvent(&event)) {
    if (debug)
      ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type) {
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        if (event.window.windowID == SDL_GetWindowID(window)) {
          closing = true;
          sdl_mtx.lock();
          SDL_DestroyWindow(window);
          SDL_Quit();
          return false;
        }
      }
      break;
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_KP_PLUS) {
        scale += 0.5;
        SDL_SetWindowSize(window, scale * 64, scale * 32);
        break;
      } else if (event.key.keysym.sym == SDLK_KP_MINUS) {
        scale -= 0.5;
        SDL_SetWindowSize(window, scale * 64, scale * 32);
        break;
      }
      if (event.key.repeat)
        break;
      emukey = translate_key(event.key.keysym.sym);
      if (emukey != -1)
        emulator.press_key(emukey);
      break;
    case SDL_KEYUP:
      if (event.key.repeat)
        break;
      if (event.key.keysym.sym == SDLK_F1) {
        debug = !debug;
        break;
      }
      emukey = translate_key(event.key.keysym.sym);
      if (emukey != -1)
        emulator.release_key(emukey);
      break;
    }
  }
  tick_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - start_time)
                  .count();
  return true;
}

int8_t SdlInterface::translate_key(const SDL_Keycode key) {
  if (key >= SDLK_KP_1 && key <= SDLK_KP_9) {
    return key - SDLK_KP_1 + 1;
  }

  switch (key) {
  case SDLK_KP_0:
    return 0;
  case SDLK_w:
  case SDLK_UP:
    return 2;
  case SDLK_s:
  case SDLK_DOWN:
    return 8;
  case SDLK_a:
  case SDLK_LEFT:
    return 4;
  case SDLK_d:
  case SDLK_RIGHT:
    return 6;
  }

  return -1;
}

void SdlInterface::update_screen() {
  if (closing)
    return;
  sdl_mtx.lock();
  Uint32 start_time = SDL_GetTicks();
  glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  emu_mtx.lock();
  if (emulator.screen_updated()) {
    gen_screentex();
    emulator.screen_update();
  }
  emu_mtx.unlock();

  glDrawArrays(GL_QUADS, 0, 4);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  if (debug) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
    guiFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
  SDL_GL_SwapWindow(window);
  render_time = SDL_GetTicks() - start_time;
  sdl_mtx.unlock();
}

void SdlInterface::guiFrame() {
  ImGui::Begin("Registers");
  ImGui::Text("I: %03X", emulator.refI(Chip8::Internal::Chip8I));
  ImGui::Text("PC: %03X", emulator.refI(Chip8::Internal::Chip8PC));
  for (int i = 0; i < 16; ++i) {
    ImGui::Text("V%01X: %02X", i, emulator.V(i));
  }
  ImGui::End();
  ImGui::Begin("Performance");
  ImGui::Text("Last render time: %d ms", render_time);
  ImGui::Text("Last tick time: %d ms", tick_time);
  ImGui::Text("Avg ImGui time: %.3f ms/%.1f FPS",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();
}

GLuint SdlInterface::load_shaders() {
  std::string vshader, fshader;
  const char *vshader_ptr, *fshader_ptr;

  GLuint vshader_id = glCreateShader(GL_VERTEX_SHADER),
         fshader_id = glCreateShader(GL_FRAGMENT_SHADER);

  std::ifstream vshader_file("shaders/vertex.vs");
  if (!vshader_file.is_open()) {
    error = true;
    err = "Cannot open vertex shader!";
    return 0;
  }

  std::stringstream vss;
  vss << vshader_file.rdbuf();
  vshader = vss.str();
  vshader_ptr = vshader.c_str();
  vshader_file.close();

  std::ifstream fshader_file("shaders/fragment.fs");
  if (!fshader_file.is_open()) {
    error = true;
    err = "Cannot open fragment shader!";
    return 0;
  }

  std::stringstream fss;
  fss << fshader_file.rdbuf();
  fshader = fss.str();
  fshader_ptr = fshader.c_str();

  GLint result = GL_FALSE;
  int log_length;

  glShaderSource(vshader_id, 1, &vshader_ptr, nullptr);
  glCompileShader(vshader_id);

  glGetShaderiv(vshader_id, GL_COMPILE_STATUS, &result);
  glGetShaderiv(vshader_id, GL_INFO_LOG_LENGTH, &log_length);

  if (log_length > 0) {
    char *log = new char[log_length + 1];
    glGetShaderInfoLog(vshader_id, log_length, nullptr, log);
    std::cout << log << std::endl;
  }

  glShaderSource(fshader_id, 1, &fshader_ptr, nullptr);
  glCompileShader(fshader_id);

  glGetShaderiv(fshader_id, GL_COMPILE_STATUS, &result);
  glGetShaderiv(fshader_id, GL_INFO_LOG_LENGTH, &log_length);

  if (log_length > 0) {
    char *log = new char[log_length + 1];
    glGetShaderInfoLog(fshader_id, log_length, nullptr, log);
    std::cout << log << std::endl;
  }

  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vshader_id);
  glAttachShader(program_id, fshader_id);
  glLinkProgram(program_id);

  glGetProgramiv(program_id, GL_COMPILE_STATUS, &result);
  glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

  if (log_length > 0) {
    char *log = new char[log_length + 1];
    glGetProgramInfoLog(program_id, log_length, nullptr, log);
    std::cout << log << std::endl;
  }

  glDetachShader(program_id, vshader_id);
  glDetachShader(program_id, fshader_id);

  glDeleteShader(vshader_id);
  glDeleteShader(fshader_id);

  return program_id;
}