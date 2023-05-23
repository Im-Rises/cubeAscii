#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-macro-to-enum"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <time.h>

#include "cUnicodeLib.h"

#define OPTION_CUBE_COUNT "-c"
#define OPTION_CUBE_GRAY_MODE "-g"
#define OPTION_HELP "-h"

#define FRAME_DELAY_MILLISECONDS 16

#define MAX_CUBE_COUNT 3

#define WITH_HEIGHT_SCALE_FACTOR 2

#define MIN_ROTATION_SPEED (-0.1F)
#define MAX_ROTATION_SPEED 0.1F

#define BACKGROUND_CHARACTER '.'
#define FACE_1_CHARACTER '@'
#define FACE_2_CHARACTER '$'
#define FACE_3_CHARACTER '~'
#define FACE_4_CHARACTER '#'
#define FACE_5_CHARACTER ';'
#define FACE_6_CHARACTER '+'

typedef struct Screen Screen;
struct Screen {
    int width, height;
    int size;
    float* zBuffer;
    char* buffer;
};

const float K1 = 40;

typedef struct Cube Cube;
struct Cube {
    float rotationX, rotationY, rotationZ;
    float rotationXSpeed, rotationYSpeed, rotationZSpeed;
    float cubeWidthHeight;
    float horizontalOffset, verticalOffset;
    int distanceFromCam;
};

Cube createCube() {
    Cube cube;
    cube.rotationX = 0;
    cube.rotationY = 0;
    cube.rotationZ = 0;
    cube.rotationXSpeed = 0.00F;
    cube.rotationYSpeed = 0.00F;
    cube.rotationZSpeed = 0.00F;
    cube.cubeWidthHeight = 40;
    cube.horizontalOffset = 0;
    cube.verticalOffset = 0;
    cube.distanceFromCam = 100;
    return cube;
}

Cube createCustomCube(float rotationX, float rotationY, float rotationZ, float rotationXSpeed, float rotationYSpeed, float rotationZSpeed, float cubeWidthHeight, float horizontalOffset, float verticalOffset, int distanceFromCam) {
    Cube cube;
    cube.rotationX = rotationX;
    cube.rotationY = rotationY;
    cube.rotationZ = rotationZ;
    cube.rotationXSpeed = rotationXSpeed;
    cube.rotationYSpeed = rotationYSpeed;
    cube.rotationZSpeed = rotationZSpeed;
    cube.cubeWidthHeight = cubeWidthHeight;
    cube.horizontalOffset = horizontalOffset;
    cube.verticalOffset = verticalOffset;
    cube.distanceFromCam = distanceFromCam;
    return cube;
}

/*
 * The three functions below are multiplication of the rotation matrix with the coordinates of a point.
 * Each one is used to calculate the position of a 3D point rotation around a specific axis (according to the cube rotation value).
 */

float calculateX(float i, float j, float k, Cube* cube) {
    return j * sinf(cube->rotationX) * sinf(cube->rotationY) * cosf(cube->rotationZ) - k * cosf(cube->rotationX) * sinf(cube->rotationY) * cosf(cube->rotationZ) +
           j * cosf(cube->rotationX) * sinf(cube->rotationZ) + k * sinf(cube->rotationX) * sinf(cube->rotationZ) +
           i * cosf(cube->rotationY) * cosf(cube->rotationZ);
}

float calculateY(float i, float j, float k, Cube* cube) {
    return j * cosf(cube->rotationX) * cosf(cube->rotationZ) + k * sinf(cube->rotationX) * cosf(cube->rotationZ) -
           j * sinf(cube->rotationX) * sinf(cube->rotationY) * sinf(cube->rotationZ) + k * cosf(cube->rotationX) * sinf(cube->rotationY) * sinf(cube->rotationZ) -
           i * cosf(cube->rotationY) * sinf(cube->rotationZ);
}

float calculateZ(float i, float j, float k, Cube* cube) {
    return k * cosf(cube->rotationX) * cosf(cube->rotationY) - j * sinf(cube->rotationX) * cosf(cube->rotationY) + i * sinf(cube->rotationY);
}

void calculateForSurface(Screen* screen, int cubeX, int cubeY, int cubeZ, Cube* cube, char ch) {
    // Calculate 3D coordinates
    float x = calculateX((float)cubeX, (float)cubeY, (float)cubeZ, cube);
    float y = calculateY((float)cubeX, (float)cubeY, (float)cubeZ, cube);
    float z = calculateZ((float)cubeX, (float)cubeY, (float)cubeZ, cube) + (float)cube->distanceFromCam;

    // Perspective projection
    float ooz = 1 / z;

    // 2D projection coordinates
    int xp = (int)((float)screen->width / 2 + cube->horizontalOffset + K1 * ooz * x * WITH_HEIGHT_SCALE_FACTOR); // Multiplied by WITH_HEIGHT_SCALE_FACTOR to compensate for the height scale factor
    int yp = (int)((float)screen->height / 2 + cube->verticalOffset + K1 * ooz * y);

    // Draw the character on the buffer according to its z coordinate
    // (if the z coordinate is closer to the camera than the previous one, it will be drawn on top of it)
    int idx = xp + yp * screen->width;
    if (idx >= 0 && idx < screen->size)
    {
        if (ooz > screen->zBuffer[idx])
        {
            screen->zBuffer[idx] = ooz;
            screen->buffer[idx] = ch;
        }
    }
}

void updateBuffers(Screen* screen, Cube* cube) {
    const int halfCubeLength = (int)(cube->cubeWidthHeight / 2);

    // Update the z buffer and text buffer with the cube points
    // Iterate through each points of a face of the cube and put it in the buffers
    for (int cubeX = -halfCubeLength; cubeX < halfCubeLength; cubeX++)
    {
        for (int cubeY = -halfCubeLength; cubeY < halfCubeLength; cubeY++)
        {
            calculateForSurface(screen, cubeX, cubeY, -halfCubeLength, cube, FACE_1_CHARACTER);
            calculateForSurface(screen, halfCubeLength, cubeY, cubeX, cube, FACE_2_CHARACTER);
            calculateForSurface(screen, -halfCubeLength, cubeY, -cubeX, cube, FACE_3_CHARACTER);
            calculateForSurface(screen, -cubeX, cubeY, halfCubeLength, cube, FACE_4_CHARACTER);
            calculateForSurface(screen, cubeX, -halfCubeLength, -cubeY, cube, FACE_5_CHARACTER);
            calculateForSurface(screen, cubeX, halfCubeLength, cubeY, cube, FACE_6_CHARACTER);
        }
    }
}

void rotateCube(Cube* cube) {
    cube->rotationX += cube->rotationXSpeed;
    cube->rotationY += cube->rotationYSpeed;
    cube->rotationZ += cube->rotationZSpeed;
}

// void sleepMilliseconds(int milliseconds) {
// #ifdef _WIN32
//     Sleep(milliseconds);
// #else
//     usleep(milliseconds * 1000);
// #endif
// }

void printToConsoleColored(Screen* screen) {
    printf(ESC_CURSOR_HOME);
    char prevChar = '\0'; // Initialize with null character
    for (int k = 0; k < screen->size; k++)
    {
        if (screen->buffer[k] != prevChar)
        {
            switch (screen->buffer[k])
            {
            case FACE_1_CHARACTER:
                printf(ESC_FG_RED);
                break;
            case FACE_2_CHARACTER:
                printf(ESC_FG_GREEN);
                break;
            case FACE_3_CHARACTER:
                printf(ESC_FG_BLUE);
                break;
            case FACE_4_CHARACTER:
                printf(ESC_FG_YELLOW);
                break;
            case FACE_5_CHARACTER:
                printf(ESC_FG_MAGENTA);
                break;
            case FACE_6_CHARACTER:
                printf(ESC_FG_CYAN);
                break;
            case BACKGROUND_CHARACTER:
                printf(ESC_RESET_FG);
                break;
            }
        }

        putchar(k % screen->width ? screen->buffer[k] : '\n');
        prevChar = screen->buffer[k];
    }
}

void clearScreen(Screen* screen) {
    memset(screen->buffer, BACKGROUND_CHARACTER, (size_t)screen->size);
    memset(screen->zBuffer, 0, (unsigned long)screen->size * sizeof(float));
}

void printToConsole(Screen* screen) {
    printf(ESC_CURSOR_HOME);
    for (int k = 0; k < screen->size; k++)
    {
        putchar(k % screen->width ? screen->buffer[k] : '\n');
    }
}

float randomFloat(float min, float max) {
    float scale = (float)rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

float randomRotationValue() {
    return randomFloat(MIN_ROTATION_SPEED, MAX_ROTATION_SPEED);
}

void printUsage(const char* programName) {
    printf("Usage: %s [OPTIONS]\n"
           "Options:\n"
           "  %s <count>    Number of cubes to render (default: 1) from 1 to %d\n"
           "  %s            Render in gray mode\n"
           "  %s            Print this help message\n"
           "\n",
        programName,
        OPTION_CUBE_COUNT, MAX_CUBE_COUNT,
        OPTION_CUBE_GRAY_MODE,
        OPTION_HELP);
}

void printUnknownArgumentError(const char* argument) {
    fprintf(stderr, "Unknown argument: %s\n", argument);
}

int main(int argc, char** argv) {
    /* Initialize the random number generator */
    srand(time(NULL));

    /* Argument variables */
    void (*printCubePtr)() = printToConsoleColored;
    int cubeCount = 1;

    /* Handle arguments */
    for (int i = 1; i < argc; i++)
    {
        printf("%s\n", argv[i]);
        if (strcmp(argv[i], OPTION_CUBE_COUNT) == 0)
        {
            if (i + 1 < argc)
            {
                cubeCount = atoi(argv[i + 1]);
                if (cubeCount < 1)
                {
                    fprintf(stderr, "Cube count must be greater than 0\n");
                    return 1;
                }
                else if (cubeCount > MAX_CUBE_COUNT)
                {
                    fprintf(stderr, "Cube count must be less than %d\n", MAX_CUBE_COUNT);
                    return 1;
                }
                i++;
            }
            else
            {
                printUsage(argv[0]);
                return 1;
            }
        }
        else if (strcmp(argv[i], OPTION_CUBE_GRAY_MODE) == 0)
        {
            printCubePtr = printToConsole;
        }
        else if (strcmp(argv[i], OPTION_HELP) == 0)
        {
            printUsage(argv[0]);
            return 0;
        }
        else
        {
            printUnknownArgumentError(argv[i]);
            return 1;
        }
    }

    /* Initialize unicode library */
    initUnicodeLib();

    /* Clear screen */
    printf(ESC_CLEAR_SCREEN);

    /* Initialize cube */
    Cube cubeArray[MAX_CUBE_COUNT];
    cubeArray[0] = createCube();
    cubeArray[0].rotationXSpeed = randomRotationValue();
    cubeArray[0].rotationYSpeed = randomRotationValue();
    cubeArray[0].rotationZSpeed = randomRotationValue();

    /* Initialize screen and cubes */
    Screen screen;
    if (cubeCount == 1)
    {
        screen.width = 70;
        screen.height = 30;
    }
    else if (cubeCount == 2)
    {
        screen.width = 100;
        screen.height = 30;
        cubeArray[0].horizontalOffset = 15;
        cubeArray[1] = createCustomCube(0, 0, 0, randomRotationValue(), randomRotationValue(), randomRotationValue(), 20, -30, 0, 100);
    }
    else if (cubeCount == 3)
    {
        screen.width = 120;
        screen.height = 30;
        cubeArray[0].horizontalOffset = 5;
        cubeArray[1] = createCustomCube(0, 0, 0, randomRotationValue(), randomRotationValue(), randomRotationValue(), 20, -42, 0, 100);
        cubeArray[2] = createCustomCube(0, 0, 0, randomRotationValue(), randomRotationValue(), randomRotationValue(), 10, +45, 0, 100);
    }
    else
    {
        fprintf(stderr, "Cube count must be less than %d\n", MAX_CUBE_COUNT);
        return 1;
    }

    screen.size = screen.width * screen.height;
    screen.buffer = malloc(screen.size * sizeof(char));
    screen.zBuffer = malloc(screen.size * sizeof(float));

    // start timer
    clock_t startClock = clock();

    /* Main loop */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1)
    {
        /* Refresh buffers */
        clearScreen(&screen);

        /* Update buffers */
        for (int i = 0; i < cubeCount; i++)
        {
            updateBuffers(&screen, &cubeArray[i]);
        }

        /* Display buffers to console */
        printCubePtr(&screen);

        /* Rotate cube */
        for (int i = 0; i < cubeCount; i++)
        {
            rotateCube(&cubeArray[i]);
        }

        /* Delay */
        clock_t endClock = clock();
        while (endClock - startClock < FRAME_DELAY_MILLISECONDS * CLOCKS_PER_SEC / 1000)
        {
            endClock = clock();
        }
        startClock = endClock;
    }
#pragma clang diagnostic pop

    free(screen.buffer);
    free(screen.zBuffer);

    return 0;
}

#pragma clang diagnostic pop