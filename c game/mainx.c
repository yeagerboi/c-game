#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define MAX_ENEMIES 100
#define MAX_BULLETS 500

typedef enum {
    OPENING_SCENE,
    GAMEPLAY,
    CLOSING_SCENE
} GameState;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
} Difficulty;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Bullet;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Enemy;

Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];
Vector2 playerPos;
float playerSpeed = 200.0f;
int score = 0;
bool gameOver = false;
Font emojiFont;
Texture2D pokeballTex;
Texture2D pikachuTex;
Texture2D logo;
Texture2D balhTex; // Added texture for game over image

void ResetGame(Difficulty difficulty) {
    playerPos = (Vector2){400, 300};
    score = 0;
    gameOver = false;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
}

void SpawnEnemy(Difficulty difficulty) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            int side = GetRandomValue(0, 3);
            Vector2 pos;

            switch (side) {
                case 0: pos = (Vector2){0, GetRandomValue(0, screenHeight)}; break;
                case 1: pos = (Vector2){screenWidth, GetRandomValue(0, screenHeight)}; break;
                case 2: pos = (Vector2){GetRandomValue(0, screenWidth), 0}; break;
                case 3: pos = (Vector2){GetRandomValue(0, screenWidth), screenHeight}; break;
            }

            float speed = (difficulty == DIFFICULTY_EASY) ? 50.0f :
                          (difficulty == DIFFICULTY_MEDIUM) ? 60.0f : 70.0f;

            enemies[i].position = pos;
            enemies[i].velocity = Vector2Scale(Vector2Normalize(Vector2Subtract(playerPos, pos)), speed);
            enemies[i].active = true;
            break;
        }
    }
}

void ShootBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].position = playerPos;
            bullets[i].velocity = (Vector2){0, -400};
            bullets[i].active = true;
            break;
        }
    }
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Capture or Escape");
    SetTargetFPS(60);

    // --- Load assets with checks ---
    if (!FileExists("resources/logo.png")) TraceLog(LOG_WARNING, "logo.png missing!");
    logo = LoadTexture("resources/logo.png");

    if (!FileExists("resources/emoji_font.ttf")) TraceLog(LOG_WARNING, "emoji_font.ttf missing! Using default.");
    emojiFont = LoadFont("resources/emoji_font.ttf");
    if (emojiFont.texture.id == 0) emojiFont = GetFontDefault();

    if (!FileExists("resources/pikachu.png")) TraceLog(LOG_WARNING, "pikachu.png missing!");
    pikachuTex = LoadTexture("resources/pikachu.png");

    const char* pokeballPath = "resources/pokeball.png";
    if (!IsPathFile(pokeballPath)) TraceLog(LOG_WARNING, "pokeball.png missing!");
    pokeballTex = LoadTexture(pokeballPath);

    const char* balhPath = "resources/balh.png";
    if (!IsPathFile(balhPath)) TraceLog(LOG_WARNING, "balh.png missing!");
    balhTex = LoadTexture(balhPath);


    GameState gameState = OPENING_SCENE;
    Difficulty selectedDifficulty = DIFFICULTY_MEDIUM;

    Rectangle easyBtn = { screenWidth/2 - 100, 300, 200, 50 };
    Rectangle mediumBtn = { screenWidth/2 - 100, 370, 200, 50 };
    Rectangle hardBtn = { screenWidth/2 - 100, 440, 200, 50 };
    Rectangle startBtn = { screenWidth/2 - 100, 510, 200, 50 };

    float gameOverScale = 0.1f;
    float scaleSpeed = 1.5f;
    bool animationComplete = false;
    float enemySpawnTimer = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        switch (gameState) {
            case OPENING_SCENE:
                if (CheckCollisionPointRec(GetMousePosition(), easyBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    selectedDifficulty = DIFFICULTY_EASY;

                if (CheckCollisionPointRec(GetMousePosition(), mediumBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    selectedDifficulty = DIFFICULTY_MEDIUM;

                if (CheckCollisionPointRec(GetMousePosition(), hardBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    selectedDifficulty = DIFFICULTY_HARD;

                if (CheckCollisionPointRec(GetMousePosition(), startBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    ResetGame(selectedDifficulty);
                    gameState = GAMEPLAY;
                }
                break;

            case GAMEPLAY:
                if (!gameOver) {
                    if (IsKeyDown(KEY_LEFT))  playerPos.x -= playerSpeed * dt;
                    if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed * dt;
                    if (IsKeyDown(KEY_UP))    playerPos.y -= playerSpeed * dt;
                    if (IsKeyDown(KEY_DOWN))  playerPos.y += playerSpeed * dt;

                    if (IsKeyPressed(KEY_SPACE)) ShootBullet();

                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active) {
                            bullets[i].position.y += bullets[i].velocity.y * dt;
                            if (bullets[i].position.y < 0) bullets[i].active = false;
                        }
                    }

                    float spawnInterval = (selectedDifficulty == DIFFICULTY_EASY) ? 1.5f :
                                         (selectedDifficulty == DIFFICULTY_MEDIUM) ? 1.0f : 0.7f;
                    enemySpawnTimer += dt;
                    if (enemySpawnTimer > spawnInterval - (score * 0.01f)) {
                        SpawnEnemy(selectedDifficulty);
                        enemySpawnTimer = 0;
                    }

                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (enemies[i].active) {
                            enemies[i].position = Vector2Add(enemies[i].position, Vector2Scale(enemies[i].velocity, dt));
                            
                            // Check collision with player
                            if (CheckCollisionCircles(enemies[i].position, 20, playerPos, 20)) {
                                gameOver = true;
                                gameState = CLOSING_SCENE;
                                break;
                            }
                            
                            // Check collision with bullets
                            for (int j = 0; j < MAX_BULLETS; j++) {
                                if (bullets[j].active && CheckCollisionCircles(enemies[i].position, 20, bullets[j].position, 5)) {
                                    enemies[i].active = false;
                                    bullets[j].active = false;
                                    score++;
                                    break;
                                }
                            }
                        }
                    }
                }
                break;

            case CLOSING_SCENE:
                if (!animationComplete) {
                    gameOverScale += scaleSpeed * dt;
                    if (gameOverScale >= 1.0f) {
                        gameOverScale = 1.0f;
                        animationComplete = true;
                    }
                }

                if (animationComplete) {
                    if (IsKeyPressed(KEY_R)) {
                        ResetGame(selectedDifficulty);
                        gameState = GAMEPLAY;
                    }
                    if (IsKeyPressed(KEY_H)) {
                        gameState = OPENING_SCENE;
                        ResetGame(selectedDifficulty);
                    }
                }
                break;
        }

        BeginDrawing();
        ClearBackground(gameState == GAMEPLAY ? GREEN : RAYWHITE);

        switch (gameState) {
            case OPENING_SCENE:
                DrawTexture(logo, screenWidth/2 - logo.width/2, 50, WHITE);
                DrawTextEx(emojiFont, "Select Game Difficulty", (Vector2){screenWidth/2 - 160, 250}, 30, 2, DARKGRAY);

                DrawRectangleRec(easyBtn, selectedDifficulty == DIFFICULTY_EASY ? LIME : LIGHTGRAY);
                DrawTextEx(emojiFont, "Easy", (Vector2){easyBtn.x + 70, easyBtn.y + 15}, 20, 2, DARKGRAY);

                DrawRectangleRec(mediumBtn, selectedDifficulty == DIFFICULTY_MEDIUM ? LIME : LIGHTGRAY);
                DrawTextEx(emojiFont, "Medium", (Vector2){mediumBtn.x + 55, mediumBtn.y + 15}, 20, 2, DARKGRAY);

                DrawRectangleRec(hardBtn, selectedDifficulty == DIFFICULTY_HARD ? LIME : LIGHTGRAY);
                DrawTextEx(emojiFont, "Hard", (Vector2){hardBtn.x + 70, hardBtn.y + 15}, 20, 2, DARKGRAY);

                DrawRectangleRec(startBtn, SKYBLUE);
                DrawTextEx(emojiFont, "Start", (Vector2){startBtn.x + 65, startBtn.y + 10}, 30, 2, DARKBLUE);
                break;

            case GAMEPLAY:
                DrawTexture(pikachuTex, playerPos.x - pikachuTex.width/2, playerPos.y - pikachuTex.height/2, WHITE);

                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (bullets[i].active) DrawCircleV(bullets[i].position, 5, WHITE);
                }

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        DrawTexture(pokeballTex, enemies[i].position.x - pokeballTex.width / 2, enemies[i].position.y - pokeballTex.height / 2, WHITE);
                    }
                }

                DrawTextEx(emojiFont, TextFormat("Score: %d", score), (Vector2){20, 20}, 20, 2, BLACK);
                break;

            case CLOSING_SCENE:
                // Draw the new image above the text, centered
                DrawTexture(balhTex, screenWidth/2 - balhTex.width/2, screenHeight/2 - balhTex.height - 50, WHITE);
                DrawTextEx(emojiFont, "OOPS THE POKEMON IS CAPTURED!", (Vector2){screenWidth/2 - 300, screenHeight/2 - 50}, 40 * gameOverScale, 2, RED);

                if (animationComplete) {
                    DrawTextEx(emojiFont, "Press R to Replay", (Vector2){screenWidth/2 - 110, screenHeight/2 + 30}, 20, 2, DARKGRAY);
                    DrawTextEx(emojiFont, "Press H to go to Home Menu", (Vector2){screenWidth/2 - 160, screenHeight/2 + 60}, 20, 2, DARKGRAY);
                }
                break;
        }

        EndDrawing();
    }

    // Cleanup
    UnloadTexture(logo);
    UnloadFont(emojiFont);
    UnloadTexture(pokeballTex);
    UnloadTexture(pikachuTex);
    UnloadTexture(balhTex); // Unload new image
    CloseWindow();
    return 0;
}