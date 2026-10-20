#include "ui.h"
#include "config.h"
#include "world.h"
#include "raylib.h"

namespace UI {

void Draw() {
    DrawFPS(10, 10);
    DrawText("Minecraft Clone (raylib)", 10, 40, 20, RAYWHITE);
    DrawText("WASD - Move | Mouse - Look | Space - Jump", 10, 70, 16, LIGHTGRAY);
    DrawText("LMB - Break | RMB - Place", 10, 90, 16, LIGHTGRAY);
}

void DrawBlockHighlight(Vector3 blockPos) {
    Vector3 center = { blockPos.x + 0.5f, blockPos.y + 0.5f, blockPos.z + 0.5f };
    DrawCubeWires(center, 1.01f, 1.01f, 1.01f, BLACK);
    DrawCubeWires(center, 1.0f, 1.0f, 1.0f, WHITE);
}

} // namespace UI