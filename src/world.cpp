#include "world.h"
#include "config.h"
#include "raymath.h"
#include "thread_pool.h"
#include <cmath>
#include <algorithm>
#include <vector>

std::unordered_map<long long, std::shared_ptr<Chunk>> World::chunks;
std::shared_mutex World::chunksMutex;
static ThreadPool pool(std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1);

long long World::Hash(int cx, int cz) {
    return (static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cz);
}

void World::Init() {
    std::unique_lock lock(chunksMutex);
    chunks.clear();
}

std::shared_ptr<Chunk> World::GetOrCreateChunk(int cx, int cz) {
    long long key = Hash(cx, cz);
    {
        std::shared_lock lock(chunksMutex);
        auto it = chunks.find(key);
        if (it != chunks.end()) return it->second;
    }
    
    std::unique_lock lock(chunksMutex);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        auto chunk = std::make_shared<Chunk>(cx, cz);
        chunks[key] = chunk;
        return chunk;
    }
    return it->second;
}

void World::Update(Vector3 playerPos) {
    int pcx = static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE));
    int pcz = static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE));

    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
        for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; dz++) {
            auto chunk = GetOrCreateChunk(pcx + dx, pcz + dz);
            
            ChunkState s = chunk->state.load();
            if (s == ChunkState::Empty) {
                chunk->state = ChunkState::Generating;
                pool.enqueue([chunk]() { 
                    chunk->GenerateData(); 
                    chunk->state = ChunkState::Generated; 
                });
            } else if (s == ChunkState::Generated) {
                chunk->state = ChunkState::Meshing;
                pool.enqueue([chunk]() { 
                    chunk->BuildMeshData(); 
                    chunk->state = ChunkState::MeshReady; 
                });
            }
        }
    }

    {
        std::shared_lock lock(chunksMutex);
        for (auto& [key, chunk] : chunks) {
            if (chunk->state.load() == ChunkState::MeshReady) {
                chunk->UploadToGPU();
            }
        }
    }

    UnloadDistantChunks(playerPos);
}

void World::Draw(Camera3D camera) {
    std::shared_lock lock(chunksMutex);
    
    // Вычисляем вектор направления взгляда камеры
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    
    // Радиус сферы, описывающей весь чанк (половина диагонали 8x64x8)
    // sqrt(4^2 + 32^2 + 4^2) ≈ 32.5
    const float chunkRadius = 32.5f;

    for (auto& [key, chunk] : chunks) {
        // Центр текущего чанка
        Vector3 chunkCenter = { 
            chunk->GetCX() * CHUNK_SIZE + CHUNK_SIZE * 0.5f, 
            CHUNK_HEIGHT * 0.5f, 
            chunk->GetCZ() * CHUNK_SIZE + CHUNK_SIZE * 0.5f 
        };
        
        Vector3 dirToChunk = Vector3Subtract(chunkCenter, camera.position);
        float dot = Vector3DotProduct(camForward, dirToChunk);
        
        // Если скалярное произведение отрицательное и по модулю больше радиуса, 
        // значит чанк полностью находится за спиной игрока — пропускаем отрисовку.
        if (dot < -chunkRadius) {
            continue; 
        }

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
    
    std::shared_lock lock(chunksMutex); 
    auto it = chunks.find(key);
    if (it == chunks.end()) return BlockType::Air;

    return it->second->GetBlock(lx, y, lz);
}

void World::SetBlock(int x, int y, int z, BlockType type) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;

    int cx = static_cast<int>(std::floor(static_cast<float>(x) / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor(static_cast<float>(z) / CHUNK_SIZE));
    int lx = x - cx * CHUNK_SIZE;
    int lz = z - cz * CHUNK_SIZE;

    auto chunk = GetOrCreateChunk(cx, cz);
    chunk->SetBlock(lx, y, lz, type);

    if (lx == 0)              { auto c = GetOrCreateChunk(cx-1, cz); c->state = ChunkState::Generated; }
    if (lx == CHUNK_SIZE - 1) { auto c = GetOrCreateChunk(cx+1, cz); c->state = ChunkState::Generated; }
    if (lz == 0)              { auto c = GetOrCreateChunk(cx, cz-1); c->state = ChunkState::Generated; }
    if (lz == CHUNK_SIZE - 1) { auto c = GetOrCreateChunk(cx, cz+1); c->state = ChunkState::Generated; }
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

    std::unique_lock lock(chunksMutex);
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