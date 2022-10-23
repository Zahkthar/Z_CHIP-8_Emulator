#ifndef CHIP_8_H_INCLUDED
#define CHIP_8_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "SDL2/SDL.h"

#define MEMORY_SIZE 4096
#define GRAPHICS_SIZE 2048

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 512

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

typedef struct chip8 {
    uint16_t stack[16];
    uint16_t I;
    uint16_t opcode;
    uint16_t pc;
    uint16_t sp;
    uint8_t memory[MEMORY_SIZE];
    uint8_t graphics[GRAPHICS_SIZE];
    uint8_t V[16];
    uint8_t keys[16];
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint8_t drawflag;
} chip8;

int chip8_RunTheGame(SDL_Renderer *renderer, FILE *file, int delayBetweenFrames);

#endif