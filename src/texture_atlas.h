#pragma once

#include "raylib.h"

class TextureAtlas {
public:
    static void Init();
    static void Unload();
    static Texture2D GetTexture();

    // Индексы наших текстур (плиток в атласе)
    enum Tile {
        TILE_GRASS_TOP = 0,
        TILE_GRASS_SIDE = 1,
        TILE_DIRT = 2,
        TILE_STONE = 3,
        TILE_SAND = 4,
        TILE_WOOD = 5,
        TILE_LEAVES = 6
    };

    static Rectangle GetUV(int tileIndex);

private:
    static Texture2D atlas;
};