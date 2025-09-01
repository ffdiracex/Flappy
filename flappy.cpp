//
// Created by ffdiracex on 9/1/25.
//

#include <vector>
#include <raylib.h>
#include <algorithm>

//Game states
enum GameState {MENU, PLAYING, GAME_OVER, CREDITS};

bool SoundIsValid(Sound sound) {
    return sound.frameCount > 0;
}

bool TextureIsValid(Texture2D texture) {
    return texture.id > 0;
}


int main() {
    //initialize window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Flappy");
    InitAudioDevice();
    SetTargetFPS(60);

    //load resouces / sound
    Sound jumpSound = LoadSound("resources/jump.wav");
    Sound scoreSound = LoadSound("resources/score.wav");
    Sound crashSound = LoadSound("resources/crash.wav");

    //fallback if sounds can't be loaded
    if (!IsSoundValid(jumpSound)) jumpSound = (Sound){0};
    if (!IsSoundValid(scoreSound)) scoreSound = (Sound){0};
    if (!IsSoundValid(crashSound)) crashSound = (Sound){0};

    Texture2D background = LoadTexture("resources/background.png");
    Texture2D birdTexture = LoadTexture("resources/bird.png");
    Texture2D pipeTexture = LoadTexture("resources/pipe.png");

    //Fallback textures
    if (!IsTextureValid(background)) background = (Texture2D){0};
    if (!IsTextureValid(birdTexture)) birdTexture = (Texture2D){0};
    if (!IsTextureValid(pipeTexture)) pipeTexture = (Texture2D){0};

    //game state
    GameState gameState = MENU;
    int score = 0;
    int highScore = 0;

    //bird properties
    float birdY = screenHeight / 2;
    float birdVelocity = 0;
    const float gravity = 0.5f;
    const float jumpStrength = -10.0f;
    const int birdWidth = 40;
    const int birdHeight = 30;
    Rectangle birdRect = {screenWidth / 4, birdY, birdWidth, birdHeight};
    float birdRotation = 0;
    int flapTimer = 0;

    //pipe properties
    struct Pipe {
        Rectangle topRect;
        Rectangle bottomRect;
        bool scored;
    };

    std::vector<Pipe> pipes;
    const int pipeWidth = 80;
    const int pipeGapSize = 200;
    float pipeSpeed = 3.0f;
    float pipeSpawnTimer = 0;
    const float pipeSpawnInterval = 2.0f; //seconds between pipes

    //Cloud properties for parallax effect

    struct Cloud {
        Vector2 position;
        float scale;
        float speed;
    };

    std::vector<Cloud> clouds;
    for (int i=0; i < 5; i++) {
        clouds.push_back({
        {(float)GetRandomValue(-100,screenWidth), (float)GetRandomValue(50,200)},
        GetRandomValue(5,15) / 10.0f,
        GetRandomValue(10,30) / 10.0f});
    }

    //ground position for scrolling
    float groundOffset = 0;
    float gameOverTimer = 0;
    const float gameOverDelay = 1.0f;

    //main game loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        //update based on game state
        switch (gameState) {
            case MENU: {
                if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameState = PLAYING;
                    //reset game variables
                    birdY = screenHeight / 2;
                    birdVelocity = 0;
                    birdRect.y = birdY;
                    pipes.clear();
                    score = 0;
                    pipeSpawnTimer = 0;
                }
                if (IsKeyPressed(KEY_C)) {
                    gameState = CREDITS;
                }
                break;
            }
            case PLAYING: {
                //bird physics
                if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    birdVelocity = jumpStrength;
                    PlaySound(jumpSound);
                    flapTimer = 10;
                }
                birdVelocity += gravity;
                birdY += birdVelocity;
                birdRect.y = birdY;

                //Bird rotation based on velocity
                birdRotation = birdVelocity * 2;
                if (birdRotation < -30) birdRotation = -30;
                if (birdRotation > 60) birdRotation = 60;

                //animate flapping
                if (flapTimer > 0) flapTimer--;

                //Spawn pipes
                pipeSpawnTimer += deltaTime;
                if (pipeSpawnTimer >= pipeSpawnInterval) {
                    pipeSpawnTimer = 0;
                    int gapY = GetRandomValue(150,screenHeight - 150 - pipeGapSize/2);
                    Pipe newPipe;
                    newPipe.topRect = {(float)screenWidth,0,(float)pipeWidth,(float)gapY - pipeGapSize/2};
                    newPipe.bottomRect = {(float)screenWidth,(float)gapY + pipeGapSize/2, (float)pipeWidth,(float)screenHeight - (gapY + pipeGapSize/2)};
                    newPipe.scored = false;

                    pipes.push_back(newPipe);
                }
                //update pipes
                for (auto& pipe : pipes) {
                    pipe.topRect.x -= pipeSpeed;
                    pipe.bottomRect.x -= pipeSpeed;

                    //scoring
                    if (!pipe.scored && pipe.topRect.x + pipe.topRect.width < birdRect.x) {
                        score++;
                        PlaySound(scoreSound);
                        pipe.scored = true;
                    }
                }
                //remove off screen pipes
                pipes.erase(std::remove_if(pipes.begin(),pipes.end(), [](const Pipe& p) {return p.topRect.x + p.topRect.width < -50; }),
                    pipes.end());

                //collision detection
                bool collision = false;

                //ground collision
                if (birdY + birdHeight > screenHeight - 50) {
                    collision = true;
                    birdY = screenHeight - 50 - birdHeight;
                }
                //ceiling collision
                if (birdY < 0) {
                    collision = true;
                    birdY = 0;
                }

                //pipe collision
                for (const auto& pipe : pipes) {
                    if (CheckCollisionRecs(birdRect, pipe.topRect) ||
                        CheckCollisionRecs(birdRect,pipe.bottomRect)) {
                        collision = true;
                        break;
                    }
                }

                if (collision) {
                    PlaySound(jumpSound);
                    gameState = GAME_OVER;
                    gameOverTimer = 0;

                    if (score > highScore) {
                        highScore = score;
                    }
                }

                //update clouds
                for (auto& cloud : clouds) {
                    cloud.position.x -= cloud.speed;
                    if (cloud.position.x < -100) {
                        cloud.position.x = screenWidth + 50;
                        cloud.position.y = GetRandomValue(50,200);
                    }
                }

                //update ground scrolling
                groundOffset -= pipeSpeed;
                if (groundOffset <= -screenWidth) groundOffset = 0;
                break;
            }
            case GAME_OVER: {
                gameOverTimer += deltaTime;
                if (gameOverTimer >= gameOverDelay && (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
                    gameState = MENU;
                }
                break;
            }
            case CREDITS: {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameState = MENU;
                }
                break;
            }
        }
        //draw everything
        BeginDrawing();
        ClearBackground(SKYBLUE);

        //Draw background
        if (IsTextureValid(background)) {
            DrawTexture(background,0,0,WHITE);
        }else {
            //fallback background
            DrawRectangleGradientV(0,0,screenWidth,screenHeight,SKYBLUE,BLUE);
        }
        //draw clouds
        for (const auto& cloud: clouds) {
            DrawCircleV(cloud.position, 30*cloud.scale, Fade(WHITE,0.8f));
            DrawCircleV({cloud.position.x +25*cloud.scale, cloud.position.y - 10*cloud.scale},
                25*cloud.scale, Fade(WHITE, 0.8f));
            DrawCircleV({cloud.position.x - 20*cloud.scale, cloud.position.y + 10*cloud.scale},
                20*cloud.scale, Fade(WHITE,0.8f));
        }

        //draw pipes
        for (const auto& pipe : pipes) {
            if (IsTextureValid(pipeTexture)) {
                //Top pipe (flipped)
                DrawTexturePro(pipeTexture, {0,0,(float)pipeTexture.width,(float)pipeTexture.height},
                    {pipe.topRect.x,pipe.topRect.y,pipe.topRect.width,pipe.topRect.height},
                    {0,0},180,WHITE);
                //bottom pipe
                DrawTexturePro(pipeTexture,{0,0,(float)pipeTexture.width,(float)pipeTexture.height},
                    {pipe.bottomRect.x,pipe.bottomRect.y,pipe.bottomRect.width,pipe.bottomRect.height},
                    {0,0},0,WHITE);
            } else {
                //fallback pipe drawing
                DrawRectangleRec(pipe.topRect, GREEN);
                DrawRectangleRec(pipe.bottomRect, GREEN);

                //pipe caps
                DrawRectangle(pipe.topRect.x - 5, pipe.topRect.y + pipe.topRect.height - 10,pipe.topRect.width + 10,10,DARKGREEN);
                DrawRectangle(pipe.bottomRect.x - 5,pipe.bottomRect.y,pipe.bottomRect.width + 10,10,DARKGREEN);
            }
        }
        //draw ground
        DrawRectangle(0,screenHeight - 50, screenWidth,50,BROWN);
        for (int i=0; i < screenWidth / 50 + 2;i++) {
            DrawRectangle(i*50+(int)groundOffset,screenHeight - 50,50,5,DARKBROWN);
            DrawRectangle(i*50+25+(int)groundOffset,screenHeight - 45,50,5,DARKBROWN);
        }

        //draw bird
        if (IsTextureValid(birdTexture)) {
            DrawTexturePro(birdTexture, {0,0,(float)birdTexture.width,(float)birdTexture.height},
                {birdRect.x,birdRect.y,birdRect.width,birdRect.height},
                {birdRect.width/2,birdRect.height/2},birdRotation, WHITE);
        } else {
            //Fallback bird drawing
            DrawCircle(birdRect.x + birdRect.width/2,birdRect.y + birdRect.height/2,birdRect.width/2,flapTimer > 0 ? ORANGE : YELLOW);
            DrawCircle(birdRect.x + birdRect.width/2 + 8,birdRect.y + birdRect.height/2 - 5, 4,BLACK);
            DrawCircle(birdRect.x + birdRect.width/2 + 8, birdRect.y + birdRect.height/2 -5,2,WHITE);
        }

        //Draw UI based on game state
        switch (gameState) {
            case MENU: {
                DrawText("FLAPPY", screenWidth/2 - MeasureText("FLAPPY",50)/2,100,50,YELLOW);
                DrawText("Press Enter or CLICK to Start", screenWidth/2 - MeasureText("Press ENTER or CLICK to Start",30)/2,250,30,WHITE);
                DrawText("Press C for Credits",screenWidth/2 - MeasureText("Press C for Credits",20)/2,300,20,LIGHTGRAY);
                DrawText(TextFormat("High Score: %d",highScore), screenWidth/2 - MeasureText(TextFormat("High Score: %d", highScore), 30)/2,350,30,GOLD);
                break;
            }

            case PLAYING: {
                DrawText(TextFormat("Score: %d", score),20,20,30,WHITE);
                DrawText(TextFormat("High Score: %d",highScore),20,60,20,LIGHTGRAY);
                break;
            }
            case GAME_OVER: {
                //Semi-transparent overlay
                DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.5f));
                DrawText("GAME OVER",screenWidth/2 - MeasureText("GAME OVER",50)/2,150,50,RED);
                DrawText(TextFormat("Score: %d",highScore),screenWidth/2 - MeasureText(TextFormat("High Score: %d",highScore),30)/2,270,30,GOLD);
                if (gameOverTimer >= gameOverDelay) {
                    DrawText("Press ENTER or CLICK to Continue", screenWidth/2 - MeasureText("Press ENTER or CLICK to Continue",25)/2,350,25,LIGHTGRAY);
                }
                break;
            }
            case CREDITS: {
                DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.8f));
                DrawText("CREDITS",screenWidth/2 - MeasureText("CREDITS",50)/2,100,50,YELLOW);
                DrawText("Game created with Raylib",screenWidth/2 - MeasureText("Game created with Raylib",30)/2,180,30,WHITE);
                DrawText("Coded by me",screenWidth/2 - MeasureText("Coded by me",25)/2,230,25,LIGHTGRAY);
                DrawText("Art: Raylib Defaults", screenWidth/2 - MeasureText("Art: Raylib Defaults", 25)/2, 260, 25, LIGHTGRAY);
                DrawText("Sound Effects: freesound.org", screenWidth/2 - MeasureText("Sound Effects: freesound.org", 25)/2, 290, 25, LIGHTGRAY);

                DrawText("Press ENTER or ESC to Return", screenWidth/2 - MeasureText("Press ENTER or ESC to Return", 20)/2, 400, 20, GRAY);
                break;
            }
        }
        EndDrawing();
    }
    //cleanup
    if (IsSoundValid(jumpSound)) UnloadSound(jumpSound);
    if (IsSoundValid(scoreSound)) UnloadSound(scoreSound);
    if (IsSoundValid(crashSound)) UnloadSound(crashSound);
    if (IsTextureValid(background)) UnloadTexture(background);
    if (IsTextureValid(birdTexture)) UnloadTexture(birdTexture);
    if (IsTextureValid(pipeTexture)) UnloadTexture(pipeTexture);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}