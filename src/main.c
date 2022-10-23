#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "utils.h"

#define MAX_COMMAND_SIZE 20
#define MAX_FILENAME_BUFFER_SIZE 200

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    FILE *file = NULL;

    if(argc >= 3) {
        if(strnlen(argv[1], MAX_COMMAND_SIZE) == MAX_COMMAND_SIZE) { printCommandUsage(); goto Quit; }

        if(strncmp(argv[1], "-run", MAX_COMMAND_SIZE) == 0) {
            if(strnlen(argv[2], MAX_FILENAME_BUFFER_SIZE) == MAX_FILENAME_BUFFER_SIZE) { printCommandUsage(); goto Quit; }

            char filenameBuffer[MAX_FILENAME_BUFFER_SIZE];
            strncpy(filenameBuffer, argv[2], MAX_FILENAME_BUFFER_SIZE);

            int delayBetweenFrames = 2;
            if (argc == 5) {
                if(strnlen(argv[3], MAX_COMMAND_SIZE) == MAX_COMMAND_SIZE) { printCommandUsage(); goto Quit; }
                if (sscanf(argv[4], "%d", &delayBetweenFrames) != 1) {
                    fprintf(stderr, "Error : You must enter a number, it's a delay in ms\n");
                    return 1;
                }
            }

            file = fopen(filenameBuffer, "rb");
            if(file == NULL) { fprintf(stderr, "Error : Couldn't open the %s file", filenameBuffer); goto Quit; }

            if(SDL_Init(SDL_INIT_EVERYTHING) != 0) { goto Quit; }

            window = SDL_CreateWindow("Z_CHIP-8_Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
            if(window == NULL) { fprintf(stderr, "Error SDL_CreateWindow : %s", SDL_GetError()); goto Quit; }

            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
            if(renderer == NULL) { fprintf(stderr, "Error SDL_CreateRenderer : %s", SDL_GetError()); goto Quit; }
            
            SDL_RenderSetLogicalSize(renderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
            
            chip8_RunTheGame(renderer, file, delayBetweenFrames);
        } else if(strncmp(argv[1], "-debug", MAX_COMMAND_SIZE) == 0) {
            printf("Not implemented yet.\n");
        } else if(strncmp(argv[1], "-disas", MAX_COMMAND_SIZE) == 0) {
            printf("Not implemented yet.\n");
        } else {
            printCommandUsage();
            return 0;
        }
    } else {
        printCommandUsage();
        return 0;
    }

Quit:
    if(file != NULL) { fclose(file); }

    if(renderer != NULL) { SDL_DestroyRenderer(renderer); }
    if(window != NULL) { SDL_DestroyWindow(window); }
    SDL_Quit();
    
    return 0;
}