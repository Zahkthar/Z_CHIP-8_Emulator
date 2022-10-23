#include "utils.h"

void printCommandUsage() {
    printf("+-----------------------------------------------------------------------------+\n");
    printf("| The emulator can be used with theses options :                              |\n");
    printf("|      -run [theFileName] -d [delayBetweenFrames - optionnal] to run the ROM  |\n");
    printf("|      -debug [theFileName] to run with the debugger                          |\n");
    printf("|      -disas [theFileName] to disassemble the ROM                            |\n");
    printf("+-----------------------------------------------------------------------------+\n");
}