#pragma once

#include "chunk.h"
#include <unordered_map>
#include <memory>

class World {
public:
    static void Init();
    static void Update(Vector3 playerPos);
    static void Draw();

    static BlockType GetBlock(int x, int y, int z);
    static void SetBlock(int x, int y, int z, BlockType type);
    
    // Новый метод для поиска высоты поверхности
    static int GetHighestBlock(int x, int z);

    static Vector3 Raycast(Vector3 origin, Vector3 direction, float maxDistance, bool& hit, Vector3& hitNormal);

private:
    static std::unordered_map<long long, std::unique_ptr<Chunk>> chunks;
    static long long Hash(int cx, int cz);
    static Chunk* GetOrCreateChunk(int cx, int cz);
    static void UnloadDistantChunks(Vector3 playerPos);
};