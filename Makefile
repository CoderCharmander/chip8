SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(subst .cpp,.o,$(subst src/,build/,$(SOURCES)))
HEADERS := $(wildcard src/*.h)

CXX ?= g++
CXXFLAGS = -Wall -g
LD := g++
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -g

OUTPUT = build/main

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT)

run: all
	$(OUTPUT)

build/%.o: src/%.cpp
	$(CXX) $? $(CXXFLAGS) -c -o $(subst src/,build/,$(subst .cpp,.o,$?))