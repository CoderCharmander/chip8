#include <iostream>
#include <fstream>
#include <cstdlib>

#include <SFML/Graphics.hpp>

#include "Chip8.h"

float scale;

void renderFrame(sf::RenderWindow &window, Chip8 &vm)
{
    sf::RectangleShape shape(sf::Vector2f(scale, scale));
    shape.setFillColor(sf::Color::White);

    window.clear();

    for (uint8_t i = 0; i < 64; i++)
    {
        for (uint8_t j = 0; j < 32; j++)
        {
            if (vm.get_pixel(i, j)) {
                shape.setPosition(sf::Vector2f(i * scale, j * scale));
                window.draw(shape);
            }
        }
    }
    window.display();
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <rom> [display size]" << std::endl;
        return 1;
    }

    if (argc < 3)
    {
        scale = 10.0f;
    }
    else
    {
        scale = strtof(argv[2], nullptr);

        if (scale == 0.0f)
        {
            std::cerr << argv[0] << ": Please enter a valid scale value." << std::endl;
            return 2;
        }
    }

    std::ifstream rom_file(argv[1], std::ios::binary | std::ios::ate);
    std::streampos begin, end;
    if (!rom_file.is_open()) {
        std::cerr << "Could not open ROM file " << argv[1] << std::endl;
        return 3;
    }
    end = rom_file.tellg();
    rom_file.seekg(0, std::ios::beg);
    begin = rom_file.tellg();

    size_t size = end - begin;
    if (size > 0x1000 - 0x200) {
        std::cerr << "The file is too large!" << std::endl;
        return 4;
    }

    std::cout << "Loading " << size << " bytes from " << argv[1] << std::endl;
    uint8_t* rom = new uint8_t[size];
    rom_file.read(reinterpret_cast<char*>(rom), size);
    rom_file.close();

    Chip8 emulator(rom, size);

    delete[] rom;

    sf::RenderWindow window(sf::VideoMode(64 * scale, 32 * scale), "Chip8 emulator");
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }
        emulator.cycle();
        renderFrame(window, emulator);
    }

    Instruction instr(0x1234);
    std::cout << instr.w0 << std::endl;

    return 0;
}