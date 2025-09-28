#include<stdio.h>
#include<time.h>
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

// Simple blur shader source
static const char *blurShaderCode =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 resolution;\n"
"void main() {\n"
"    vec2 texel = 1.0 / resolution;\n"
"    vec4 sum = vec4(0.0);\n"
"    for(int x=-2; x<=2; x++) {\n"
"        for(int y=-2; y<=2; y++) {\n"
"            sum += texture(texture0, fragTexCoord + vec2(x,y) * texel);\n"
"        }\n"
"    }\n"
"    finalColor = sum / 25.0;\n"
"}\n";

#define MAX_ENEMIES   100
#define MAX_BULLETS   500
#define MAX_OBSTACLES 4
#define NUM_PINS      10
#define MAX_BALLS     10

// Bowling sizes/speeds
#define BOWLING_BALL_RADIUS   15
#define PIN_RADIUS    20
#define LANE_LEFT     140
#define LANE_RIGHT    660

// Falling balls defines
#define BALLS_RADIUS 20
#define BALL_SPEED 4
#define TARGET_SCORE 5
#define MAX_MISSES 5

// Wordle defines
#define WORD_LENGTH 5
#define MAX_GUESSES 6

typedef enum {
    OPENING_SCENE,
    DIFFICULTY_TRANSITION,
    GAMEPLAY,
    REVIVE_PROMPT,
    MINI_GAME,
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
    float speed;
    bool active;
    int type; // 0: pokeball, 1: ultra, 2: master
    int health;
} Enemy;

typedef struct {
    Rectangle rect;
    bool active;
} Obstacle;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    bool fallen;
    bool animating;
} Pin;

// Falling balls struct
typedef struct {
    Vector2 position;
    Color color;
    bool active;
} Ball;

// ------------ Globals ------------
Bullet   bullets[MAX_BULLETS] = {0};
Enemy    enemies[MAX_ENEMIES] = {0};
Obstacle obstacles[MAX_OBSTACLES] = {0};
Pin      pins[NUM_PINS] = {0};

Vector2  playerPos = {400, 300};
float    basePlayerSpeed = 200.0f;
float    playerSpeed = 200.0f;
int      score = 0;
bool     gameOver = false;
bool     secondChanceUsed = false;
float    gameTimer = 0.0f;

Font     emojiFont = {0};
Texture2D pokeballTex = {0};
Texture2D ultraBallTex = {0};
Texture2D masterBallTex = {0};
Texture2D pikachuTex = {0};
Texture2D raichuTex = {0};
Texture2D logo = {0};
Texture2D balhTex = {0};
Texture2D obstacleTex = {0};
Texture2D aoiTex = {0};
Texture2D easyBg = {0};
Texture2D mediumBg = {0};
Texture2D hardBg = {0};
Texture2D homeBg = {0};
Texture2D normalBulletTex = {0};
Texture2D specialBulletTex = {0};

// Background music
Music    bgm = {0};
bool     musicPlaying = false;

// Death sound
Sound    deadSound = {0};
bool     deadSoundPlayed = false;

// Elixir buff system
Texture2D elixirTex = {0};
bool     elixirAvailable = false;
Vector2  elixirPos = {0};
bool     elixirReady = false;
float    elixirSpawnTimer = 0.0f;
float    elixirDurationTimer = 0.0f;
float    elixirSpawnInterval = 0.0f;
const float ELIXIR_DURATION = 8.0f;

// Elixir effect variables
bool     elixirEffectActive = false;
float    elixirEffectTimer = 0.0f;
const float ELIXIR_EFFECT_DURATION = 8.0f;

// Thunderstone buff system
Texture2D thunderstoneTex = {0};
bool     thunderstoneAvailable = false;
Vector2  thunderstonePos = {0};
float    thunderstoneSpawnTimer = 0.0f;
float    thunderstoneDurationTimer = 0.0f;
float    thunderstoneSpawnInterval = 0.0f;
const float THUNDERSTONE_DURATION = 5.0f;

// Thunderstone effect variables
bool     thunderstoneEffectActive = false;
float    thunderstoneEffectTimer = 0.0f;
const float THUNDERSTONE_EFFECT_DURATION = 8.0f;

// Speed buff system
Texture2D speedTex = {0};
bool     speedAvailable = false;
Vector2  speedPos = {0};
float    speedSpawnTimer = 0.0f;
float    speedDurationTimer = 0.0f;
float    speedSpawnInterval = 0.0f;
const float SPEED_DURATION = 8.0f;

// Speed effect variables
bool     speedEffectActive = false;
float    speedEffectTimer = 0.0f;
const float SPEED_EFFECT_DURATION = 8.0f;

// Power buff system
Texture2D powerTex = {0};
bool     powerAvailable = false;
Vector2  powerPos = {0};
float    powerSpawnTimer = 0.0f;
float    powerDurationTimer = 0.0f;
float    powerSpawnInterval = 0.0f;
const float POWER_DURATION = 8.0f;

// Power effect variables
bool     powerEffectActive = false;
float    powerEffectTimer = 0.0f;
const float POWER_EFFECT_DURATION = 8.0f;

// Bowling state
Vector2  ballPos = {0};
float    t = 0.0f;
float    throwAngle = 0.0f;
const float maxAngle = PI / 6;
Vector2  ellipseCenter = {0};
float    a = 100.0f;
float    b = 500.0f;
float    baseSpeed = 0.02f;
float    ballSpeed = 0.0f;
bool     ballLaunched = false;
float    power = 0.0f;
float    maxPower = 1.0f;
bool     charging = false;
bool     strikeMode = false;
bool     luckyStrike = false;

// Bowling assets
Sound    hitSound = {0};
Texture2D bowlingBg = {0};

// Falling balls globals
Ball balls[MAX_BALLS] = {0};
float basketX;
float basketY;
float basketWidth;
float basketHeight;
float basketSideHeight;
int basketSpeed = 7;
int ballScore = 0;
int misses = 0;
Texture2D ballsBg = {0};
Texture2D basketTex = {0};  // New texture for the basket PNG

// Wordle globals
const char* wordList[] = {
    "APPLE", "BREAD", "CHAIR", "DANCE", "EAGLE",
    "FLAME", "GRAPE", "HOUSE", "IMAGE", "JOKER"
};
const int NUM_WORDS = 10;
char targetWord[WORD_LENGTH + 1];
char guesses[MAX_GUESSES][WORD_LENGTH + 1];
Color guessColors[MAX_GUESSES][WORD_LENGTH];
int currentGuess = 0;
int currentLetter = 0;
bool wordleWon = false;

// Revive prompt globals
Shader blurShader = {0};
RenderTexture2D reviveTarget = {0};
int reviveChoice = 0; // 0 = YES, 1 = NO
int reviveTimer = 600; // 10 seconds at 60 FPS

// ------------ Helpers ------------
static bool ColorsEqual(Color c1, Color c2) {
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

static void ResetElixirState(void) {
    elixirAvailable = false;
    elixirReady = false;
    elixirSpawnTimer = 0.0f;
    elixirDurationTimer = 0.0f;
    elixirEffectActive = false;
    elixirEffectTimer = 0.0f;
}

static void ResetThunderstoneState(void) {
    thunderstoneAvailable = false;
    thunderstoneSpawnTimer = 0.0f;
    thunderstoneDurationTimer = 0.0f;
    thunderstoneEffectActive = false;
    thunderstoneEffectTimer = 0.0f;
}

static void ResetSpeedState(void) {
    speedAvailable = false;
    speedSpawnTimer = 0.0f;
    speedDurationTimer = 0.0f;
    speedEffectActive = false;
    speedEffectTimer = 0.0f;
    playerSpeed = basePlayerSpeed;
}

static void ResetPowerState(void) {
    powerAvailable = false;
    powerSpawnTimer = 0.0f;
    powerDurationTimer = 0.0f;
    powerEffectActive = false;
    powerEffectTimer = 0.0f;
}

static void ResetGame(Difficulty difficulty) {
    playerPos = (Vector2){400, 300};
    playerSpeed = basePlayerSpeed;
    score = 0;
    gameOver = false;
    deadSoundPlayed = false;
    gameTimer = 0.0f;
    for (int i = 0; i < MAX_ENEMIES;  i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS;  i++) bullets[i].active = false;
    for (int i = 0; i < MAX_OBSTACLES; i++) obstacles[i].active = false;
    ResetElixirState();
    ResetThunderstoneState();
    ResetSpeedState();
    ResetPowerState();
}

static void SpawnEnemy(Difficulty difficulty) {
    int screenWidth  = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            int side = GetRandomValue(0, 3);
            Vector2 pos;
            switch (side) {
                case 0: pos = (Vector2){0, GetRandomValue(0, screenHeight)}; break;
                case 1: pos = (Vector2){screenWidth, GetRandomValue(0, screenHeight)}; break;
                case 2: pos = (Vector2){GetRandomValue(0, screenWidth), 0}; break;
                default: pos = (Vector2){GetRandomValue(0, screenWidth), screenHeight}; break;
            }

            float baseSpeed = 50.0f;
            float speed = baseSpeed;
            if (difficulty == DIFFICULTY_MEDIUM) speed = baseSpeed * 1.7f;
            if (difficulty == DIFFICULTY_HARD)   speed = baseSpeed * 2.0f;

            int type = 0;
            if (gameTimer > 20.0f) {
                type = GetRandomValue(0, 2);
            } else if (gameTimer > 10.0f) {
                type = GetRandomValue(0, 1);
            } else {
                type = 0;
            }

            enemies[i].position = pos;
            enemies[i].speed = speed;
            enemies[i].velocity = Vector2Scale(Vector2Normalize(Vector2Subtract(playerPos, pos)), speed);
            enemies[i].active = true;
            enemies[i].type = type;
            enemies[i].health = (type == 0) ? 1 : (type == 1) ? 2 : 3;
            break;
        }
    }
}

static void SpawnObstacles(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            float w = (float)obstacleTex.width;
            float h = (float)obstacleTex.height;
            
            // Ensure obstacles don't spawn on player's starting position
            bool tooCloseToPlayer;
            Rectangle obsRect;
            do {
                float x = (float)GetRandomValue(100, GetScreenWidth()  - (int)w);
                float y = (float)GetRandomValue(100, GetScreenHeight() - (int)h);
                obsRect = (Rectangle){x, y, w, h};
                
                Rectangle playerSafeZone = {
                    playerPos.x - w - 50, 
                    playerPos.y - h - 50, 
                    w + 100, 
                    h + 100
                };
                tooCloseToPlayer = CheckCollisionRecs(obsRect, playerSafeZone);
            } while (tooCloseToPlayer);

            bool overlap;
            do {
                overlap = false;
                for (int j = 0; j < i; j++) {
                    if (obstacles[j].active && CheckCollisionRecs(obstacles[j].rect, obsRect)) {
                        overlap = true;
                        float x = (float)GetRandomValue(100, GetScreenWidth()  - (int)w);
                        float y = (float)GetRandomValue(100, GetScreenHeight() - (int)h);
                        obsRect = (Rectangle){x, y, w, h};
                        break;
                    }
                }
            } while (overlap);

            obstacles[i].rect = obsRect;
            obstacles[i].active = true;
        }
    }
}

static void ShootBullet(void) {
    if (thunderstoneEffectActive) {
        float speed = 400.0f;
        float diagSpeed = speed / sqrtf(2.0f);
        Vector2 directions[] = {
            {0, -speed},
            {speed, 0},
            {0, speed},
            {-speed, 0},
            {-diagSpeed, -diagSpeed},
            {diagSpeed, -diagSpeed},
            {-diagSpeed, speed},
            {diagSpeed, speed}
        };
        
        for (int d = 0; d < 8; d++) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].position = playerPos;
                    bullets[i].velocity = directions[d];
                    bullets[i].active = true;
                    break;
                }
            }
        }
    } else {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].position = playerPos;
                bullets[i].velocity = (Vector2){0, -400};
                bullets[i].active = true;
                break;
            }
        }
    }
}

static void LayoutPins(void) {
    const float cx = GetScreenWidth() / 2.0f;
    const float topY = 120.0f;
    const float spacing = 35.0f;

    int idx = 0;
    for (int row = 0; row < 4; row++) {
        for (int i = 0; i <= row; i++) {
            if (idx < NUM_PINS) {
                pins[idx].position.x = cx + (i - row / 2.0f) * spacing;
                pins[idx].position.y = topY + row * spacing;
                pins[idx].fallen = false;
                pins[idx].animating = false;
                pins[idx].velocity = (Vector2){0, 0};
                pins[idx].rotation = 0;
                idx++;
            }
        }
    }
}

static void ResetMiniGame(Difficulty difficulty) {
    if (difficulty == DIFFICULTY_EASY) {
        // Reset falling balls
        basketX = GetScreenWidth() / 2.0f - 75;
        basketY = GetScreenHeight() - 80;
        basketWidth = 150;
        basketHeight = 30;
        basketSideHeight = 50;
        ballScore = 0;
        misses = 0;
        for (int i = 0; i < MAX_BALLS; i++) {
            balls[i].position.x = (float)GetRandomValue(BALLS_RADIUS, GetScreenWidth() - BALLS_RADIUS);
            balls[i].position.y = (float)GetRandomValue(-600, 0);
            balls[i].color = (Color){ (unsigned char)GetRandomValue(50,255), (unsigned char)GetRandomValue(50,255), (unsigned char)GetRandomValue(50,255), 255 };
            balls[i].active = true;
        }
    } else if (difficulty == DIFFICULTY_MEDIUM) {
        // Reset wordle
        int wordIndex = GetRandomValue(0, NUM_WORDS - 1);
        strcpy(targetWord, wordList[wordIndex]);
        memset(guesses, 0, sizeof(guesses));
        memset(guessColors, 0, sizeof(guessColors));
        currentGuess = 0;
        currentLetter = 0;
        wordleWon = false;
    } else {
        // Reset bowling
        ballPos = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight() - 80.0f };
        t = 0.0f;
        throwAngle = 0.0f;
        ballLaunched = false;
        power = 0.0f;
        charging = false;
        strikeMode = false;
        luckyStrike = false;
        LayoutPins();
    }
}

// ------------ Main ------------
int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Capture or Escape");
    InitAudioDevice();
    SetTargetFPS(60);

    // --- Load assets ---
    if (!FileExists("resources/logo.png")) TraceLog(LOG_WARNING, "logo.png missing!");
    logo = LoadTexture("resources/logo.png");

    if (!FileExists("resources/home.png")) TraceLog(LOG_WARNING, "home.png missing!");
    homeBg = LoadTexture("resources/home.png");

    if (!FileExists("resources/emoji_font.ttf")) TraceLog(LOG_WARNING, "emoji_font.ttf missing! Using default.");
    emojiFont = LoadFont("resources/emoji_font.ttf");
    if (emojiFont.texture.id == 0) emojiFont = GetFontDefault();

    if (!FileExists("resources/pikachu.png")) TraceLog(LOG_WARNING, "pikachu.png missing!");
    pikachuTex = LoadTexture("resources/pikachu.png");

    if (!FileExists("resources/raichu.png")) TraceLog(LOG_WARNING, "raichu.png missing!");
    raichuTex = LoadTexture("resources/raichu.png");

    if (!FileExists("resources/pokeball.png")) TraceLog(LOG_WARNING, "pokeball.png missing!");
    pokeballTex = LoadTexture("resources/pokeball.png");

    if (!FileExists("resources/ultra_ball.png")) TraceLog(LOG_WARNING, "ultra_ball.png missing!");
    ultraBallTex = LoadTexture("resources/ultra_ball.png");

    if (!FileExists("resources/master_ball.png")) TraceLog(LOG_WARNING, "master_ball.png missing!");
    masterBallTex = LoadTexture("resources/master_ball.png");

    if (!FileExists("resources/balh.png")) TraceLog(LOG_WARNING, "balh.png missing!");
    balhTex = LoadTexture("resources/balh.png");

    if (!FileExists("resources/aoi.png")) TraceLog(LOG_WARNING, "aoi.png missing!");
    aoiTex = LoadTexture("resources/aoi.png");

    // Load difficulty backgrounds
    if (!FileExists("resources/easy.png")) TraceLog(LOG_WARNING, "easy.png missing!");
    easyBg = LoadTexture("resources/easy.png");
    
    if (!FileExists("resources/medium.png")) TraceLog(LOG_WARNING, "medium.png missing!");
    mediumBg = LoadTexture("resources/medium.png");
    
    if (!FileExists("resources/hard.png")) TraceLog(LOG_WARNING, "hard.png missing!");
    hardBg = LoadTexture("resources/hard.png");

    if (!FileExists("resources/Rock.png")) TraceLog(LOG_WARNING, "Rock.png missing!");
    Image rockImg = LoadImage("resources/Rock.png");
    if (rockImg.data != NULL) {
        ImageResize(&rockImg, rockImg.width / 3, rockImg.height / 3);
        obstacleTex = LoadTextureFromImage(rockImg);
        UnloadImage(rockImg);
    }

    if (!FileExists("resources/elixir.png")) TraceLog(LOG_WARNING, "elixir.png missing! A fallback circle will be drawn.");
    elixirTex = LoadTexture("resources/elixir.png");

    if (!FileExists("resources/thunderstone.png")) TraceLog(LOG_WARNING, "thunderstone.png missing! A fallback circle will be drawn.");
    thunderstoneTex = LoadTexture("resources/thunderstone.png");

    if (!FileExists("resources/speed.png")) TraceLog(LOG_WARNING, "speed.png missing! A fallback circle will be drawn.");
    speedTex = LoadTexture("resources/speed.png");

    if (!FileExists("resources/power.png")) TraceLog(LOG_WARNING, "power.png missing! A fallback circle will be drawn.");
    powerTex = LoadTexture("resources/power.png");

    if (!FileExists("resources/normal_lightning_bolt.png")) TraceLog(LOG_WARNING, "normal_lightning_bolt.png missing!");
    normalBulletTex = LoadTexture("resources/normal_lightning_bolt.png");

    if (!FileExists("resources/special_lightning_bolt.png")) TraceLog(LOG_WARNING, "special_lightning_bolt.png missing!");
    specialBulletTex = LoadTexture("resources/special_lightning_bolt.png");

    hitSound = LoadSound("resources/strike.wav");
    bowlingBg = LoadTexture("resources/background.png");

    if (!FileExists("resources/background3.png")) TraceLog(LOG_WARNING, "background3.png missing!");
    ballsBg = LoadTexture("resources/background3.png");

    if (!FileExists("resources/bg_fruit_bask.png")) TraceLog(LOG_WARNING, "bg_fruit_bask.png missing!");
    basketTex = LoadTexture("resources/bg_fruit_bask.png");
    
    // Load death sound
    if (!FileExists("resources/dead.mp3")) TraceLog(LOG_WARNING, "dead.mp3 missing!");
    deadSound = LoadSound("resources/dead.mp3");
    
    // Load background music
    if (!FileExists("resources/bgm.mp3")) TraceLog(LOG_WARNING, "bgm.mp3 missing!");
    bgm = LoadMusicStream("resources/bgm.mp3");
    if (bgm.ctxData != NULL) {
        PlayMusicStream(bgm);
        musicPlaying = true;
        SetMusicVolume(bgm, 0.9f);
    }

    // Initialize blur shader and render texture for revive prompt
    blurShader = LoadShaderFromMemory(0, blurShaderCode);
    int resolutionLoc = GetShaderLocation(blurShader, "resolution");
    Vector2 resolution = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(blurShader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
    reviveTarget = LoadRenderTexture(screenWidth, screenHeight);

    GameState gameState = OPENING_SCENE;
    Difficulty selectedDifficulty = DIFFICULTY_MEDIUM;
    float transitionTimer = 0.0f;

    float gameOverScale = 0.1f;
    float scaleSpeed = 1.5f;
    bool animationComplete = false;
    float enemySpawnTimer = 0.0f;

    ResetMiniGame(selectedDifficulty);
    ResetElixirState();
    ResetThunderstoneState();
    ResetSpeedState();
    ResetPowerState();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        if (musicPlaying) {
            UpdateMusicStream(bgm);
        }

        Rectangle easyBtn = { GetScreenWidth()/2 - 100, 250, 200, 50 };
        Rectangle mediumBtn = { GetScreenWidth()/2 - 100, 320, 200, 50 };
        Rectangle hardBtn = { GetScreenWidth()/2 - 100, 390, 200, 50 };
        Rectangle startBtn = { GetScreenWidth()/2 - 100, 460, 200, 50 };

        // ---------------- UPDATE ----------------
        switch (gameState) {
            case OPENING_SCENE: {
                if (CheckCollisionPointRec(GetMousePosition(), easyBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    selectedDifficulty = DIFFICULTY_EASY;
                if (CheckCollisionPointRec(GetMousePosition(), mediumBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    selectedDifficulty = DIFFICULTY_MEDIUM;
                if (CheckCollisionPointRec(GetMousePosition(), hardBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    selectedDifficulty = DIFFICULTY_HARD;
                if (CheckCollisionPointRec(GetMousePosition(), startBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ResetGame(selectedDifficulty);
                    if (selectedDifficulty == DIFFICULTY_HARD) SpawnObstacles();
                    elixirSpawnInterval = (selectedDifficulty == DIFFICULTY_EASY) ? 8.0f : 
                                         (selectedDifficulty == DIFFICULTY_MEDIUM) ? 5.0f : 7.0f;
                    thunderstoneSpawnInterval = (selectedDifficulty == DIFFICULTY_EASY) ? 8.0f : 
                                               (selectedDifficulty == DIFFICULTY_MEDIUM) ? 10.0f : 12.0f;
                    speedSpawnInterval = (selectedDifficulty == DIFFICULTY_EASY) ? 10.0f : 
                                         (selectedDifficulty == DIFFICULTY_MEDIUM) ? 12.0f : 15.0f;
                    powerSpawnInterval = (selectedDifficulty == DIFFICULTY_EASY) ? 12.0f : 
                                         (selectedDifficulty == DIFFICULTY_MEDIUM) ? 15.0f : 18.0f;
                    secondChanceUsed = false;
                    transitionTimer = 0.0f;
                    gameState = DIFFICULTY_TRANSITION;
                }
            } break;

            case DIFFICULTY_TRANSITION: {
                transitionTimer += dt;
                if (transitionTimer >= 2.5f) {
                    gameState = GAMEPLAY;
                }
            } break;

            case GAMEPLAY: {
                if (!gameOver) {
                    gameTimer += dt;
                }

                if (elixirEffectActive) {
                    elixirEffectTimer -= dt;
                    if (elixirEffectTimer <= 0.0f) {
                        elixirEffectActive = false;
                    }
                }
                
                if (thunderstoneEffectActive) {
                    thunderstoneEffectTimer -= dt;
                    if (thunderstoneEffectTimer <= 0.0f) {
                        thunderstoneEffectActive = false;
                    }
                }

                if (speedEffectActive) {
                    speedEffectTimer -= dt;
                    if (speedEffectTimer <= 0.0f) {
                        speedEffectActive = false;
                        playerSpeed = basePlayerSpeed;
                    }
                }

                if (powerEffectActive) {
                    powerEffectTimer -= dt;
                    if (powerEffectTimer <= 0.0f) {
                        powerEffectActive = false;
                    }
                }
                
                if (!gameOver) {
                    float delta_x = 0.0f;
                    if (IsKeyDown(KEY_LEFT))  delta_x -= playerSpeed * dt;
                    if (IsKeyDown(KEY_RIGHT)) delta_x += playerSpeed * dt;
                    float delta_y = 0.0f;
                    if (IsKeyDown(KEY_UP))    delta_y -= playerSpeed * dt;
                    if (IsKeyDown(KEY_DOWN))  delta_y += playerSpeed * dt;
                    playerPos.x += delta_x;
                    playerPos.y += delta_y;

                    if (IsKeyPressed(KEY_SPACE)) ShootBullet();

                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active) {
                            bullets[i].position.x += bullets[i].velocity.x * dt;
                            bullets[i].position.y += bullets[i].velocity.y * dt;
                            
                            if (bullets[i].position.x < 0 || bullets[i].position.x > GetScreenWidth() ||
                                bullets[i].position.y < 0 || bullets[i].position.y > GetScreenHeight()) {
                                bullets[i].active = false;
                            }
                        }
                    }

                    float spawnInterval = (selectedDifficulty == DIFFICULTY_EASY) ? 1.5f :
                                          (selectedDifficulty == DIFFICULTY_MEDIUM) ? 1.0f : 0.7f;
                    enemySpawnTimer += dt;
                    if (enemySpawnTimer > spawnInterval - (score * 0.01f)) {
                        SpawnEnemy(selectedDifficulty);
                        enemySpawnTimer = 0;
                    }

                    if (!elixirAvailable && !elixirReady && elixirSpawnInterval > 0.0f) {
                        elixirSpawnTimer += dt;
                        if (elixirSpawnTimer >= elixirSpawnInterval) {
                            elixirSpawnTimer = 0.0f;
                            float margin = 50.0f;
                            elixirPos.x = GetRandomValue((int)margin, GetScreenWidth() - (int)margin);
                            elixirPos.y = GetRandomValue((int)margin, GetScreenHeight() - (int)margin);
                            elixirAvailable = true;
                            elixirDurationTimer = 0.0f;
                        }
                    }

                    if (elixirAvailable) {
                        elixirDurationTimer += dt;
                        if (elixirDurationTimer >= ELIXIR_DURATION) {
                            elixirAvailable = false;
                            elixirDurationTimer = 0.0f;
                        } else {
                            float pickupRadius = 50.0f;
                            if (CheckCollisionCircles(playerPos, 20.0f, elixirPos, pickupRadius)) {
                                elixirAvailable = false;
                                elixirReady = true;
                                elixirDurationTimer = 0.0f;
                            }
                        }
                    }

                    if (elixirReady && IsKeyPressed(KEY_S)) {
                        int enemiesDestroyed = 0;
                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            if (enemies[i].active) {
                                enemies[i].active = false;
                                enemiesDestroyed++;
                            }
                        }
                        score += enemiesDestroyed;
                        elixirReady = false;
                        elixirEffectActive = true;
                        elixirEffectTimer = ELIXIR_EFFECT_DURATION;
                    }

                    if (!thunderstoneAvailable) {
                        thunderstoneSpawnTimer += dt;
                        if (thunderstoneSpawnTimer >= thunderstoneSpawnInterval) {
                            thunderstoneSpawnTimer = 0.0f;
                            float margin = 50.0f;
                            thunderstonePos.x = GetRandomValue((int)margin, GetScreenWidth() - (int)margin);
                            thunderstonePos.y = GetRandomValue((int)margin, GetScreenHeight() - (int)margin);
                            thunderstoneAvailable = true;
                            thunderstoneDurationTimer = 0.0f;
                        }
                    }

                    if (thunderstoneAvailable) {
                        thunderstoneDurationTimer += dt;
                        if (thunderstoneDurationTimer >= THUNDERSTONE_DURATION) {
                            thunderstoneAvailable = false;
                            thunderstoneDurationTimer = 0.0f;
                        } else {
                            float pickupRadius = 50.0f;
                            if (CheckCollisionCircles(playerPos, 20.0f, thunderstonePos, pickupRadius)) {
                                thunderstoneAvailable = false;
                                thunderstoneEffectActive = true;
                                thunderstoneEffectTimer = THUNDERSTONE_EFFECT_DURATION;
                            }
                        }
                    }

                    if (!speedAvailable) {
                        speedSpawnTimer += dt;
                        if (speedSpawnTimer >= speedSpawnInterval) {
                            speedSpawnTimer = 0.0f;
                            float margin = 50.0f;
                            speedPos.x = GetRandomValue((int)margin, GetScreenWidth() - (int)margin);
                            speedPos.y = GetRandomValue((int)margin, GetScreenHeight() - (int)margin);
                            speedAvailable = true;
                            speedDurationTimer = 0.0f;
                        }
                    }

                    if (speedAvailable) {
                        speedDurationTimer += dt;
                        if (speedDurationTimer >= SPEED_DURATION) {
                            speedAvailable = false;
                            speedDurationTimer = 0.0f;
                        } else {
                            float pickupRadius = 50.0f;
                            if (CheckCollisionCircles(playerPos, 20.0f, speedPos, pickupRadius)) {
                                speedAvailable = false;
                                speedEffectActive = true;
                                speedEffectTimer = SPEED_EFFECT_DURATION;
                                playerSpeed = basePlayerSpeed * 2.5f;
                            }
                        }
                    }

                    if (!powerAvailable) {
                        powerSpawnTimer += dt;
                        if (powerSpawnTimer >= powerSpawnInterval) {
                            powerSpawnTimer = 0.0f;
                            float margin = 50.0f;
                            powerPos.x = GetRandomValue((int)margin, GetScreenWidth() - (int)margin);
                            powerPos.y = GetRandomValue((int)margin, GetScreenHeight() - (int)margin);
                            powerAvailable = true;
                            powerDurationTimer = 0.0f;
                        }
                    }

                    if (powerAvailable) {
                        powerDurationTimer += dt;
                        if (powerDurationTimer >= POWER_DURATION) {
                            powerAvailable = false;
                            powerDurationTimer = 0.0f;
                        } else {
                            float pickupRadius = 50.0f;
                            if (CheckCollisionCircles(playerPos, 20.0f, powerPos, pickupRadius)) {
                                powerAvailable = false;
                                powerEffectActive = true;
                                powerEffectTimer = POWER_EFFECT_DURATION;
                            }
                        }
                    }

                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (enemies[i].active) {
                            Vector2 direction = Vector2Subtract(playerPos, enemies[i].position);
                            if (Vector2Length(direction) > 0.0f)
                                enemies[i].velocity = Vector2Scale(Vector2Normalize(direction), enemies[i].speed);
                            enemies[i].position = Vector2Add(enemies[i].position, Vector2Scale(enemies[i].velocity, dt));

                            if (CheckCollisionCircles(enemies[i].position, 20, playerPos, 20)) {
                                if (!secondChanceUsed) {
                                    gameState = REVIVE_PROMPT;
                                    reviveChoice = 0; // Default to YES
                                    reviveTimer = 600; // Reset timer (10 seconds at 60 FPS)
                                } else {
                                    gameOver = true;
                                    gameState = CLOSING_SCENE;
                                    if (!deadSoundPlayed && deadSound.frameCount > 0) {
                                        PlaySound(deadSound);
                                        deadSoundPlayed = true;
                                    }
                                }
                                break;
                            }

                            for (int j = 0; j < MAX_BULLETS; j++) {
                                if (bullets[j].active && CheckCollisionCircles(enemies[i].position, 20, bullets[j].position, 5)) {
                                    bullets[j].active = false;
                                    if (powerEffectActive) {
                                        enemies[i].health = 0;
                                    } else {
                                        enemies[i].health--;
                                    }
                                    if (enemies[i].health <= 0) {
                                        enemies[i].active = false;
                                        score++;
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    if (elixirEffectActive) {
                        float progress = 1.0f - (elixirEffectTimer / ELIXIR_EFFECT_DURATION);
                        float radius = 10.0f + progress * 100.0f;
                        int enemiesDestroyed = 0;
                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            if (enemies[i].active && CheckCollisionCircles(playerPos, radius, enemies[i].position, 20)) {
                                enemies[i].active = false;
                                enemiesDestroyed++;
                            }
                        }
                        score += enemiesDestroyed;
                    }

                    if (selectedDifficulty == DIFFICULTY_HARD) {
                        Rectangle playerRect = {
                            playerPos.x - pikachuTex.width/2.0f,
                            playerPos.y - pikachuTex.height/2.0f,
                            (float)pikachuTex.width,
                            (float)pikachuTex.height
                        };
                        for (int i = 0; i < MAX_OBSTACLES; i++) {
                            if (obstacles[i].active && CheckCollisionRecs(playerRect, obstacles[i].rect)) {
                                if (!secondChanceUsed) {
                                    gameState = REVIVE_PROMPT;
                                    reviveChoice = 0; // Default to YES
                                    reviveTimer = 600; // Reset timer (10 seconds at 60 FPS)
                                } else {
                                    gameOver = true;
                                    gameState = CLOSING_SCENE;
                                    if (!deadSoundPlayed && deadSound.frameCount > 0) {
                                        PlaySound(deadSound);
                                        deadSoundPlayed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            } break;

            case REVIVE_PROMPT: {
                // Update choice
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT)) {
                    reviveChoice = !reviveChoice;
                }
                // Confirm choice
                if (IsKeyPressed(KEY_ENTER)) {
                    if (reviveChoice == 0) { // YES
                        ResetMiniGame(selectedDifficulty);
                        ResetElixirState();
                        ResetThunderstoneState();
                        ResetSpeedState();
                        ResetPowerState();
                        gameState = MINI_GAME;
                    } else { // NO
                        gameOver = true;
                        secondChanceUsed = true;
                        gameState = CLOSING_SCENE;
                        if (!deadSoundPlayed && deadSound.frameCount > 0) {
                            PlaySound(deadSound);
                            deadSoundPlayed = true;
                        }
                    }
                }
                // Countdown timer
                reviveTimer--;
                if (reviveTimer <= 0) {
                    gameOver = true;
                    secondChanceUsed = true;
                    gameState = CLOSING_SCENE;
                    if (!deadSoundPlayed && deadSound.frameCount > 0) {
                        PlaySound(deadSound);
                        deadSoundPlayed = true;
                    }
                }
            } break;

            case MINI_GAME: {
                if (selectedDifficulty == DIFFICULTY_EASY) {
                    // Falling balls update
                    if (IsKeyDown(KEY_LEFT) && basketX > 0) basketX -= basketSpeed;
                    if (IsKeyDown(KEY_RIGHT) && basketX + basketWidth < GetScreenWidth()) basketX += basketSpeed;

                    int activeBalls = 0;
                    for (int i = 0; i < MAX_BALLS; i++) {
                        if (balls[i].active) {
                            balls[i].position.y += BALL_SPEED;

                            if (balls[i].position.y + BALLS_RADIUS >= basketY &&
                                balls[i].position.x >= basketX &&
                                balls[i].position.x <= basketX + basketWidth) {
                                ballScore++;
                                balls[i].active = false;
                            }

                            if (balls[i].position.y - BALLS_RADIUS > GetScreenHeight()) {
                                misses++;
                                balls[i].position.y = (float)GetRandomValue(-600, 0);
                                balls[i].position.x = (float)GetRandomValue(BALLS_RADIUS, GetScreenWidth() - BALLS_RADIUS);
                                balls[i].color = (Color){ (unsigned char)GetRandomValue(50,255), (unsigned char)GetRandomValue(50,255), (unsigned char)GetRandomValue(50,255), 255 };
                                balls[i].active = true;
                            }
                            activeBalls++;
                        }
                    }

                    if (activeBalls < 2) {
                        for (int i = 0; i < MAX_BALLS; i++) {
                            if (!balls[i].active) {
                                balls[i].active = true;
                                balls[i].position = (Vector2){ (float)GetRandomValue(BALLS_RADIUS, GetScreenWidth() - BALLS_RADIUS),
                                                               (float)GetRandomValue(-600, 0) };
                                balls[i].color = (Color){ (unsigned char)GetRandomValue(50,255), (unsigned char)GetRandomValue(50,255), (unsigned char)GetRandomValue(50,255), 255 };
                                activeBalls++;
                                if (activeBalls >= 2) break;
                            }
                        }
                    }

                    if (ballScore >= TARGET_SCORE) {
                        secondChanceUsed = true;
                        playerPos = (Vector2){400, 300};
                        gameOver = false;
                        for (int i = 0; i < MAX_ENEMIES;  i++) enemies[i].active = false;
                        for (int i = 0; i < MAX_BULLETS;  i++) bullets[i].active = false;
                        ResetElixirState();
                        ResetThunderstoneState();
                        ResetSpeedState();
                        ResetPowerState();
                        gameState = GAMEPLAY;
                    } else if (misses >= MAX_MISSES) {
                        gameOver = true;
                        secondChanceUsed = true;
                        gameState = CLOSING_SCENE;
                        if (!deadSoundPlayed && deadSound.frameCount > 0) {
                            PlaySound(deadSound);
                            deadSoundPlayed = true;
                        }
                    }
                } else if (selectedDifficulty == DIFFICULTY_MEDIUM) {
                    // Wordle update
                    if (!wordleWon && currentGuess < MAX_GUESSES) {
                        int key = GetCharPressed();
                        while (key > 0) {
                            if (key >= 'A' && key <= 'Z' && currentLetter < WORD_LENGTH) {
                                guesses[currentGuess][currentLetter] = (char)key;
                                currentLetter++;
                            }
                            key = GetCharPressed();
                        }

                        if (IsKeyPressed(KEY_BACKSPACE) && currentLetter > 0) {
                            currentLetter--;
                            guesses[currentGuess][currentLetter] = '\0';
                        }

                        if (IsKeyPressed(KEY_ENTER) && currentLetter == WORD_LENGTH) {
                            // Compute feedback
                            int letterCount[26] = {0};
                            for (int i = 0; i < WORD_LENGTH; i++) {
                                letterCount[targetWord[i] - 'A']++;
                            }

                            // Mark greens
                            for (int i = 0; i < WORD_LENGTH; i++) {
                                if (guesses[currentGuess][i] == targetWord[i]) {
                                    guessColors[currentGuess][i] = GREEN;
                                    letterCount[guesses[currentGuess][i] - 'A']--;
                                } else {
                                    guessColors[currentGuess][i] = LIGHTGRAY;
                                }
                            }

                            // Mark yellows
                            for (int i = 0; i < WORD_LENGTH; i++) {
                                if (ColorsEqual(guessColors[currentGuess][i], LIGHTGRAY)) {
                                    char letter = guesses[currentGuess][i];
                                    if (letterCount[letter - 'A'] > 0) {
                                        guessColors[currentGuess][i] = YELLOW;
                                        letterCount[letter - 'A']--;
                                    }
                                }
                            }

                            // Check if won
                            wordleWon = true;
                            for (int i = 0; i < WORD_LENGTH; i++) {
                                if (!ColorsEqual(guessColors[currentGuess][i], GREEN)) {
                                    wordleWon = false;
                                    break;
                                }
                            }

                            currentGuess++;
                            currentLetter = 0;
                        }
                    }

                    if (wordleWon || (currentGuess == MAX_GUESSES)) {
                        if (wordleWon) {
                            secondChanceUsed = true;
                            playerPos = (Vector2){400, 300};
                            gameOver = false;
                            for (int i = 0; i < MAX_ENEMIES;  i++) enemies[i].active = false;
                            for (int i = 0; i < MAX_BULLETS;  i++) bullets[i].active = false;
                            ResetElixirState();
                            ResetThunderstoneState();
                            ResetSpeedState();
                            ResetPowerState();
                            gameState = GAMEPLAY;
                        } else {
                            gameOver = true;
                            secondChanceUsed = true;
                            gameState = CLOSING_SCENE;
                            if (!deadSoundPlayed && deadSound.frameCount > 0) {
                                PlaySound(deadSound);
                                deadSoundPlayed = true;
                            }
                        }
                    }
                } else {
                    // Bowling update
                    if (IsKeyPressed(KEY_S) && !ballLaunched) {
                        strikeMode = !strikeMode;
                        if (strikeMode) {
                            luckyStrike = (GetRandomValue(0, 1) == 1);
                        }
                    }

                    if (!ballLaunched) {
                        if (IsKeyDown(KEY_LEFT)) throwAngle -= 0.02f;
                        if (IsKeyDown(KEY_RIGHT)) throwAngle += 0.02f;
                        throwAngle = Clamp(throwAngle, -maxAngle, maxAngle);
                        ballPos.x = GetScreenWidth()/2.0f + sinf(throwAngle) * a;
                        ballPos.y = GetScreenHeight() - 80.0f;
                    }

                    if (IsKeyDown(KEY_SPACE) && !ballLaunched) {
                        charging = true;
                        power += 0.01f;
                        power = Clamp(power, 0.0f, maxPower);
                    }
                    if (IsKeyReleased(KEY_SPACE) && charging) {
                        charging = false;
                        ellipseCenter = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight() + 50 };
                        ballLaunched = true;
                        t = 0.0f;
                        ballSpeed = baseSpeed + power * 0.05f;
                    }

                    if (ballLaunched) {
                        t += ballSpeed;
                        float x = a * cosf(t);
                        float y = b * sinf(t);
                        ballPos.x = ellipseCenter.x + x * cosf(throwAngle) - y * sinf(throwAngle);
                        ballPos.y = ellipseCenter.y - x * sinf(throwAngle) - y * cosf(throwAngle);

                        if (ballPos.x < LANE_LEFT + BOWLING_BALL_RADIUS) ballPos.x = LANE_LEFT + BOWLING_BALL_RADIUS;
                        if (ballPos.x > LANE_RIGHT - BOWLING_BALL_RADIUS) ballPos.x = LANE_RIGHT - BOWLING_BALL_RADIUS;

                        for (int i = 0; i < NUM_PINS; i++) {
                            if (!pins[i].fallen && CheckCollisionCircles(ballPos, BOWLING_BALL_RADIUS, pins[i].position, PIN_RADIUS)) {
                                bool isStrikeCondition = (strikeMode && luckyStrike) || (fabsf(throwAngle) < 0.1f && power > 0.8f);
                                if (isStrikeCondition) {
                                    for (int j = 0; j < NUM_PINS; j++) {
                                        pins[j].fallen = true;
                                        pins[j].animating = true;
                                        pins[j].velocity = (Vector2){(float)GetRandomValue(-5, 5), (float)GetRandomValue(5, 10)};
                                        pins[j].rotation = (float)GetRandomValue(0, 360);
                                    }
                                    if (hitSound.frameCount > 0) PlaySound(hitSound);
                                    break;
                                } else {
                                    pins[i].fallen = true;
                                    pins[i].animating = true;
                                    pins[i].velocity = (Vector2){(float)GetRandomValue(-5, 5), (float)GetRandomValue(5, 10)};
                                    pins[i].rotation = (float)GetRandomValue(0, 360);
                                    if (hitSound.frameCount > 0) PlaySound(hitSound);
                                }
                            }
                        }

                        if (t >= PI / 2) {
                            bool strike = true;
                            for (int i = 0; i < NUM_PINS; i++) {
                                if (!pins[i].fallen) { strike = false; break; }
                            }
                            if (strike) {
                                secondChanceUsed = true;
                                playerPos = (Vector2){400, 300};
                                gameOver = false;
                                for (int i = 0; i < MAX_ENEMIES;  i++) enemies[i].active = false;
                                for (int i = 0; i < MAX_BULLETS;  i++) bullets[i].active = false;
                                ResetElixirState();
                                ResetThunderstoneState();
                                ResetSpeedState();
                                ResetPowerState();
                                gameState = GAMEPLAY;
                            } else {
                                gameOver = true;
                                secondChanceUsed = true;
                                gameState = CLOSING_SCENE;
                                if (!deadSoundPlayed && deadSound.frameCount > 0) {
                                    PlaySound(deadSound);
                                    deadSoundPlayed = true;
                                }
                            }
                            ResetMiniGame(selectedDifficulty);
                        }
                    }

                    for (int i = 0; i < NUM_PINS; i++) {
                        if (pins[i].animating) {
                            pins[i].position.x += pins[i].velocity.x;
                            pins[i].position.y += pins[i].velocity.y;
                            pins[i].velocity.y += 0.3f;
                            pins[i].rotation += 10.0f;
                            if (pins[i].position.y > GetScreenHeight() + 50.0f) {
                                pins[i].animating = false;
                            }
                        }
                    }
                }
            } break;

            case CLOSING_SCENE: {
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
                        if (selectedDifficulty == DIFFICULTY_HARD) SpawnObstacles();
                        secondChanceUsed = false;
                        gameState = GAMEPLAY;
                    }
                    if (IsKeyPressed(KEY_H)) {
                        ResetGame(selectedDifficulty);
                        secondChanceUsed = false;
                        gameState = OPENING_SCENE;
                    }
                }
            } break;
        }

        // ---------------- DRAW ----------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (gameState) {
            case OPENING_SCENE: {
                if (homeBg.id != 0) {
                    DrawTexturePro(
                        homeBg,
                        (Rectangle){0, 0, (float)homeBg.width, (float)homeBg.height},
                        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                        (Vector2){0, 0}, 0.0f, WHITE
                    );
                } else {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), DARKGRAY);
                    DrawText("home.png missing!", GetScreenWidth()/2 - 100, GetScreenHeight()/2, 20, RED);
                }

                if (logo.id != 0) {
                    float logoScale = 0.3f;
                    float logoWidth = logo.width * logoScale;
                    float logoHeight = logo.height * logoScale;
                    float logoX = GetScreenWidth()/2 - logoWidth/2;
                    float logoY = 50;
                    DrawTextureEx(logo, (Vector2){logoX, logoY}, 0.0f, logoScale, WHITE);
                }

                DrawTextEx(emojiFont, "Select Game Difficulty", (Vector2){GetScreenWidth()/2 - 160, 200}, 30, 2, WHITE);

                DrawRectangleRec(easyBtn, selectedDifficulty == DIFFICULTY_EASY ? LIME : LIGHTGRAY);
                Vector2 textSize = MeasureTextEx(emojiFont, "Easy", 20, 2);
                DrawTextEx(emojiFont, "Easy", 
                          (Vector2){easyBtn.x + (easyBtn.width - textSize.x)/2, easyBtn.y + (easyBtn.height - textSize.y)/2}, 
                          20, 2, DARKGRAY);

                DrawRectangleRec(mediumBtn, selectedDifficulty == DIFFICULTY_MEDIUM ? LIME : LIGHTGRAY);
                textSize = MeasureTextEx(emojiFont, "Medium", 20, 2);
                DrawTextEx(emojiFont, "Medium", 
                          (Vector2){mediumBtn.x + (mediumBtn.width - textSize.x)/2, mediumBtn.y + (mediumBtn.height - textSize.y)/2}, 
                          20, 2, DARKGRAY);

                DrawRectangleRec(hardBtn, selectedDifficulty == DIFFICULTY_HARD ? LIME : LIGHTGRAY);
                textSize = MeasureTextEx(emojiFont, "Hard", 20, 2);
                DrawTextEx(emojiFont, "Hard", 
                          (Vector2){hardBtn.x + (hardBtn.width - textSize.x)/2, hardBtn.y + (hardBtn.height - textSize.y)/2}, 
                          20, 2, DARKGRAY);

                DrawRectangleRec(startBtn, SKYBLUE);
                textSize = MeasureTextEx(emojiFont, "Start", 30, 2);
                DrawTextEx(emojiFont, "Start", 
                          (Vector2){startBtn.x + (startBtn.width - textSize.x)/2, startBtn.y + (startBtn.height - textSize.y)/2}, 
                          30, 2, DARKBLUE);
            } break;

            case DIFFICULTY_TRANSITION: {
                if (aoiTex.id != 0) {
                    float scale = fminf((float)GetScreenWidth() / aoiTex.width, (float)GetScreenHeight() / aoiTex.height);
                    float scaledWidth = aoiTex.width * scale;
                    float scaledHeight = aoiTex.height * scale;
                    float x = (GetScreenWidth() - scaledWidth) / 2.0f;
                    float y = (GetScreenHeight() - scaledHeight) / 2.0f;
                    DrawTexturePro(
                        aoiTex,
                        (Rectangle){0, 0, (float)aoiTex.width, (float)aoiTex.height},
                        (Rectangle){x, y, scaledWidth, scaledHeight},
                        (Vector2){0, 0}, 0.0f, WHITE
                    );
                } else {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), DARKGRAY);
                    DrawText("aoi.png missing!", GetScreenWidth()/2 - 100, GetScreenHeight()/2, 20, RED);
                }
            } break;

            case GAMEPLAY: {
                Texture2D currentBg;
                switch(selectedDifficulty) {
                    case DIFFICULTY_EASY: currentBg = easyBg; break;
                    case DIFFICULTY_MEDIUM: currentBg = mediumBg; break;
                    case DIFFICULTY_HARD: currentBg = hardBg; break;
                }
                
                if (currentBg.id != 0) {
                    DrawTexturePro(
                        currentBg,
                        (Rectangle){0, 0, (float)currentBg.width, (float)currentBg.height},
                        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                        (Vector2){0, 0}, 0.0f, WHITE
                    );
                } else {
                    ClearBackground(selectedDifficulty == DIFFICULTY_EASY ? GREEN : 
                                   (selectedDifficulty == DIFFICULTY_MEDIUM ? BLUE : RED));
                }

                if (elixirEffectActive) {
                    float progress = 1.0f - (elixirEffectTimer / ELIXIR_EFFECT_DURATION);
                    float radius = 10.0f + progress * 100.0f;
                    int alpha = (int)(255 * (1.0f - progress));
                    Color color = Fade(YELLOW, alpha/255.0f);
                    
                    for (int i = 0; i < 30; i++) {
                        float offset = i * 20.0f * progress;
                        DrawCircleLines((int)playerPos.x, (int)playerPos.y, radius - offset, color);
                    }
                    
                    DrawTextEx(emojiFont, "ELIXIR ACTIVATED!", 
                              (Vector2){GetScreenWidth()/2 - 120, GetScreenHeight()/2 - 30}, 
                              30, 2, Fade(YELLOW, alpha/255.0f));
                }

                if (thunderstoneEffectActive) {
                    float progress = 1.0f - (thunderstoneEffectTimer / THUNDERSTONE_EFFECT_DURATION);
                    float radius = 10.0f + progress * 100.0f;
                    int alpha = (int)(255 * (1.0f - progress));
                    Color color = Fade(ORANGE, alpha/255.0f);
                    
                    for (int i = 0; i < 3; i++) {
                        float offset = i * 20.0f * progress;
                        DrawCircleLines((int)playerPos.x, (int)playerPos.y, radius - offset, color);
                    }
                    
                    DrawTextEx(emojiFont, "RAICHU EVOLVED!", 
                              (Vector2){GetScreenWidth()/2 - 100, GetScreenHeight()/2 - 30}, 
                              30, 2, Fade(ORANGE, alpha/255.0f));
                }

                if (thunderstoneEffectActive && raichuTex.id != 0) {
                    Rectangle srcRec = { 0, 0, (float)raichuTex.width, (float)raichuTex.height };
                    Rectangle destRec = { playerPos.x, playerPos.y, (float)pikachuTex.width, (float)pikachuTex.height };
                    Vector2 origin = { (float)pikachuTex.width/2, (float)pikachuTex.height/2 };
                    DrawTexturePro(raichuTex, srcRec, destRec, origin, 0.0f, WHITE);
                } else if (pikachuTex.id != 0) {
                    DrawTexture(pikachuTex, playerPos.x - pikachuTex.width/2, playerPos.y - pikachuTex.height/2, WHITE);
                }

                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (bullets[i].active) {
                        Texture2D bulletTex = powerEffectActive ? specialBulletTex : normalBulletTex;
                        if (bulletTex.id != 0) {
                            float rotation = atan2f(bullets[i].velocity.y, bullets[i].velocity.x) * RAD2DEG;
                            DrawTextureEx(bulletTex, (Vector2){bullets[i].position.x - bulletTex.width / 2.0f, bullets[i].position.y - bulletTex.height / 2.0f}, rotation, 1.0f, WHITE);
                        } else {
                            DrawCircleV(bullets[i].position, 5, WHITE);
                        }
                    }
                }

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        Texture2D enemyTex = (enemies[i].type == 0) ? pokeballTex : (enemies[i].type == 1) ? ultraBallTex : masterBallTex;
                        if (enemyTex.id != 0) {
                            DrawTexture(enemyTex, enemies[i].position.x - enemyTex.width/2, enemies[i].position.y - enemyTex.height/2, WHITE);
                        }
                    }
                }

                if (elixirAvailable) {
                    if (elixirTex.id != 0) {
                        float elixirWidth = 100.0f;
                        float elixirHeight = elixirWidth * (float)elixirTex.height / (float)elixirTex.width;
                        DrawTexturePro(
                            elixirTex,
                            (Rectangle){0, 0, (float)elixirTex.width, (float)elixirTex.height},
                            (Rectangle){elixirPos.x, elixirPos.y, elixirWidth, elixirHeight},
                            (Vector2){elixirWidth/2, elixirHeight/2}, 0.0f, WHITE
                        );
                    } else {
                        DrawCircleV(elixirPos, 50.0f, PURPLE);
                        DrawText("E", (int)elixirPos.x - 20, (int)elixirPos.y - 24, 40, WHITE);
                    }
                }

                if (thunderstoneAvailable) {
                    if (thunderstoneTex.id != 0) {
                        float thunderstoneWidth = 100.0f;
                        float thunderstoneHeight = thunderstoneWidth * (float)thunderstoneTex.height / (float)thunderstoneTex.width;
                        DrawTexturePro(
                            thunderstoneTex,
                            (Rectangle){0, 0, (float)thunderstoneTex.width, (float)thunderstoneTex.height},
                            (Rectangle){thunderstonePos.x, thunderstonePos.y, thunderstoneWidth, thunderstoneHeight},
                            (Vector2){thunderstoneWidth/2, thunderstoneHeight/2}, 0.0f, WHITE
                        );
                    } else {
                        DrawCircleV(thunderstonePos, 50.0f, ORANGE);
                        DrawText("T", (int)thunderstonePos.x - 20, (int)thunderstonePos.y - 24, 40, WHITE);
                    }
                }

                if (speedAvailable) {
                    if (speedTex.id != 0) {
                        float speedWidth = 100.0f;
                        float speedHeight = speedWidth * (float)speedTex.height / (float)speedTex.width;
                        DrawTexturePro(
                            speedTex,
                            (Rectangle){0, 0, (float)speedTex.width, (float)speedTex.height},
                            (Rectangle){speedPos.x, speedPos.y, speedWidth, speedHeight},
                            (Vector2){speedWidth/2, speedHeight/2}, 0.0f, WHITE
                        );
                    } else {
                        DrawCircleV(speedPos, 50.0f, BLUE);
                        DrawText("S", (int)speedPos.x - 20, (int)speedPos.y - 24, 40, WHITE);
                    }
                }

                if (powerAvailable) {
                    if (powerTex.id != 0) {
                        float powerWidth = 100.0f;
                        float powerHeight = powerWidth * (float)powerTex.height / (float)powerTex.width;
                        DrawTexturePro(
                            powerTex,
                            (Rectangle){0, 0, (float)powerTex.width, (float)powerTex.height},
                            (Rectangle){powerPos.x, powerPos.y, powerWidth, powerHeight},
                            (Vector2){powerWidth/2, powerHeight/2}, 0.0f, WHITE
                        );
                    } else {
                        DrawCircleV(powerPos, 50.0f, RED);
                        DrawText("P", (int)powerPos.x - 20, (int)powerPos.y - 24, 40, WHITE);
                    }
                }

                if (elixirReady) {
                    DrawText("Elixir READY! Press S to clear enemies!", 20, 50, 18, WHITE);
                }

                if (selectedDifficulty == DIFFICULTY_HARD) {
                    for (int i = 0; i < MAX_OBSTACLES; i++) {
                        if (obstacles[i].active && obstacleTex.id != 0) {
                            Rectangle src = {0, 0, (float)obstacleTex.width, (float)obstacleTex.height};
                            Rectangle dst = obstacles[i].rect;
                            DrawTexturePro(obstacleTex, src, dst, (Vector2){0,0}, 0.0f, WHITE);
                        }
                    }
                }

                if (thunderstoneEffectActive) {
                    DrawText(TextFormat("RAICHU FORM: %.1f seconds left", thunderstoneEffectTimer), 
                            20, 80, 18, ORANGE);
                }

                if (speedEffectActive) {
                    DrawText(TextFormat("SPEED BOOST: %.1f seconds left", speedEffectTimer), 
                            20, 110, 18, BLUE);
                }

                if (powerEffectActive) {
                    DrawText(TextFormat("POWER BOOST: %.1f seconds left", powerEffectTimer), 
                            20, 140, 18, RED);
                }

                DrawTextEx(emojiFont, TextFormat("Score: %d", score), (Vector2){20, 20}, 20, 2, WHITE);
            } break;

            case REVIVE_PROMPT: {
                // Draw game state into render texture
                BeginTextureMode(reviveTarget);
                    ClearBackground(RAYWHITE);
                    Texture2D currentBg;
                    switch(selectedDifficulty) {
                        case DIFFICULTY_EASY: currentBg = easyBg; break;
                        case DIFFICULTY_MEDIUM: currentBg = mediumBg; break;
                        case DIFFICULTY_HARD: currentBg = hardBg; break;
                    }
                    if (currentBg.id != 0) {
                        DrawTexturePro(
                            currentBg,
                            (Rectangle){0, 0, (float)currentBg.width, (float)currentBg.height},
                            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                            (Vector2){0, 0}, 0.0f, WHITE
                        );
                    } else {
                        ClearBackground(selectedDifficulty == DIFFICULTY_EASY ? GREEN : 
                                       (selectedDifficulty == DIFFICULTY_MEDIUM ? BLUE : RED));
                    }
                    if (thunderstoneEffectActive && raichuTex.id != 0) {
                        Rectangle srcRec = { 0, 0, (float)raichuTex.width, (float)raichuTex.height };
                        Rectangle destRec = { playerPos.x, playerPos.y, (float)pikachuTex.width, (float)pikachuTex.height };
                        Vector2 origin = { (float)pikachuTex.width/2, (float)pikachuTex.height/2 };
                        DrawTexturePro(raichuTex, srcRec, destRec, origin, 0.0f, WHITE);
                    } else if (pikachuTex.id != 0) {
                        DrawTexture(pikachuTex, playerPos.x - pikachuTex.width/2, playerPos.y - pikachuTex.height/2, WHITE);
                    }
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active) {
                            Texture2D bulletTex = powerEffectActive ? specialBulletTex : normalBulletTex;
                            if (bulletTex.id != 0) {
                                float rotation = atan2f(bullets[i].velocity.y, bullets[i].velocity.x) * RAD2DEG;
                                DrawTextureEx(bulletTex, (Vector2){bullets[i].position.x - bulletTex.width / 2.0f, bullets[i].position.y - bulletTex.height / 2.0f}, rotation, 1.0f, WHITE);
                            } else {
                                DrawCircleV(bullets[i].position, 5, WHITE);
                            }
                        }
                    }
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (enemies[i].active) {
                            Texture2D enemyTex = (enemies[i].type == 0) ? pokeballTex : (enemies[i].type == 1) ? ultraBallTex : masterBallTex;
                            if (enemyTex.id != 0) {
                                DrawTexture(enemyTex, enemies[i].position.x - enemyTex.width/2, enemies[i].position.y - enemyTex.height/2, WHITE);
                            }
                        }
                    }
                    if (elixirAvailable) {
                        if (elixirTex.id != 0) {
                            float elixirWidth = 100.0f;
                            float elixirHeight = elixirWidth * (float)elixirTex.height / (float)elixirTex.width;
                            DrawTexturePro(
                                elixirTex,
                                (Rectangle){0, 0, (float)elixirTex.width, (float)elixirTex.height},
                                (Rectangle){elixirPos.x, elixirPos.y, elixirWidth, elixirHeight},
                                (Vector2){elixirWidth/2, elixirHeight/2}, 0.0f, WHITE
                            );
                        } else {
                            DrawCircleV(elixirPos, 50.0f, PURPLE);
                            DrawText("E", (int)elixirPos.x - 20, (int)elixirPos.y - 24, 40, WHITE);
                        }
                    }
                    if (thunderstoneAvailable) {
                        if (thunderstoneTex.id != 0) {
                            float thunderstoneWidth = 100.0f;
                            float thunderstoneHeight = thunderstoneWidth * (float)thunderstoneTex.height / (float)thunderstoneTex.width;
                            DrawTexturePro(
                                thunderstoneTex,
                                (Rectangle){0, 0, (float)thunderstoneTex.width, (float)thunderstoneTex.height},
                                (Rectangle){thunderstonePos.x, thunderstonePos.y, thunderstoneWidth, thunderstoneHeight},
                                (Vector2){thunderstoneWidth/2, thunderstoneHeight/2}, 0.0f, WHITE
                            );
                        } else {
                            DrawCircleV(thunderstonePos, 50.0f, ORANGE);
                            DrawText("T", (int)thunderstonePos.x - 20, (int)thunderstonePos.y - 24, 40, WHITE);
                        }
                    }
                    if (speedAvailable) {
                        if (speedTex.id != 0) {
                            float speedWidth = 100.0f;
                            float speedHeight = speedWidth * (float)speedTex.height / (float)speedTex.width;
                            DrawTexturePro(
                                speedTex,
                                (Rectangle){0, 0, (float)speedTex.width, (float)speedTex.height},
                                (Rectangle){speedPos.x, speedPos.y, speedWidth, speedHeight},
                                (Vector2){speedWidth/2, speedHeight/2}, 0.0f, WHITE
                            );
                        } else {
                            DrawCircleV(speedPos, 50.0f, BLUE);
                            DrawText("S", (int)speedPos.x - 20, (int)speedPos.y - 24, 40, WHITE);
                        }
                    }
                    if (powerAvailable) {
                        if (powerTex.id != 0) {
                            float powerWidth = 100.0f;
                            float powerHeight = powerWidth * (float)powerTex.height / (float)powerTex.width;
                            DrawTexturePro(
                                powerTex,
                                (Rectangle){0, 0, (float)powerTex.width, (float)powerTex.height},
                                (Rectangle){powerPos.x, powerPos.y, powerWidth, powerHeight},
                                (Vector2){powerWidth/2, powerHeight/2}, 0.0f, WHITE
                            );
                        } else {
                            DrawCircleV(powerPos, 50.0f, RED);
                            DrawText("P", (int)powerPos.x - 20, (int)powerPos.y - 24, 40, WHITE);
                        }
                    }
                    if (elixirReady) {
                        DrawText("Elixir READY! Press S to clear enemies!", 20, 50, 18, WHITE);
                    }
                    if (selectedDifficulty == DIFFICULTY_HARD) {
                        for (int i = 0; i < MAX_OBSTACLES; i++) {
                            if (obstacles[i].active && obstacleTex.id != 0) {
                                Rectangle src = {0, 0, (float)obstacleTex.width, (float)obstacleTex.height};
                                Rectangle dst = obstacles[i].rect;
                                DrawTexturePro(obstacleTex, src, dst, (Vector2){0,0}, 0.0f, WHITE);
                            }
                        }
                    }
                    if (thunderstoneEffectActive) {
                        DrawText(TextFormat("RAICHU FORM: %.1f seconds left", thunderstoneEffectTimer), 
                                20, 80, 18, ORANGE);
                    }
                    if (speedEffectActive) {
                        DrawText(TextFormat("SPEED BOOST: %.1f seconds left", speedEffectTimer), 
                                20, 110, 18, BLUE);
                    }
                    if (powerEffectActive) {
                        DrawText(TextFormat("POWER BOOST: %.1f seconds left", powerEffectTimer), 
                                20, 140, 18, RED);
                    }
                    DrawTextEx(emojiFont, TextFormat("Score: %d", score), (Vector2){20, 20}, 20, 2, WHITE);
                EndTextureMode();

                // Draw to screen
                ClearBackground(BLACK);
                // Apply blur shader
                BeginShaderMode(blurShader);
                    DrawTextureRec(reviveTarget.texture, 
                                  (Rectangle){0, 0, (float)reviveTarget.texture.width, -(float)reviveTarget.texture.height}, 
                                  (Vector2){0, 0}, WHITE);
                EndShaderMode();
                // Dim overlay
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
                // Title
                Vector2 titleSize = MeasureTextEx(emojiFont, "Continue?", 60, 2);
                DrawTextEx(emojiFont, "Continue?", 
                          (Vector2){GetScreenWidth()/2 - titleSize.x/2, 120}, 
                          60, 2, YELLOW);
                // YES/NO with highlight boxes
                if (reviveChoice == 0) {
                    DrawRectangleRounded((Rectangle){280, 280, 200, 70}, 0.3f, 10, Fade(GREEN, 0.4f));
                    DrawTextEx(emojiFont, "YES", (Vector2){330, 300}, 40, 2, GREEN);
                    DrawTextEx(emojiFont, "NO", (Vector2){530, 300}, 40, 2, WHITE);
                } else {
                    DrawRectangleRounded((Rectangle){480, 280, 200, 70}, 0.3f, 10, Fade(RED, 0.4f));
                    DrawTextEx(emojiFont, "YES", (Vector2){330, 300}, 40, 2, WHITE);
                    DrawTextEx(emojiFont, "NO", (Vector2){530, 300}, 40, 2, RED);
                }
                // Timer countdown
                char buf[32];
                sprintf(buf, "Time left: %d", reviveTimer / 60);
                Vector2 timerSize = MeasureTextEx(emojiFont, buf, 30, 2);
                DrawTextEx(emojiFont, buf, 
                          (Vector2){GetScreenWidth()/2 - timerSize.x/2, 400}, 
                          30, 2, ORANGE);
                Vector2 instructionSize = MeasureTextEx(emojiFont, "Use LEFT/RIGHT to choose, ENTER to confirm", 20, 2);
                DrawTextEx(emojiFont, "Use LEFT/RIGHT to choose, ENTER to confirm", 
                          (Vector2){GetScreenWidth()/2 - instructionSize.x/2, 520}, 
                          20, 2, LIGHTGRAY);
            } break;

            case MINI_GAME: {
                if (selectedDifficulty == DIFFICULTY_EASY) {
                    if (ballsBg.id != 0) {
                        DrawTexturePro(
                            ballsBg,
                            (Rectangle){0, 0, (float)ballsBg.width, (float)ballsBg.height},
                            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                            (Vector2){0, 0}, 0.0f, WHITE
                        );
                    } else {
                        ClearBackground(RAYWHITE);
                    }

                    for (int i = 0; i < MAX_BALLS; i++) {
                        if (balls[i].active) {
                            DrawCircleV(balls[i].position, BALLS_RADIUS, balls[i].color);
                        }
                    }

                 // Draw basket using PNG
if (basketTex.id != 0) {
    // Display full original image, scaled to basketWidth while maintaining aspect ratio
    Rectangle sourceRec = { 0, 0, 
                          (float)basketTex.width, 
                          (float)basketTex.height };
    float aspectRatio = (float)basketTex.height / (float)basketTex.width;
    float scaledHeight = basketWidth * aspectRatio;
    Rectangle destRec = { basketX, basketY - scaledHeight, 
                        basketWidth, scaledHeight };
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
                    DrawText(TextFormat("Score: %d / %d", ballScore, TARGET_SCORE), 10, 10, 20, BLACK);
                    DrawText(TextFormat("Misses: %d / %d", misses, MAX_MISSES), 10, 40, 20, BLACK);
                } else if (selectedDifficulty == DIFFICULTY_MEDIUM) {
                    ClearBackground(BLACK);
                    DrawText("Wordle Mini-Game: Guess the 5-letter word", 100, 20, 24, WHITE);
                    DrawText("Type letters, BACKSPACE to delete, ENTER to submit", 100, 50, 18, WHITE);

                    const int boxSize = 60;
                    const int spacing = 5;
                    int startX = (GetScreenWidth() - (WORD_LENGTH * boxSize + (WORD_LENGTH - 1) * spacing)) / 2;
                    int startY = (GetScreenHeight() - (MAX_GUESSES * boxSize + (MAX_GUESSES - 1) * spacing)) / 2 + 50;

                    for (int row = 0; row < MAX_GUESSES; row++) {
                        for (int col = 0; col < WORD_LENGTH; col++) {
                            int x = startX + col * (boxSize + spacing);
                            int y = startY + row * (boxSize + spacing);
                            Color bgColor = LIGHTGRAY;
                            char letter = '\0';

                            if (row < currentGuess) {
                                bgColor = guessColors[row][col];
                                letter = guesses[row][col];
                            } else if (row == currentGuess && col < currentLetter) {
                                bgColor = WHITE;
                                letter = guesses[row][col];
                            }

                            DrawRectangle(x, y, boxSize, boxSize, bgColor);
                            DrawRectangleLines(x, y, boxSize, boxSize, BLACK);

                            if (letter != '\0') {
                                DrawText(TextFormat("%c", letter), x + boxSize/2 - 10, y + boxSize/2 - 20, 40, BLACK);
                            }
                        }
                    }

                    if (currentGuess == MAX_GUESSES && !wordleWon) {
                        DrawText(TextFormat("The word was: %s", targetWord), 100, GetScreenHeight() - 50, 20, RED);
                    }
                } else {
                    if (bowlingBg.id != 0) {
                        DrawTexturePro(bowlingBg,
                                       (Rectangle){0,0,(float)bowlingBg.width,(float)bowlingBg.height},
                                       (Rectangle){0,0,(float)GetScreenWidth(),(float)GetScreenHeight()},
                                       (Vector2){0,0}, 0.0f, WHITE);
                    } else {
                        ClearBackground(DARKGREEN);
                        DrawRectangle(LANE_LEFT - 20, 60, (LANE_RIGHT - LANE_LEFT) + 40, GetScreenHeight() - 120, BROWN);
                    }

                    DrawText("SECOND CHANCE! Score a STRIKE to revive!", 140, 20, 24, WHITE);
                    DrawText("Angle: LEFT/RIGHT | Power: Hold SPACE | S: Strike Mode", 120, 50, 18, WHITE);

                    for (int i = 0; i < NUM_PINS; i++) {
                        if (!pins[i].fallen || pins[i].animating) {
                            DrawCircleV(pins[i].position, PIN_RADIUS, WHITE);
                            DrawCircleV(pins[i].position, 8, RED);
                        }
                    }

                    DrawCircleV(ballPos, BOWLING_BALL_RADIUS, BLUE);

                    if (!ballLaunched) {
                        Vector2 guideEnd = {ballPos.x + 50 * sinf(throwAngle), ballPos.y - 50 * cosf(throwAngle)};
                        DrawLineEx(ballPos, guideEnd, 2, strikeMode ? RED : DARKBLUE);
                    }

                    if (charging) {
                        DrawRectangle(50, GetScreenHeight() - 40, (int)(200 * (power / maxPower)), 20, GREEN);
                        DrawRectangleLines(50, GetScreenHeight() - 40, 200, 20, BLACK);
                    }
                }
            } break;

            case CLOSING_SCENE: {
                if (homeBg.id != 0) {
                    DrawTexturePro(
                        homeBg,
                        (Rectangle){0, 0, (float)homeBg.width, (float)homeBg.height},
                        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                        (Vector2){0, 0}, 0.0f, WHITE
                    );
                } else {
                    ClearBackground(BLACK);
                }
                
                if (balhTex.id != 0) {
                    DrawTexture(balhTex, GetScreenWidth()/2 - balhTex.width/2, GetScreenHeight()/2 - balhTex.height - 50, WHITE);
                }
                
                const char* message = "OOPS THE POKEMON IS CAPTURED!";
                Vector2 textSize = MeasureTextEx(emojiFont, message, 40 * gameOverScale, 2);
                DrawTextEx(emojiFont, message, 
                          (Vector2){GetScreenWidth()/2 - textSize.x/2, GetScreenHeight()/2 - 50}, 
                          40 * gameOverScale, 2, RED);

                if (animationComplete) {
                    const char* replayText = "Press R to Replay";
                    const char* homeText = "Press H to go to Home Menu";
                    
                    Vector2 replaySize = MeasureTextEx(emojiFont, replayText, 20, 2);
                    Vector2 homeSize = MeasureTextEx(emojiFont, homeText, 20, 2);
                    
                    DrawTextEx(emojiFont, replayText, 
                              (Vector2){GetScreenWidth()/2 - replaySize.x/2, GetScreenHeight()/2 + 30}, 
                              20, 2, WHITE);
                    DrawTextEx(emojiFont, homeText, 
                              (Vector2){GetScreenWidth()/2 - homeSize.x/2, GetScreenHeight()/2 + 60}, 
                              20, 2, WHITE);
                }
            } break;
        }

        EndDrawing();
    }

    // Cleanup
    if (logo.id != 0) UnloadTexture(logo);
    if (homeBg.id != 0) UnloadTexture(homeBg);
    if (emojiFont.texture.id != 0) UnloadFont(emojiFont);
    if (pokeballTex.id != 0) UnloadTexture(pokeballTex);
    if (ultraBallTex.id != 0) UnloadTexture(ultraBallTex);
    if (masterBallTex.id != 0) UnloadTexture(masterBallTex);
    if (pikachuTex.id != 0) UnloadTexture(pikachuTex);
    if (raichuTex.id != 0) UnloadTexture(raichuTex);
    if (balhTex.id != 0) UnloadTexture(balhTex);
    if (aoiTex.id != 0) UnloadTexture(aoiTex);
    if (easyBg.id != 0) UnloadTexture(easyBg);
    if (mediumBg.id != 0) UnloadTexture(mediumBg);
    if (hardBg.id != 0) UnloadTexture(hardBg);
    if (obstacleTex.id != 0) UnloadTexture(obstacleTex);
    if (bowlingBg.id != 0) UnloadTexture(bowlingBg);
    if (ballsBg.id != 0) UnloadTexture(ballsBg);
    if (elixirTex.id != 0) UnloadTexture(elixirTex);
    if (thunderstoneTex.id != 0) UnloadTexture(thunderstoneTex);
    if (speedTex.id != 0) UnloadTexture(speedTex);
    if (powerTex.id != 0) UnloadTexture(powerTex);
    if (normalBulletTex.id != 0) UnloadTexture(normalBulletTex);
    if (specialBulletTex.id != 0) UnloadTexture(specialBulletTex);
    if (basketTex.id != 0) UnloadTexture(basketTex);
    if (hitSound.frameCount > 0) UnloadSound(hitSound);
    if (deadSound.frameCount > 0) UnloadSound(deadSound);
    if (bgm.ctxData != NULL) UnloadMusicStream(bgm);
    if (blurShader.id != 0) UnloadShader(blurShader);
    if (reviveTarget.id != 0) UnloadRenderTexture(reviveTarget);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}