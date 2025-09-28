#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_BALLS 10
#define BALL_RADIUS 20
#define BALL_SPEED 4
typedef struct Ball {
Vector2 position;
Color color;
bool active;
} Ball;
int main() {
InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Falling Balls with Basket and Background");
SetTargetFPS(60);
srand(time(NULL));
// Load background image
Texture2D background = LoadTexture("background3.png");
// Load basket image
Texture2D basketTex = LoadTexture("bg_fruit_bask.png");
// Basket properties
float basketX = SCREEN_WIDTH / 2 - 75;
float basketY = SCREEN_HEIGHT - 80;
float basketWidth = 150;
float basketHeight = 30;
float basketSideHeight = 50;
int basketSpeed = 7;
// Balls setup
Ball balls[MAX_BALLS];
for (int i = 0; i < MAX_BALLS; i++) {
balls[i].position = (Vector2){ GetRandomValue(BALL_RADIUS, SCREEN_WIDTH - BALL_RADIUS), 
GetRandomValue(-600, 0) };
balls[i].color = (Color){ GetRandomValue(50,255), GetRandomValue(50,255), GetRandomValue(50,255), 255 };
balls[i].active = true;
}
int score = 0;
while (!WindowShouldClose()) {
// Basket movement
if (IsKeyDown(KEY_LEFT) && basketX > 0) basketX -= basketSpeed;
if (IsKeyDown(KEY_RIGHT) && basketX + basketWidth < SCREEN_WIDTH) basketX += basketSpeed;
// Update balls
int activeBalls = 0;
for (int i = 0; i < MAX_BALLS; i++) {
if (balls[i].active) {
balls[i].position.y += BALL_SPEED;
// Check collision with basket base
if (balls[i].position.y + BALL_RADIUS >= basketY &&
balls[i].position.x >= basketX &&
balls[i].position.x <= basketX + basketWidth) {
score++;
balls[i].active = false;
}
// Reset ball if it goes below screen
if (balls[i].position.y - BALL_RADIUS > SCREEN_HEIGHT) {
balls[i].position.y = GetRandomValue(-600, 0);
balls[i].position.x = GetRandomValue(BALL_RADIUS, SCREEN_WIDTH - BALL_RADIUS);
balls[i].color = (Color){ GetRandomValue(50,255), GetRandomValue(50,255), 
GetRandomValue(50,255), 255 };
balls[i].active = true;
}
activeBalls++;
}
}
// Ensure at least 2 balls are active
if (activeBalls < 2) {
for (int i = 0; i < MAX_BALLS; i++) {
if (!balls[i].active) {
balls[i].active = true;
balls[i].position = (Vector2){ GetRandomValue(BALL_RADIUS, SCREEN_WIDTH - BALL_RADIUS), 
GetRandomValue(-600, 0) };
balls[i].color = (Color){ GetRandomValue(50,255), GetRandomValue(50,255), 
GetRandomValue(50,255), 255 };
activeBalls++;
if (activeBalls >= 2) break;
}
}
}
// Draw everything
BeginDrawing();
ClearBackground(RAYWHITE);
// Draw background image
DrawTexture(background, 0, 0, WHITE);
// Draw balls
for (int i = 0; i < MAX_BALLS; i++) {
if (balls[i].active) {
DrawCircleV(balls[i].position, BALL_RADIUS, balls[i].color);
}
}
// Draw basket using PNG
if (basketTex.id != 0) {
    // Crop white borders (assuming ~50px border on each side based on similar images)
    float border = 50.0f;
    Rectangle sourceRec = { border, border, 
                          (float)basketTex.width - 2 * border, 
                          (float)basketTex.height - 2 * border };
    // Scale to fit basketWidth (150px) and adjust height
    float scaleX = basketWidth / (sourceRec.width);
    float scaleY = basketHeight / (sourceRec.height);
    Rectangle destRec = { basketX, basketY - basketHeight, 
                        basketWidth, basketHeight };
    DrawTexturePro(
        basketTex,
        sourceRec,
        destRec,
        (Vector2){0, 0}, 0.0f, WHITE
    );
} else {
    // Fallback if basket texture is missing
    DrawRectangle(basketX, basketY, basketWidth, basketHeight, BROWN);
    Vector2 leftTriangle[3] = {
        {basketX, basketY},
        {basketX, basketY - basketSideHeight},
        {basketX + 25, basketY}
    };
    Vector2 rightTriangle[3] = {
        {basketX + basketWidth, basketY},
        {basketX + basketWidth, basketY - basketSideHeight},
        {basketX + basketWidth - 25, basketY}
    };
    DrawTriangle(leftTriangle[0], leftTriangle[1], leftTriangle[2], BROWN);
    DrawTriangle(rightTriangle[0], rightTriangle[1], rightTriangle[2], BROWN);
}
// Draw score
DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
EndDrawing();
}
// Cleanup
if (background.id != 0) UnloadTexture(background);
if (basketTex.id != 0) UnloadTexture(basketTex);
CloseWindow();
return 0;
}