#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "block.h"
#include "world.h"
#include "player.h"
#include "ui.h"
#include "texture_atlas.h"
#include <cmath>

int main() {
    SetTraceLogLevel(LOG_WARNING); 

    // ⬇️ УБРАН FLAG_MSAA_4X_HINT ⬇️
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Minecraft Clone - raylib");
    DisableCursor();

    TextureAtlas::Init();
    BlockRegistry::Init();
    World::Init();

    World::Update({0, 0, 0}); 

    Player player;
    player.SetPosition({0.5f, 60.0f, 0.5f});

    const double targetFrameTime = 1.0 / 240.0;

    while (!WindowShouldClose()) {
        double frameStartTime = GetTime();
        float dt = GetFrameTime();

        World::Update(player.GetPosition());
        player.Update(dt);

        Vector3 origin = player.GetCamera().position;
        Vector3 dir = player.GetForward();
        bool hit = false;
        Vector3 hitNormal = {0, 0, 0};
        Vector3 hitBlock = World::Raycast(origin, dir, 6.0f, hit, hitNormal);

        if (hit) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                World::SetBlock(static_cast<int>(hitBlock.x),
                                static_cast<int>(hitBlock.y),
                                static_cast<int>(hitBlock.z),
                                BlockType::Air);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                Vector3 placePos = Vector3Add(hitBlock, hitNormal);
                int px = static_cast<int>(placePos.x);
                int py = static_cast<int>(placePos.y);
                int pz = static_cast<int>(placePos.z);

                Vector3 playerPos = player.GetPosition();
                bool insidePlayer = (px >= static_cast<int>(floorf(playerPos.x - PLAYER_RADIUS)) &&
                                     px <= static_cast<int>(floorf(playerPos.x + PLAYER_RADIUS)) &&
                                     py >= static_cast<int>(floorf(playerPos.y)) &&
                                     py <= static_cast<int>(floorf(playerPos.y + PLAYER_HEIGHT)) &&
                                     pz >= static_cast<int>(floorf(playerPos.z - PLAYER_RADIUS)) &&
                                     pz <= static_cast<int>(floorf(playerPos.z + PLAYER_RADIUS)));

                if (!insidePlayer) {
                    World::SetBlock(px, py, pz, BlockType::Stone);
                }
            }
        }

        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(player.GetCamera());
        World::Draw(player.GetCamera());
        if (hit) {
            UI::DrawBlockHighlight(hitBlock);
        }
        EndMode3D();

        player.DrawCrosshair();
        UI::Draw();

        if (hit) {
            DrawText(TextFormat("Looking at: %d, %d, %d",
                                static_cast<int>(hitBlock.x),
                                static_cast<int>(hitBlock.y),
                                static_cast<int>(hitBlock.z)),
                     10, SCREEN_HEIGHT - 30, 16, RAYWHITE);
        }

        EndDrawing();

        double frameTime = GetTime() - frameStartTime;
        double timeLeft = targetFrameTime - frameTime;
        if (timeLeft > 0) {
            WaitTime(timeLeft);
        }
    }

    TextureAtlas::Unload();
    CloseWindow();
    return 0;
}