#include "chip8.h"

#define INCREMENT_PC(pc) pc += 2;

static void chip8_Init(struct chip8 *c8);
static int chip8_LoadROM(struct chip8 *c8, FILE *file);
static void chip8_EmulateOneInstruction(struct chip8 *c8);
static int chip8_ExecuteTheInstruction(struct chip8 *c8);
static int chip8_HandleEvents(struct chip8 *c8);
static void chip8_Render(SDL_Renderer *renderer, SDL_Texture *texture, struct chip8 *c8);

int chip8_RunTheGame(SDL_Renderer *renderer, FILE *file, int delayBetweenFrames) {
    srand(time(NULL));

    chip8 *c8 = malloc(sizeof(chip8));
    if(c8 == NULL) { fprintf(stderr, "malloc c8"); return 1; }
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if(texture == NULL) { fprintf(stderr, "Error SDL_CreateTexture : %s", SDL_GetError()); return 1; }

    chip8_Init(c8);
    if(chip8_LoadROM(c8, file) != 0) { return 1; }

    while(1) {
        if(chip8_HandleEvents(c8) != 0) { break; }
        chip8_EmulateOneInstruction(c8);
        if(c8->drawflag) { chip8_Render(renderer, texture, c8); }

        // Delay here
        SDL_Delay(delayBetweenFrames);
    }

    free(c8);
    SDL_DestroyTexture(texture);
    return 0;
}

static void chip8_Init(struct chip8 *c8) {
    uint8_t chip8Fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    c8->pc = 0x200; // Adresse du début de la ROM
    c8->opcode = 0;
    c8->I = 0;
    c8->sp = 0;
    c8->delayTimer = 0;
    c8->soundTimer = 0;

    memset(c8->V, 0, sizeof(c8->V));
    memset(c8->keys, 0, sizeof(c8->keys));
    memset(c8->stack, 0, sizeof(c8->stack));
    memset(c8->graphics, 0, sizeof(c8->graphics));
    memset(c8->memory, 0, sizeof(c8->memory));

    for(int i = 0; i < 80; ++i) {
        c8->memory[i] = chip8Fontset[i];
    }
}

static int chip8_LoadROM(struct chip8 *c8, FILE *rom) {
    char* buffer = NULL;
    size_t romSize = 0;

    fseek(rom, 0, SEEK_END);
    romSize = ftell(rom);

    // If the ROM is too large
    if(romSize >= 4092 - 512) { fprintf(stderr, "ROM cannot fit into memory"); return 1; }
    rewind(rom);

    buffer = malloc(romSize + 1);
    if(buffer == NULL) { fprintf(stderr, "buffer allocation error LoadROM"); return 1; }

    // ROMFile -> buffer
    if(fread(buffer, sizeof(char), romSize, rom) != romSize) { fprintf(stderr, "cannot read the rom into buffer"); return 1; }
    buffer[romSize] = '\0';

    // Write the ROM in the Chip-8 memory
    for (size_t i = 0; i < romSize; ++i) {
        c8->memory[0x200 + i] = buffer[i];
    }

    free(buffer);
    return 0;
}

static void chip8_EmulateOneInstruction(struct chip8 *c8) {
    c8->opcode = c8->memory[c8->pc] << 8 | c8->memory[c8->pc + 1];
    if(chip8_ExecuteTheInstruction(c8) == 0) {
        INCREMENT_PC(c8->pc);
        if(c8->delayTimer > 0) { c8->delayTimer--; }
        if(c8->soundTimer > 0) { c8->soundTimer--; }
    }
    
    printf("Opcode: %x    Memory: %x    I: %x    SP: %x    PC: %d\n", 
           c8->opcode, c8->memory[c8->pc] << 8 | c8->memory[c8->pc + 1], c8->I, c8->sp, c8->pc);
}

static int chip8_ExecuteTheInstruction(struct chip8 *c8) {
    int pos, keypress = 0;
    uint16_t vx, vy, nn, nnn;
    uint16_t h, pixel;
    uint8_t cvx, cvy;
    
    vx = (c8->opcode & 0x0F00) >> 8;
    vy = (c8->opcode & 0x00F0) >> 4;
    nn = c8->opcode & 0x00FF;
    nnn = c8->opcode & 0x0FFF;
    
    switch (c8->opcode & 0xF000) {
        case 0x0000: // 00E_
            switch (nn) {
                case 0xE0: // 00E0 - Clear screen
                    memset(c8->graphics, 0, sizeof(c8->graphics));
                    c8->drawflag = 1;
                    break;
                case 0xEE: // 00EE - Return from subroutine
                    c8->pc = c8->stack[--c8->sp];
                    break;
                default:
                    fprintf(stderr, "Unknown opcode: %x\n", c8->opcode);
                    return 1;
                }
            break;
        case 0x1000: // 1NNN - Jump to address NNN
            c8->pc = nnn - 2;
            break;
        case 0x2000: // 2NNN - Call subroutine at NNN
            c8->stack[c8->sp++] = c8->pc;
            c8->pc = nnn - 2;
            break;
        case 0x3000: // 3NNN - Skip next instruction if VX == NN
            if (c8->V[vx] == nn) { INCREMENT_PC(c8->pc); }
            break;
        case 0x4000: // 4NNN - Skip next instruction if VX != NN
            if (c8->V[vx] != nn) { INCREMENT_PC(c8->pc); }
            break;
        case 0x5000: // 5XY0 - Skip next instruction if VX == VY
            if (c8->V[vx] == c8->V[vy]) { INCREMENT_PC(c8->pc); }
            break;
        case 0x6000: // 6XNN - Set VX to NN
            c8->V[vx] = nn;
            break;
        case 0x7000: // 7XNN - Add NN to VX
            c8->V[vx] += nn;
            break;
        case 0x8000: // 8XY_
            switch (c8->opcode & 0x000f) {
                case 0x0000: // 8XY0 - Set VX to VY
                    c8->V[vx] = c8->V[vy];
                    break;
                case 0x0001: // 8XY1 - Set VX to (VX OR VY)
                    c8->V[vx] |= c8->V[vy];
                    break;
                case 0x0002: // 8XY2 - Set VX to (VX AND VY)
                    c8->V[vx] &= c8->V[vy];
                    break;
                case 0x0003: // 8XY3 - Set VX to (VX XOR VY)
                    c8->V[vx] ^= c8->V[vy];
                    break;
                case 0x0004: // 8XY4 - Add VY to VX, VF = 1 if there is a carry
                    c8->V[vx] += c8->V[vy];
                    c8->V[0xF] = (c8->V[vy] > (0xFF - c8->V[vx])) ? 1 : 0;
                    break;
                case 0x0005: // 8XY5 - Sub VY from VX, VF = 0 if there is a borrow
                    c8->V[0xF] = (c8->V[vy] > c8->V[vx]) ? 0 : 1;
                    c8->V[vx] -= c8->V[vy];
                    break;
                case 0x0006: // 8XY6 - Shift VX right by 1. VF = LSB of VX before shift
                    c8->V[0xF] = c8->V[vx] & 0x1;
                    c8->V[vx] >>= 1;
                    break;
                case 0x0007: // 8XY7 - Set VX to VY-VX. VF = 0 if there is a borrow
                    c8->V[0xF] = (c8->V[vx] > c8->V[vy]) ? 0 : 1;
                    c8->V[vx] = c8->V[vy] - c8->V[vx];
                    break;
                case 0x000e: // 8XYE - Shift VX left by 1. VF = MSB of VX before shift
                    c8->V[0xF] = c8->V[vx] >> 7;
                    c8->V[vx] <<= 1;
                    break;
                default:
                    fprintf(stderr, "Unknown opcode: %x\n", c8->opcode);
                    return 1;
            }
            break;
        case 0x9000: // 9XY0 - Skip next instruction if VX != VY
            if (c8->V[vx] != c8->V[vy]) { INCREMENT_PC(c8->pc); }
            break;
        case 0xA000: // ANNN - Set I to the address NNN
            c8->I = nnn;
            break;
        case 0xB000: // BNNN - Jump to NNN + V0
            c8->pc = (nnn + c8->V[0]) - 2;
            break;
        case 0xC000: // CNNN - Set VX to random number masked by NN
            c8->V[vx] = (rand() % (0xff + 1)) & nn;
            break;
        case 0xD000: // Draw an 8 pixel sprite at (VX, VY)
            cvx = c8->V[vx];
            cvy = c8->V[vy];
            c8->V[0xF] = 0;
            h = c8->opcode & 0x000F;
            for (int yl = 0; yl < h; yl++) {
                for (int xl = 0; xl < 8; xl++) {
                    pixel = c8->memory[c8->I + yl];
                    pos = cvx + xl + ((cvy + yl) * DISPLAY_WIDTH);
                    if ((pixel & (0x80 >> xl)) == 0) { continue; }
                    if (c8->graphics[pos] == 1) { c8->V[0xF] = 1; }
                    c8->graphics[pos] ^= 1;
                }
            }
            c8->drawflag = 1;
            break;
        case 0xE000: // EX__
            switch (nn) {
                case 0x009E: // EX9E - Skip next instruction if key in VX is pressed
                    if (c8->keys[c8->V[vx]] == 1) { INCREMENT_PC(c8->pc); }
                    break;
                case 0x00A1: // EXA1 - Skip next instruction if key in VX isn't pressed
                    if (c8->keys[c8->V[vx]] == 0) { INCREMENT_PC(c8->pc); }
                    break;
                default:
                    fprintf(stderr, "Unknown opcode: %x\n", c8->opcode);
                    return 1;
            }
            break;
        case 0xF000: // FX__
            switch (nn) {
                case 0x0007: // FX07 - Set VX to delaytimer
                    c8->V[vx] = c8->delayTimer;
                    break;
                case 0x000A: // FX0A - Wait for key press and then store it in VX
                    for (int i = 0; i < 16; i++) {
                        if (c8->keys[i] == 1) {
                            c8->V[vx] = i;
                            keypress = 1;
                        }
                    }
                    if (keypress == 0) { return 1; }
                    break;
                case 0x0015: // FX15 - Set the delaytimer to VX
                    c8->delayTimer = c8->V[vx];
                    break;
                case 0x0018: // FX18 - Set the soundtimer to VX
                    c8->soundTimer = c8->V[vx];
                    break;
                case 0x001E: // FX1E - Add VX to I
                    c8->V[0xF] = ((c8->I + c8->V[vx]) > 0xFFF) ? 1 : 0;
                    c8->I += c8->V[vx];
                    break;
                case 0x0029: // FX29 - Set I to the location of the sprite for char VX
                    c8->I = c8->V[vx] * 0x5;
                    break;
                case 0x0033: // FX33 - Store bin coded decimal of VX at I, I+1 and I+2
                    c8->memory[c8->I] = c8->V[vx] / 100;
                    c8->memory[c8->I + 1] = (c8->V[vx] / 10) % 10;
                    c8->memory[c8->I + 2] = c8->V[vx] % 10;
                    break;
                case 0x0055: // FX55 - Store V0 to VX in memory starting at I
                    for (int i = 0; i <= (vx); i++) { 
                        c8->memory[c8->I + i] = c8->V[i];
                    }
                    c8->I += vx + 1;
                    break;
                case 0x0065: // FX65 - Fill V0 to VX with vals from memory starting at I
                    for (int i = 0; i <= (vx); i++) {
                        c8->V[i] = c8->memory[c8->I + i];
                    }
                    c8->I += vx + 1;
                    break;
                default:
                    fprintf(stderr, "Unknown opcode: %x\n", c8->opcode);
                    return 1;
            }
            break;
        default:
            fprintf(stderr, "Unimplemented opcode\n");
            return 1;
    }
    return 0;
}

static int chip8_HandleEvents(struct chip8 *c8) {
    SDL_Event e;

    static const SDL_Keycode keymap[16] = {
        SDLK_1, SDLK_2, SDLK_3, SDLK_4,
        SDLK_a, SDLK_z, SDLK_e, SDLK_r,
        SDLK_q, SDLK_s, SDLK_d, SDLK_f,
        SDLK_w, SDLK_x, SDLK_c, SDLK_v
    };

    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT || e.key.keysym.sym == SDLK_ESCAPE) { return 1; }
        if(e.type == SDL_KEYUP || e.type == SDL_KEYDOWN) {
            for (int i = 0; i < 16; ++i) {
                if(e.key.keysym.sym == keymap[i]) { 
                    if(e.type == SDL_KEYUP) { c8->keys[i] = 0; }
                    if(e.type == SDL_KEYDOWN) { c8->keys[i] = 1; }
                }
            }
        }
    }

    return 0;
}

static void chip8_Render(SDL_Renderer *renderer, SDL_Texture *texture, struct chip8 *c8) {
    Uint32 pixels[GRAPHICS_SIZE];

    c8->drawflag = 0;
    for(int i = 0; i < GRAPHICS_SIZE; ++i) {
        pixels[i] = (0x00ffffff * c8->graphics[i]) | 0xff000000;
    }

    SDL_UpdateTexture(texture, NULL, pixels, DISPLAY_WIDTH * sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}