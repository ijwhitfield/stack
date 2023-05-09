#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "raylib.h"

#define DEFAULT_SCREEN_WIDTH 400
#define DEFAULT_SCREEN_HEIGHT 711
#define STACK_SIZE 16
#define STACK_COUNT 3
#define BASE_SPEED 0.2f
#define BASE_POINTS 30

static const Color colors[] = {RED, GREEN, BLUE, VIOLET};

typedef struct {
    int8_t next;
    int8_t stack[STACK_SIZE];
    uint8_t stackHeight;
    float positions[STACK_SIZE];
} Stack;

int main(void) {
    /* Stack initialization */
    Stack stacks[STACK_COUNT] = {0};
    for (int i = 0; i < STACK_COUNT; i++) {
        stacks[i] = (Stack) {0};
        for (int j = 0; j < STACK_SIZE; j++) {
            stacks[i].stack[j] = -1;
        }
        stacks[i].next = GetRandomValue(0, 3);
    }

    /* gamestate variables */
    float growOffset = 0.0f;
    float growSpeed = BASE_SPEED;
    char holding = -1;
    int score = 0;
    int scoreGoal = BASE_POINTS * 10;
    int level = 1;
    int gameOver = 0;

    /* drawing variables */
    int screenWidth = DEFAULT_SCREEN_WIDTH;
    int screenHeight = DEFAULT_SCREEN_HEIGHT;
    char textBuffer[128] = {0};
    int stackPadding = 10;
    int blockSize = screenHeight / 20;
    int fieldWidth = (stackPadding * 2 + blockSize) * STACK_COUNT;
    int fieldHeight = blockSize * STACK_SIZE;
    Rectangle field = (Rectangle){(screenWidth - fieldWidth) / 2,
                                  screenHeight - (fieldHeight + (2 * blockSize)),
                                  fieldWidth,
                                  fieldHeight};
    /* init audio */
    InitAudioDevice();
    Sound fxPlace = LoadSound("resources/place.wav");
    Sound fxPickup = LoadSound("resources/pickup.wav");
    Sound fxMisplace = LoadSound("resources/misplace.wav");
    Sound fxSpeedup = LoadSound("resources/speedup.wav");
    Sound fxGameover = LoadSound("resources/gameover.wav");
    Sound fxPop = LoadSound("resources/pop.wav");

    /* init window */
    float frameTime;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Stack game");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        frameTime = GetFrameTime();

        if (!gameOver) {
            /* update growOffset and push to bottom of Stack when ready */
            if (growOffset >= 1.0f) {
                for (int i = 0; i < STACK_COUNT; i++) {
                    stacks[i].stackHeight++;
                    for (int j = STACK_SIZE - 1; j > 0; j--) {
                        stacks[i].stack[j] = stacks[i].stack[j - 1];
                    }
                    stacks[i].stack[0] = stacks[i].next;
                    stacks[i].next = (char) GetRandomValue(0, 3);
                }
                growOffset = 0.0f;
            }
            growOffset += growSpeed * frameTime;

            /* handle input */
            int stackPick = -1;
            if (IsKeyPressed(KEY_J)) {
                stackPick = 0;
            }
            else if (IsKeyPressed(KEY_K)) {
                stackPick = 1;
            }
            else if (IsKeyPressed(KEY_L)) {
                stackPick = 2;
            }
            if (stackPick != -1) {
                Stack *s = &stacks[stackPick];
                if (holding == -1) {  /* if pressed a Stack button and not holding anything */
                    if (s->stackHeight > 0) {  /* if there's at least one block above ground */
                        holding = s->stack[s->stackHeight - 1];
                        s->stack[s->stackHeight - 1] = -1;
                        s->stackHeight--;
                        PlaySound(fxPickup);
                    }
                    else {
                        PlaySound(fxMisplace);
                    }
                }
                else {
                    s->stack[s->stackHeight] = holding;
                    s->stackHeight++;
                    holding = -1;
                    PlaySound(fxPlace);
                }
            }

            /* check for 3 in a rows */
            if (IsKeyPressed(KEY_SPACE)) {
                int popped = 0;
                for (int i = 0; i < STACK_COUNT; i++) {
                    int run = 1;
                    char lastColor = -1;
                    for (int j = 0; j < STACK_SIZE; j++) {
                        if (stacks[i].stack[j] != -1 && stacks[i].stack[j] == lastColor) {
                            run++;
                        }
                        else {
                            if (run >= 3) {
                                popped += run;
                                for (int k = j - run; k < STACK_SIZE; k++) {
                                    if (k + run >= STACK_SIZE) {
                                        stacks[i].stack[k] = -1;
                                    }
                                    else {
                                        stacks[i].stack[k] = stacks[i].stack[k + run];
                                    }
                                }
                                stacks[i].stackHeight -= run;
                                j = 1;
                                if (score >= scoreGoal) {
                                    growSpeed *= 1.2f;
                                    scoreGoal += BASE_POINTS * 10 * powf(1.2, level - 1);
                                    level++;
                                    PlaySound(fxSpeedup);
                                }
                            }
                            run = 1;
                            lastColor = stacks[i].stack[j];
                        }
                    }
                }
                if (popped > 0) {
                    score += BASE_POINTS * powf(1.2, popped - 1);
                    PlaySound(fxPop);
                }
                else {
                    PlaySound(fxMisplace);
                }
            }

            /* check for gameover */
            for (int i = 0; i < STACK_COUNT; i++) {
                if (stacks[i].stackHeight >= STACK_SIZE) {
                    PlaySound(fxGameover);
                    gameOver = 1;
                    break;
                }
            }
        }

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            if (screenWidth > (screenHeight * (9.0f/16.0f))) {
                screenWidth = screenHeight * (9.0f/16.0f);
            }
            if (screenHeight > (screenWidth * (16.0f/9.0f))) {
                screenHeight = screenWidth * (16.0f/9.0f);
            }
            blockSize = screenHeight / 20;
            stackPadding = blockSize / 4;
            fieldWidth = (stackPadding * 2 + blockSize) * STACK_COUNT;
            fieldHeight = blockSize * STACK_SIZE;
            field = (Rectangle){(screenWidth - fieldWidth) / 2,
                                screenHeight - (fieldHeight + (2 * blockSize)),
                                fieldWidth,
                                fieldHeight};
        }

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            DrawRectangleRec(field, GRAY);
            DrawRectangle((screenWidth - blockSize)/2, stackPadding, blockSize, blockSize, colors[holding]);
            for (int i = 0; i < STACK_COUNT; i++) {
                Color c = colors[stacks[i].next];
                c.r /= 2;
                c.g /= 2;
                c.b /= 2;
                DrawRectangle(field.x + stackPadding + ((stackPadding * 2 + blockSize) * i),
                              field.y + fieldHeight - ((growOffset) * blockSize),
                              blockSize, blockSize, c);
                for (int j = 0; j < STACK_SIZE; j++) {
                    if (stacks[i].stack[j] == -1) {
                        //assert(stacks[i].stackHeight == j);
                        break;
                    }
                    c = colors[stacks[i].stack[j]];
                    DrawRectangle(field.x + stackPadding + ((stackPadding * 2 + blockSize) * i),
                                  field.y + fieldHeight - ((growOffset + 1 + j) * blockSize),
                                  blockSize, blockSize, c);
                }
            }
            int fontSize = blockSize / 2;
            sprintf(textBuffer, "score: %d", score);
            DrawText(textBuffer, stackPadding, stackPadding, fontSize, DARKGRAY);
            sprintf(textBuffer, "level: %d", level);
            DrawText(textBuffer, stackPadding, stackPadding + fontSize, fontSize, DARKGRAY);
            sprintf(textBuffer, "next: %d", scoreGoal);
            DrawText(textBuffer, stackPadding, stackPadding + (fontSize * 2), fontSize, DARKGRAY);
            int jTextPos = field.x + stackPadding + (blockSize/2);
            int kTextPos = jTextPos + blockSize + (stackPadding * 2);
            int lTextPos = kTextPos + blockSize + (stackPadding * 2);
            jTextPos -= MeasureText("j",fontSize)/2;
            kTextPos -= MeasureText("k",fontSize)/2;
            lTextPos -= MeasureText("l",fontSize)/2;
            DrawText("j", jTextPos, screenHeight - fontSize - stackPadding, fontSize, DARKGRAY);
            DrawText("k", kTextPos, screenHeight - fontSize - stackPadding, fontSize, DARKGRAY);
            DrawText("l", lTextPos, screenHeight - fontSize - stackPadding, fontSize, DARKGRAY);
            if (gameOver) {
                int gowidth = MeasureText("game over", fontSize);
                DrawText("game over", (screenWidth - gowidth)/2 , 10, fontSize, DARKGRAY);

            }
            DrawRectangle(field.x, field.y + field.height, field.width, blockSize, DARKGRAY);
        }
        EndDrawing();
    }

    UnloadSound(fxPlace);
    UnloadSound(fxPickup);
    UnloadSound(fxMisplace);
    UnloadSound(fxSpeedup);
    UnloadSound(fxGameover);
    UnloadSound(fxPop);
    CloseWindow();

    return 0;
}