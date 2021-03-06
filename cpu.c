#include "cpu.h"


void handleTimers(CHIP8_CPU *cpu) {
    if (cpu->DT != 0) {
        cpu->DT--;
    }
    if (cpu->ST != 0) {
        cpu->ST--;
    }
}

static uint8_t getSingularKeyPress(CHIP8_CPU *cpu) {
    uint8_t key = 0;
    uint8_t i = 0;
    for (;;) {
        if ((cpu->KEY >> (uint16_t)i) & 1) {
            return i;
        }
        i++;
        if (i >= 0xF) {
            i = 0;
        }
    }
    return 0;
}

#ifdef DEBUG
static void dumpRamToFile(CHIP8_CPU *cpu) {
    FILE *file = fopen("ramdump.bin", "wb+");
    fwrite(cpu->ramPtr->mem, 1, 4096, file);
    fclose(file);
}

static void debugDrawKeys(CHIP8_CPU *cpu, CHIP8_DISPLAY *display) {
    static SDL_Rect rect;
    rect.w = 10;
    rect.h = 10;
    rect.y = 10;
    for (uint8_t k = 0; k < 0xF; k++) {
        if ((cpu->KEY >> k) & 1) {
            cpu->displayPtr->frameBuf[0][k] ^= 1;
        } else {
            cpu->displayPtr->frameBuf[0][k] ^= 0;
        }
    }
}
#endif 

void initCPU(CHIP8_CPU *cpu, CHIP8_RAM *ram, CHIP8_DISPLAY *display) {
    memset(cpu->reg, 0, sizeof(cpu->reg));
    memset(cpu->reg, 0, sizeof(cpu->STACK));

    cpu->DT = 0;
    cpu->ST = 0;
    cpu->VF = &cpu->reg[15];
    cpu->SP = 0;
    cpu->I = 0;
    cpu->PC = 0x200;
    cpu->ramPtr = ram;
    cpu->displayPtr = display;
}

static void displaySprite(CHIP8_CPU *cpu, uint8_t x, uint8_t y, uint8_t spriteSize, uint8_t *sprite) {
    uint8_t pixelsErased = 0;
    for (int spriteY = 0; spriteY < spriteSize; spriteY++) {
        for (int spriteX = 0; spriteX < 8; spriteX++) {
            uint8_t bitStatus = (*(sprite + spriteY) >> (7 - spriteX)) & 1;
            if (!bitStatus)
                continue;
            uint8_t drawX, drawY;

            drawX = (x + spriteX) % 64;
            drawY = (y + spriteY) % 32;

            if (!pixelsErased) {
                if (cpu->displayPtr->frameBuf[drawY][drawX] & bitStatus) {
                    *(cpu->VF) = 1;
                    pixelsErased = 1;
                }
            }
            cpu->displayPtr->frameBuf[drawY][drawX] ^= bitStatus;
        }
    }
    if (!pixelsErased)
        *(cpu->VF) = 0;
    SDL_RenderClear(cpu->displayPtr->renderer);
    drawDisplay(cpu->displayPtr);
    SDL_RenderPresent(cpu->displayPtr->renderer);
}

void executeInstructions(CHIP8_CPU *cpu) {
    for (int cycles = 0; cycles <= 10; cycles++) {
        uint16_t opcode = (cpu->ramPtr->mem[cpu->PC] << 8) | cpu->ramPtr->mem[cpu->PC + 1];
        #ifdef DEBUG
        debugDrawKeys(cpu, cpu->displayPtr);
        #endif

        uint8_t instrHeader = (opcode >> 12) & 0xf;
        uint8_t num1 = (opcode >> 8) & 0xf;
        uint8_t num2 = (opcode >> 4) & 0xf;
        uint8_t num3 = opcode & 0xf;
        uint16_t num1to3 = opcode & 0xfff;
        uint8_t num2to3 = opcode & 0xff;

        switch (instrHeader) {
            case 0x0:
                switch (num3) {
                    case 0x0:
                        clearScreen(cpu->displayPtr);
                    break;
                    case 0xE:
                        cpu->PC = cpu->STACK[cpu->SP];
                        cpu->SP--;
                        continue;
                    break;
                }
            break;
            case 0x1: 
                cpu->PC = num1to3;
                continue;
            break;
            case 0x2: 
                cpu->SP++;
                cpu->STACK[cpu->SP] = cpu->PC + 2;
                cpu->PC = num1to3;
                continue;
            break;
            case 0x3:
                if (cpu->reg[num1] == num2to3) {
                    cpu->PC += 4;
                    continue;
                }
            break;
            case 0x4:
                if (cpu->reg[num1] != num2to3) {
                    cpu->PC += 4;
                    continue;
                }  
            break;
            case 0x5: 
                if (cpu->reg[num1] == cpu->reg[num2]) {
                    cpu->PC += 4;
                    continue;
                } 
            break;
            case 0x6: 
                cpu->reg[num1] = num2to3; 
            break;
            case 0x7: 
                cpu->reg[num1] += num2to3;
            break;
            case 0x8:
                switch (num3) {
                    case 0x0:
                        cpu->reg[num1] = cpu->reg[num2];
                    break;
                    case 0x1:
                        cpu->reg[num1] = cpu->reg[num1] | cpu->reg[num2];
                    break;
                    case 0x2:
                        cpu->reg[num1] = cpu->reg[num1] & cpu->reg[num2];
                    break;
                    case 0x3:
                        cpu->reg[num1] = cpu->reg[num1] ^ cpu->reg[num2];
                    break;
                    case 0x4:
                        cpu->reg[num1] += cpu->reg[num2];
                        if (cpu->reg[num1] + cpu->reg[num2] != (cpu->reg[num1] + cpu->reg[num3] & 0xFF)) {
                            *(cpu->VF) = 1;
                        } else {
                            *(cpu->VF) = 0;
                        }
                    break;
                    case 0x5:
                        if (cpu->reg[num1] > cpu->reg[num2]) {
                            *(cpu->VF) = 1;
                        } else {
                            *(cpu->VF) = 0;
                        }
                        cpu->reg[num1] -= cpu->reg[num2];
                    break;
                    case 0x6:
                        if (cpu->reg[num1] & 0b00000001) {
                            *(cpu->VF) = 1;
                        } else {
                            *(cpu->VF) = 0;
                        }
                        cpu->reg[num1] /= 2;
                    break;
                    case 0x7:
                        if (cpu->reg[num3] > cpu->reg[num2]) {
                            *(cpu->VF) = 1;
                        } else {
                            *(cpu->VF) = 0;
                        }
                        cpu->reg[num1] = cpu->reg[num2] - cpu->reg[num1];
                    break;
                    case 0xE:
                        if (cpu->reg[num1] & 0b10000000) {
                            *(cpu->VF) = 1;
                        } else {
                            *(cpu->VF) = 0;
                        }
                        cpu->reg[num1] *= 2;
                    break;
                }
            break;
            case 0x9:
                if (cpu->reg[num1] != cpu->reg[num2]) {
                    cpu->PC += 4;
                    continue;
                }
            break;
            case 0xA: 
                cpu->I = num1to3;
            break;
            case 0xB: 
                cpu->PC = num1to3 + cpu->reg[0];
                continue;
            break;
            case 0xC: 
                cpu->reg[num1] = (rand() % 255) & num2to3;
            break;
            case 0xD:
                displaySprite(cpu, cpu->reg[num1], cpu->reg[num2], num3, cpu->ramPtr->mem + cpu->I);
            break;
            case 0xE: 
                switch (num3) {
                    case 0x1:
                        if (!((cpu->KEY >> cpu->reg[num1]) & 1)) {
                            cpu->PC += 4;
                            continue;
                        }
                    break;
                    case 0xE:
                        if (((cpu->KEY >> cpu->reg[num1]) & 1)) {
                            cpu->PC += 4;
                            continue;
                        }
                    break;
                }
            break;
            case 0xF:
                switch (num3) {
                    case 0x3:
                        cpu->ramPtr->mem[cpu->I] = (cpu->reg[num1] / 100) % 10;
                        cpu->ramPtr->mem[cpu->I + 1] = (cpu->reg[num1] / 10) % 10;
                        cpu->ramPtr->mem[cpu->I + 2] = (cpu->reg[num1] % 10);
                    break;
                    case 0x5:
                        if (num2 == 0x1) {
                            cpu->DT = cpu->reg[num1];
                        } else if (num2 == 0x5) {
                            for (int i = 0; i <= num1; i++) {
                                cpu->ramPtr->mem[cpu->I + i] = cpu->reg[i];
                            }
                        } else {
                            for (int i = 0; i <= num1; i++) {
                                cpu->reg[i] = cpu->ramPtr->mem[cpu->I + i];
                            }
                        }
                    break;
                    case 0x7:
                        cpu->reg[num1] = cpu->DT;
                    break;
                    case 0x8:
                        cpu->ST = cpu->reg[num1];
                    break;
                    case 0x9:
                        cpu->I = 0x50 + cpu->reg[num1] * 5;
                    break;
                    case 0xA:
                        cpu->reg[num1] = getSingularKeyPress(cpu);
                    break;
                    case 0xE:
                        cpu->I += cpu->reg[num1];
                    break;
                }
            break;
        }
        cpu->PC += 2;
    }
}