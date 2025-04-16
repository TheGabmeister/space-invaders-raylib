#include "raylib.h"
#include <math.h>

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define NUM_SHOOTS 5
#define NUM_MAX_ENEMIES 50
#define FIRST_WAVE 50
#define GRID_WIDTH 10
#define GRID_HEIGHT 5
#define GRID_SPACING_X 50  // Adjust this value for desired horizontal spacing
#define GRID_SPACING_Y 50  // Adjust this value for desired vertical spacing
#define GRID_OFFSET_X 65
#define GRID_OFFSET_Y 120

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player{
    Rectangle rec;
    Vector2 speed;
    Color color;
} Player;

typedef struct Enemy{
    Rectangle rec;
    Vector2 speed;
    float baseSpeed;
    bool active;
    Color color;
} Enemy;

typedef struct Shoot{
    Rectangle rec;
    Vector2 speed;
    bool active;
    Color color;
} Shoot;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = 600;
static const int screenHeight = 800;

static bool gameOver = false;
static bool pause =  false;
static int score = 0;
static int highScore = 0;
static bool victory = false;

static Player player = { 0 };
static Texture2D playerTexture;
static Enemy enemy[NUM_MAX_ENEMIES] = { 0 };
static Shoot shoot[NUM_SHOOTS] = { 0 };

static int shootRate = 0;
static float alpha = 0.0f;

static int activeEnemies = 0;
static int enemiesKill = 0;
static bool smooth = false;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "classic game: space invaders");

    InitGame();

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
    
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame();         // Unload loaded data (textures, sounds, models...)

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Initialize game variables
    shootRate = 0;
    pause = false;
    gameOver = false;
    victory = false;
    smooth = false;
    activeEnemies = 50;
    enemiesKill = 0;
    score = 0;
    alpha = 0;

    // Initialize player
    player.rec.x = screenWidth / 2;
    player.rec.y = screenHeight - 50;
    player.rec.width = 40;
    player.rec.height = 40;
    player.speed.x = 5;
    player.speed.y = 5;
    player.color = BLACK;
    playerTexture = LoadTexture("resources/player-ship.png");

    // Initialize enemies in a grid
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        enemy[i].rec.width = 20;
        enemy[i].rec.height = 20;
        
        enemy[i].speed.x = 0.5;
        enemy[i].speed.y = 0.5;
        enemy[i].baseSpeed = 0.5;
        enemy[i].active = true;
        enemy[i].color = GRAY;

        int row = i / GRID_WIDTH;
        int col = i % GRID_WIDTH;
        enemy[i].rec.x = col * GRID_SPACING_X + GRID_OFFSET_X;
        enemy[i].rec.y = row * GRID_SPACING_Y + GRID_OFFSET_Y;
    }

    // Initialize shoots
    for (int i = 0; i < NUM_SHOOTS; i++)
    {
        shoot[i].rec.y = player.rec.y;
        shoot[i].rec.x = player.rec.x + player.rec.width / 4;
        shoot[i].rec.width = 5;
        shoot[i].rec.height = 10;
        shoot[i].speed.x = 0;
        shoot[i].speed.y = 7;
        shoot[i].active = false;
        shoot[i].color = MAROON;
    }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver)
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (!pause)
        {
            

            // Player movement
            if (IsKeyDown(KEY_RIGHT)) player.rec.x += player.speed.x;
            if (IsKeyDown(KEY_LEFT)) player.rec.x -= player.speed.x;
            if (IsKeyDown(KEY_UP)) player.rec.y -= player.speed.y;
            if (IsKeyDown(KEY_DOWN)) player.rec.y += player.speed.y;

            // Player collision with enemy
            for (int i = 0; i < activeEnemies; i++)
            {
                if (CheckCollisionRecs(player.rec, enemy[i].rec)) 
                      gameOver = true;
            }

            // Enemy behaviour ------------------------------------------
            static int direction = 1; // 1 for right, -1 for left
            static float moveDown = 0;
            static bool reachedEdge = false;

            for (int i = 0; i < activeEnemies; i++)
            {
                if (enemy[i].active)
                {
                    
                    enemy[i].rec.x += enemy[i].speed.x * direction;

                    if (enemy[i].rec.x <= 0 || enemy[i].rec.x + enemy[i].rec.width >= screenWidth)
                    {
                        reachedEdge = true;
                    }
                }
            }

            if (reachedEdge)
            {
                direction *= -1; // change direction
                moveDown += 10; // move down a bit
                reachedEdge = false;
            }

            for (int i = 0; i < activeEnemies; i++)
            {
                if (enemy[i].active)
                {
                    enemy[i].rec.y += moveDown;
                }
            }

            moveDown = 0; // reset moveDown for the next frame

            // ------------------------------------------------------

            // Wall behaviour
            if (player.rec.x <= 0) player.rec.x = 0;
            if (player.rec.x + player.rec.width >= screenWidth) player.rec.x = screenWidth - player.rec.width;
            if (player.rec.y <= 0) player.rec.y = 0;
            if (player.rec.y + player.rec.height >= screenHeight) player.rec.y = screenHeight - player.rec.height;

            // Shoot initialization
            if (IsKeyDown(KEY_SPACE))
            {
                shootRate += 5;

                for (int i = 0; i < NUM_SHOOTS; i++)
                {
                    if (!shoot[i].active && shootRate%20 == 0)
                    {
                        shoot[i].rec.x = player.rec.x + player.rec.width / 4;
                        shoot[i].rec.y = player.rec.y;
                        shoot[i].active = true;
                        break;
                    }
                }
            }

            // Shoot logic
            for (int i = 0; i < NUM_SHOOTS; i++)
            {
                if (shoot[i].active)
                {
                    // Movement
                    shoot[i].rec.y -= shoot[i].speed.y;

                    // Collision with enemy
                    for (int j = 0; j < activeEnemies; j++)
                    {
                        if (enemy[j].active)
                        {
                            if (CheckCollisionRecs(shoot[i].rec, enemy[j].rec))
                            {
                                shoot[i].active = false;
                                enemy[j].active = false;
                                enemy[j].rec.x = 0;
                                enemy[j].rec.y = 0;
                                shootRate = 0;
                                enemiesKill++;
                                score += 100;

                                // Adjust all enemies' speed for every enemy killed
                                for (int k = 0; k < activeEnemies; k++)
                                { 
                                    enemy[k].speed.x = enemy[k].baseSpeed * (1 + pow(((enemiesKill * 2.0f) / NUM_MAX_ENEMIES), 4) );
                                    
                                }
                                 
                            }

                            if (shoot[i].rec.y + shoot[i].rec.height < 0)
                            {
                                shoot[i].active = false;
                                shootRate = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

        ClearBackground(BLACK);

        if (!gameOver)
        {
            DrawTextureEx(
                playerTexture,
                (Vector2){ player.rec.x, player.rec.y },
                0.0f,
                player.rec.width / playerTexture.width,
                WHITE
            );
            for (int i = 0; i < activeEnemies; i++)
            {
                if (enemy[i].active) DrawRectangleRec(enemy[i].rec, enemy[i].color);
                
            }

            for (int i = 0; i < NUM_SHOOTS; i++)
            {
                if (shoot[i].active) DrawRectangleRec(shoot[i].rec, shoot[i].color);
            }

             // --- Add these lines for the labels ---
             DrawText("SCORE<1>", 20, 20, 25, LIGHTGRAY);
             DrawText("HI-SCORE", screenWidth/2 - MeasureText("HI-SCORE", 20)/2, 20, 25, LIGHTGRAY);
             DrawText("SCORE<2>", screenWidth - 40 - MeasureText("SCORE<2>", 20), 20, 25, LIGHTGRAY);
             // --------------------------------------
            
            DrawText(TextFormat("%04i", score), 30, 60, 25, LIGHTGRAY);
            DrawText(TextFormat("%04i", highScore), screenWidth / 2 - MeasureText("HI-SCORE", 20) / 4, 60, 25, LIGHTGRAY);
            DrawText("0000", screenWidth - 20 - MeasureText("SCORE<2>", 20), 60, 25, LIGHTGRAY);

            if (victory) DrawText("YOU WIN", screenWidth/2 - MeasureText("YOU WIN", 40)/2, screenHeight/2 - 40, 40, BLACK);

            if (pause) DrawText("GAME PAUSED", screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, screenHeight/2 - 40, 40, GRAY);
        }
        else DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth()/2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20)/2, GetScreenHeight()/2 - 50, 20, GRAY);

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}
