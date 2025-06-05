#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define MAX_ENEMIES 50
#define MAX_BULLETS 100

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
Texture2D pikachutex;

void ResetGame() {
    playerPos = (Vector2){400, 300};
    score = 0;
    gameOver = false;

    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
}

void SpawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            int side = GetRandomValue(0, 3);
            Vector2 pos;

            if (side == 0) pos = (Vector2){0, GetRandomValue(0, 600)};
            else if (side == 1) pos = (Vector2){800, GetRandomValue(0, 600)};
            else if (side == 2) pos = (Vector2){GetRandomValue(0, 800), 0};
            else pos = (Vector2){GetRandomValue(0, 800), 600};

            enemies[i].position = pos;
            enemies[i].velocity = Vector2Scale(Vector2Normalize(Vector2Subtract(playerPos, pos)), 60.0f);
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

int main() {
    InitWindow(800, 600, "CAPTURE OR ESCAPE");
    SetTargetFPS(60);

    emojiFont = LoadFont("resources/emoji_font.ttf");
    if (emojiFont.texture.id == 0) emojiFont = GetFontDefault();

    pokeballTex = LoadTexture("resources/pikachu.png");
    pikachutex= LoadTexture("resources/pokeball.png");

    ResetGame();
    float enemySpawnTimer = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (!gameOver) {
            // Player Movement
            if (IsKeyDown(KEY_LEFT))  playerPos.x -= playerSpeed * dt;
            if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed * dt;
            if (IsKeyDown(KEY_UP))    playerPos.y -= playerSpeed * dt;
            if (IsKeyDown(KEY_DOWN))  playerPos.y += playerSpeed * dt;

            // Shoot Bullet
            if (IsKeyPressed(KEY_SPACE)) ShootBullet();

            // Move Bullets
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    bullets[i].position.y += bullets[i].velocity.y * dt;
                    if (bullets[i].position.y < 0) bullets[i].active = false;
                }
            }

            // Spawn Enemies
            enemySpawnTimer += dt;
            if (enemySpawnTimer > 1.0f - (score * 0.01f)) {
                SpawnEnemy();
                enemySpawnTimer = 0;
            }

            // Move Enemies and Check Collisions
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    enemies[i].velocity = Vector2Scale(Vector2Normalize(Vector2Subtract(playerPos, enemies[i].position)), 60.0f);
                    enemies[i].position = Vector2Add(enemies[i].position, Vector2Scale(enemies[i].velocity, dt));

                    // Check collision with player
                    if (CheckCollisionCircles(playerPos, 25, enemies[i].position, 20)) {
                        gameOver = true;
                    }

                    // Check collision with bullets
                    for (int j = 0; j < MAX_BULLETS; j++) {
                        if (bullets[j].active &&
                            CheckCollisionCircles(bullets[j].position, 10, enemies[i].position, 20)) {
                            bullets[j].active = false;
                            enemies[i].active = false;
                            score++;
                        }
                    }
                }
            }
        }

        if (gameOver && IsKeyPressed(KEY_R)) {
            ResetGame();
        }

        // === DRAWING SECTION ===
        BeginDrawing();
        ClearBackground(GREEN);

        // Draw player Poké Ball
        DrawTexture(pokeballTex, playerPos.x - pokeballTex.width / 2, playerPos.y - pokeballTex.height / 2, WHITE);

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, WHITE);
            }
        }

        // Draw enemy Poké Balls (red tint)
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                DrawTexture(pikachutex, enemies[i].position.x - pikachutex.width / 2, enemies[i].position.y - pikachutex.height / 2,WHITE);
            }
        }

        // Draw score
        DrawText(TextFormat("Score: %d", score), 20, 20, 20, BLACK);

        // Game over message
        if (gameOver) {
            DrawText("OOPS THE POKEMON GOT CAPTURED!  Press R", 240, 280, 20, RED);
        }

        EndDrawing();
    }

    // Cleanup
    UnloadFont(emojiFont);
    UnloadTexture(pokeballTex);
    UnloadTexture(pikachutex);
    
    CloseWindow();
    return 0;
}



