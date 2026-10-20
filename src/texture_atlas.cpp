#include "texture_atlas.h"
#include "raymath.h"

Texture2D TextureAtlas::atlas = { 0 };

// Надежная хеш-функция для генерации случайных чисел на лету
static unsigned int Hash2D(int x, int y, int seed) {
    unsigned int h = x * 374761393 + y * 668265263 + seed * 1442695040888963407ULL;
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

void TextureAtlas::Init() {
    // Атлас 512x512 (16 тайлов по 32 пикселя)
    Image img = GenImageColor(512, 512, BLANK);
    const int TILE_SIZE = 32;

    // 1. ЗЕМЛЯ (Dirt)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 1) % 100;
            Color c = { 134, 96, 67, 255 }; // Базовый
            
            if (n < 15) c = { 94, 56, 27, 255 };        // Темные комки
            else if (n < 30) c = { 160, 120, 90, 255 }; // Светлые камушки
            else if (n < 40) c = { 150, 110, 80, 255 }; // Светлая земля
            
            ImageDrawPixel(&img, 2 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 2. КАМЕНЬ (Stone) - Теперь это просто шумная крошка, как в оригинале
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 2) % 100;
            Color c = { 128, 128, 128, 255 }; // Базовый серый
            
            // Распределяем оттенки для создания эффекта "крошки"
            if (n < 15) c = { 95, 95, 95, 255 };        // Темные пятна
            else if (n < 25) c = { 80, 80, 80, 255 };   // Очень темные
            else if (n < 45) c = { 145, 145, 145, 255 }; // Светлые пятна
            else if (n < 55) c = { 160, 160, 160, 255 }; // Очень светлые
            
            ImageDrawPixel(&img, 3 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 3. ПЕСОК (Sand)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 4) % 100;
            Color c = { 218, 204, 155, 255 }; // Базовый
            
            if (n < 20) c = { 198, 184, 135, 255 }; // Темные песчинки
            else if (n < 40) c = { 238, 224, 175, 255 }; // Светлые песчинки
            
            ImageDrawPixel(&img, 4 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 4. ДЕРЕВО (Wood)
    for (int x = 0; x < TILE_SIZE; x++) {
        unsigned int lineNoise = Hash2D(x, 0, 5) % 100;
        for (int y = 0; y < TILE_SIZE; y++) {
            Color c;
            if (lineNoise < 25) c = { 82, 47, 22, 255 };  // Темная кора
            else if (lineNoise < 50) c = { 122, 87, 62, 255 }; // Светлая кора
            else c = { 102, 67, 42, 255 };                  // Базовая
            
            // Продольные волокна
            if (Hash2D(x, y, 6) % 100 < 15) {
                c = { 72, 37, 12, 255 };
            }
            
            ImageDrawPixel(&img, 5 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 5. ЛИСТВА (Leaves)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 7) % 100;
            Color c = { 60, 120, 50, 255 }; // Базовый
            
            if (n < 25) c = { 30, 90, 20, 255 };        // Темные дырки
            else if (n < 45) c = { 90, 160, 70, 255 };  // Светлые листья
            else if (n < 55) c = { 140, 210, 120, 255 }; // Яркие блики
            
            ImageDrawPixel(&img, 6 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 6. ТРАВА (Верхняя грань)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 8) % 100;
            Color c = { 95, 159, 53, 255 }; // Базовый
            
            if (n < 20) c = { 65, 129, 23, 255 };       // Темная
            else if (n < 40) c = { 115, 179, 73, 255 };  // Светлая
            else if (n < 50) c = { 140, 200, 90, 255 }; // Яркий блик
            
            ImageDrawPixel(&img, 0 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 7. ТРАВА (Боковая грань)
    for (int x = 0; x < TILE_SIZE; x++) {
        int grassHeight = 6 + (Hash2D(x, 0, 9) % 6); // Высота 6-11 пикселей
        
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 1) % 100;
            Color c;
            
            if (y < grassHeight - 2) {
                // Трава сверху
                if (n < 25) c = { 75, 139, 33, 255 };
                else if (n < 45) c = { 115, 179, 73, 255 };
                else c = { 95, 159, 53, 255 };
            } else if (y >= grassHeight - 2 && y <= grassHeight) {
                // Темная граница
                c = { 60, 110, 30, 255 }; 
            } else {
                // Земля снизу
                if (n < 15) c = { 94, 56, 27, 255 };
                else if (n < 30) c = { 160, 120, 90, 255 };
                else if (n < 40) c = { 150, 110, 80, 255 };
                else c = { 134, 96, 67, 255 };
            }
            
            ImageDrawPixel(&img, 1 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    atlas = LoadTextureFromImage(img);
    SetTextureFilter(atlas, TEXTURE_FILTER_POINT); 
    UnloadImage(img);
}

void TextureAtlas::Unload() {
    UnloadTexture(atlas);
}

Texture2D TextureAtlas::GetTexture() {
    return atlas;
}

Rectangle TextureAtlas::GetUV(int tileIndex) {
    int tx = tileIndex % 16;
    int ty = tileIndex / 16;
    
    return {
        (float)tx / 16.0f,
        (float)ty / 16.0f,
        1.0f / 16.0f,
        1.0f / 16.0f
    };
}