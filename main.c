#include <stdio.h>
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
    uint8_t size;
    /* floats in terms of blockSizes */
    float offset;  /* how far the next block has grown */
    float positions[STACK_SIZE];  /* y added to last block y */
    float velocities[STACK_SIZE];
} Stack;

static Stack stacks[STACK_COUNT] = {0};
static Stack holding = {0};

static Sound fxPlace;
static Sound fxPickup;
static Sound fxMisplace;
static Sound fxSpeedup;
static Sound fxGameover;
static Sound fxPop;

void pickup(uint32_t stackIndex);
void place(uint32_t stackIndex);
unsigned int pop();

int main(void) {
    /* Stack initialization */
    for (int i = 0; i < STACK_COUNT; i++) {
        stacks[i] = (Stack) {0};
        for (int j = 0; j < STACK_SIZE; j++) {
            stacks[i].stack[j] = -1;
        }
        stacks[i].next = GetRandomValue(0, 3);
    }
    for (int i = 0; i < STACK_SIZE; i++) {
        holding.stack[i] = -1;
    }

    /* gamestate variables */
    float growSpeed = BASE_SPEED;
    float speedBoost = 10.0f;
    int score = 0;
    int scoreGoal = BASE_POINTS * 100;
    int level = 1;
    int gameOver = 0;

    /* drawing variables */
    uint32_t screenWidth = DEFAULT_SCREEN_WIDTH;
    uint32_t screenHeight = DEFAULT_SCREEN_HEIGHT;
    uint32_t screenX = 0;
    uint32_t screenY = 0;
    char textBuffer[128] = {0};
    int stackPadding = 10;
    int blockSize = screenHeight / 20;
    int fieldWidth = (stackPadding * 2 + blockSize) * STACK_COUNT;
    int fieldHeight = blockSize * STACK_SIZE;
    Rectangle field = (Rectangle){(screenWidth - fieldWidth) / 2,
                                  screenHeight - (fieldHeight + (2 * blockSize)),
                                  fieldWidth, fieldHeight};
    /* init audio */
    InitAudioDevice();
    fxPlace = LoadSound("resources/place.wav");
    fxPickup = LoadSound("resources/pickup.wav");
    fxMisplace = LoadSound("resources/misplace.wav");
    fxSpeedup = LoadSound("resources/speedup.wav");
    fxGameover = LoadSound("resources/gameover.wav");
    fxPop = LoadSound("resources/pop.wav");

    /* init window */
    float frameTime;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Stack game");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        frameTime = GetFrameTime();

        if (!gameOver) {
            /* update offset and push to bottom of Stack when ready */
            for (int i = 0; i < STACK_COUNT; i++) {
                Stack *s = &(stacks[i]);
                s->offset += growSpeed * frameTime * ((s->size == 0) * speedBoost + 1);
                if (s->offset >= 1.0f) {
                    s->offset -= 1.0f;
                    s->size++;
                    for (int j = STACK_SIZE - 1; j > 0; j--) {
                        s->stack[j] = s->stack[j - 1];
                    }
                    s->stack[0] = s->next;
                    s->next = (char) GetRandomValue(0, 3);
                }
            }


            /* handle input */
            if (IsKeyPressed(KEY_J)) { place(0); }
            else if (IsKeyPressed(KEY_K)) { place(1); }
            else if (IsKeyPressed(KEY_L)) { place(2); }

            if (IsKeyPressed(KEY_S)) { pickup(0); }
            else if (IsKeyPressed(KEY_D)) { pickup(1); }
            else if (IsKeyPressed(KEY_F)) { pickup(2); }

            /* pop 3+ in a row */
            if (IsKeyPressed(KEY_SPACE)) {
                uint32_t points = pop();
                score += points;
                if (points > 0) {
                    PlaySound(fxPop);
                } else {
                    PlaySound(fxMisplace);
                }
                if (score >= scoreGoal) {
                    PlaySound(fxSpeedup);
                }
                while (score >= scoreGoal) {
                    growSpeed *= 1.1f;
                    scoreGoal += BASE_POINTS * 100 * powf(1.2, level);
                    level++;
                }
            }

            /* check for gameover */
            for (int i = 0; i < STACK_COUNT; i++) {
                if (stacks[i].size >= STACK_SIZE) {
                    PlaySound(fxGameover);
                    gameOver = 1;
                    break;
                }
            }
        }

        /* handle window resizes */
        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            uint32_t maxWidth = screenHeight * (9.0f/16.0f);
            uint32_t maxHeight = screenWidth * (9.0f/16.0f);
            if (screenWidth > maxWidth) {
                screenX = (screenWidth - maxWidth) / 2;
                screenY = 0;
                screenWidth = maxWidth;
            }
            if (screenHeight > maxHeight) {
                screenX = 0;
                screenY = (screenHeight - maxHeight) / 2;
                screenHeight = maxHeight;
            }
            blockSize = screenHeight / 20;
            stackPadding = blockSize / 4;
            fieldWidth = (stackPadding * 2 + blockSize) * STACK_COUNT;
            fieldHeight = blockSize * STACK_SIZE;
            field = (Rectangle){screenX + ((screenWidth - fieldWidth) / 2),
                                screenY + (screenHeight - (fieldHeight + (2 * blockSize))),
                                fieldWidth,
                                fieldHeight};
        }

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            DrawRectangleRec(field, GRAY);
            for (int i = 0; i < STACK_SIZE; i++) {
                /* draw holding stack */
                if (holding.stack[i] == -1) {
                    break;
                }
                DrawRectangle(field.x + fieldWidth + stackPadding,
                              field.y + fieldHeight - (i * blockSize),
                              blockSize, blockSize, colors[holding.stack[i]]);
            }
            for (int i = 0; i < STACK_COUNT; i++) {
                /* draw next */
                Color c = colors[stacks[i].next];
                c.r /= 2;
                c.g /= 2;
                c.b /= 2;
                DrawRectangle(field.x + stackPadding + ((stackPadding * 2 + blockSize) * i),
                              field.y + fieldHeight - ((stacks[i].offset) * blockSize),
                              blockSize, blockSize, c);
                /* draw stack */
                float y = stacks[i].offset;
                for (int j = 0; j < STACK_SIZE; j++) {
                    y += 1;
                    if (stacks[i].stack[j] == -1) {
                        assert(stacks[i].size == j);
                        break;
                    }
                    c = colors[stacks[i].stack[j]];
                    y += stacks[i].positions[j];
                    DrawRectangle(field.x + stackPadding + ((stackPadding * 2 + blockSize) * i),
                                  field.y + fieldHeight - (y * blockSize), blockSize, blockSize, c);
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
                DrawText("game over",field.x + ((field.width - gowidth) / 2),
                         field.y + (field.width / 2), fontSize, DARKGRAY);
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

void pickup(uint32_t stackIndex) {
    Stack *s = &stacks[stackIndex];
    if (holding.size < STACK_SIZE && s->size > 0) {
        holding.stack[holding.size] = s->stack[s->size - 1];
        holding.size++;
        s->stack[s->size - 1] = -1;
        s->size--;
        PlaySound(fxPickup);
    }
    else {
        PlaySound(fxMisplace);
    }
}

void place(unsigned int stackIndex) {
    Stack *s = &stacks[stackIndex];
    if (holding.size > 0) {
        s->stack[s->size] = holding.stack[holding.size - 1];
        s->size++;
        holding.stack[holding.size - 1] = -1;
        holding.size--;
        PlaySound(fxPlace);
    }
    else {
        PlaySound(fxMisplace);
    }
}

uint32_t pop() {
    uint32_t popped = 0;  /* number of blocks popped */
    uint32_t points = BASE_POINTS;  /* points per block */
    uint32_t score = 0;  /* total score from this pop */
    for (int i = 0; i < STACK_COUNT; i++) {
        Stack *s = &stacks[i];
        int run = 1;
        char lastColor = -1;
        for (int j = 0; j < STACK_SIZE; j++) {
            if (s->stack[j] != -1 && s->stack[j] == lastColor) {
                run++;
            }
            else {
                if (run >= 3) {
                    popped += run;
                    for (int k = j - run; k < STACK_SIZE; k++) {
                        score += points;
                        points *= 1.1f;
                        if (k + run >= STACK_SIZE) {
                            s->stack[k] = -1;
                        }
                        else {
                            s->stack[k] = s->stack[k + run];
                        }
                    }
                    s->size -= run;
                    j = 1;
                }
                run = 1;
                lastColor = s->stack[j];
            }
        }
    }
    return score;
}