#pragma once
#include "chunk.h"
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <mutex>

class World {
public:
    static void Init();
    static void Update(Vector3 playerPos);
    static void Draw(Camera3D camera); // ⬅️ Теперь принимает камеру

    static BlockType GetBlock(int x, int y, int z);
    static void SetBlock(int x, int y, int z, BlockType type);
    static Vector3 Raycast(Vector3 origin, Vector3 direction, float maxDistance, bool& hit, Vector3& hitNormal);

private:
    static std::unordered_map<long long, std::shared_ptr<Chunk>> chunks;
    static std::shared_mutex chunksMutex;
    static long long Hash(int cx, int cz);
    static std::shared_ptr<Chunk> GetOrCreateChunk(int cx, int cz);
    static void UnloadDistantChunks(Vector3 playerPos);
};