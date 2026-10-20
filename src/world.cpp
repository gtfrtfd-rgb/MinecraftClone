#include "world.h"
#include "config.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <vector>

std::unordered_map<long long, std::unique_ptr<Chunk>> World::chunks;

long long World::Hash(int cx, int cz) {
    return (static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cz);
}

void World::Init() {
    chunks.clear();
}

Chunk* World::GetOrCreateChunk(int cx, int cz) {
    long long key = Hash(cx, cz);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        auto chunk = std::make_unique<Chunk>(cx, cz);
        chunk->Generate();
        Chunk* ptr = chunk.get();
        chunks[key] = std::move(chunk);
        return ptr;
    }
    return it->second.get();
}

void World::Update(Vector3 playerPos) {
    int pcx = static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE));
    int pcz = static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE));

    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
        for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; dz++) {
            GetOrCreateChunk(pcx + dx, pcz + dz);
        }
    }

    for (auto& [key, chunk] : chunks) {
        if (chunk->IsDirty()) {
            chunk->BuildMesh();
        }
    }

    UnloadDistantChunks(playerPos);
}

void World::Draw() {
    for (auto& [key, chunk] : chunks) {
        chunk->Draw();
    }
}

BlockType World::GetBlock(int x, int y, int z) {
    if (y < 0 || y >= CHUNK_HEIGHT) return BlockType::Air;

    int cx = static_cast<int>(std::floor(static_cast<float>(x) / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor(static_cast<float>(z) / CHUNK_SIZE));
    int lx = x - cx * CHUNK_SIZE;
    int lz = z - cz * CHUNK_SIZE;

    long long key = Hash(cx, cz);
    auto it = chunks.find(key);
    if (it == chunks.end()) return BlockType::Air;

    return it->second->GetBlock(lx, y, lz);
}

int World::GetHighestBlock(int x, int z) {
    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
        if (GetBlock(x, y, z) != BlockType::Air) {
            return y;
        }
    }
    return 0;
}

void World::SetBlock(int x, int y, int z, BlockType type) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;

    int cx = static_cast<int>(std::floor(static_cast<float>(x) / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor(static_cast<float>(z) / CHUNK_SIZE));
    int lx = x - cx * CHUNK_SIZE;
    int lz = z - cz * CHUNK_SIZE;

    Chunk* chunk = GetOrCreateChunk(cx, cz);
    chunk->SetBlock(lx, y, lz, type);

    if (lx == 0)              { auto* c = GetOrCreateChunk(cx-1, cz); c->MarkDirty(); }
    if (lx == CHUNK_SIZE - 1) { auto* c = GetOrCreateChunk(cx+1, cz); c->MarkDirty(); }
    if (lz == 0)              { auto* c = GetOrCreateChunk(cx, cz-1); c->MarkDirty(); }
    if (lz == CHUNK_SIZE - 1) { auto* c = GetOrCreateChunk(cx, cz+1); c->MarkDirty(); }
}

Vector3 World::Raycast(Vector3 origin, Vector3 direction, float maxDistance, bool& hit, Vector3& hitNormal) {
    hit = false;
    hitNormal = {0, 0, 0};

    for (float t = 0; t < maxDistance; t += 0.05f) {
        Vector3 checkPos = Vector3Add(origin, Vector3Scale(direction, t));
        int bx = static_cast<int>(std::floor(checkPos.x));
        int by = static_cast<int>(std::floor(checkPos.y));
        int bz = static_cast<int>(std::floor(checkPos.z));

        if (GetBlock(bx, by, bz) != BlockType::Air) {
            hit = true;
            Vector3 local = { checkPos.x - bx - 0.5f, checkPos.y - by - 0.5f, checkPos.z - bz - 0.5f };
            float ax = fabsf(local.x), ay = fabsf(local.y), az = fabsf(local.z);
            if (ax > ay && ax > az)      hitNormal = { (local.x > 0) ? 1.0f : -1.0f, 0, 0 };
            else if (ay > az)             hitNormal = { 0, (local.y > 0) ? 1.0f : -1.0f, 0 };
            else                          hitNormal = { 0, 0, (local.z > 0) ? 1.0f : -1.0f };
            return { static_cast<float>(bx), static_cast<float>(by), static_cast<float>(bz) };
        }
    }

    return {0, 0, 0};
}

void World::UnloadDistantChunks(Vector3 playerPos) {
    int pcx = static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE));
    int pcz = static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE));

    std::vector<long long> toRemove;
    for (auto& [key, chunk] : chunks) {
        int dx = chunk->GetCX() - pcx;
        int dz = chunk->GetCZ() - pcz;
        if (std::abs(dx) > RENDER_DISTANCE + 2 || std::abs(dz) > RENDER_DISTANCE + 2) {
            toRemove.push_back(key);
        }
    }
    for (long long key : toRemove) {
        chunks.erase(key);
    }
}