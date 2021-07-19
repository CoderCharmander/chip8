SOURCES := $(wildcard src/*.cpp) $(wildcard src/imgui/*.cpp)
OBJECTS := $(subst .cpp,.o,$(subst src/,build/,$(SOURCES)))
HEADERS := $(wildcard src/*.h)

SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL2_LIBS = $(shell pkg-config --libs sdl2)
OPT_FLAGS = -pg -g

CXX ?= g++
CXXFLAGS ?= -Wall $(OPT_FLAGS) $(SDL2_CFLAGS)
LD := g++
LDFLAGS ?= -lncursesw -lGL -lGLEW -lglad $(OPT_FLAGS) $(SDL2_LIBS)

OUTPUT = build/main

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT)

run: all
	$(OUTPUT)

build/%.o: src/%.cpp
	$(CXX) $? $(CXXFLAGS) -c -o $(subst src/,build/,$(subst .cpp,.o,$?))

clean:
	rm -f $(OBJECTS) $(OUTPUT)