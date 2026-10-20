#include "chunk.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include "texture_atlas.h"
#include <cmath>
#include <cstring>

Chunk::Chunk(int cx, int cz) : cx(cx), cz(cz), state(ChunkState::Empty) {
    memset(blocks, 0, sizeof(blocks));
}

Chunk::~Chunk() {
    UnloadMesh();
}

void Chunk::GenerateData() {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = cx * CHUNK_SIZE + x;
            int worldZ = cz * CHUNK_SIZE + z;

            float noise = Utils::Noise2D(worldX * TERRAIN_FREQUENCY, worldZ * TERRAIN_FREQUENCY);
            int height = static_cast<int>(TERRAIN_BASE_HEIGHT + noise * TERRAIN_AMPLITUDE);

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y > height) {
                    blocks[x][y][z] = BlockType::Air;
                } else if (y == height) {
                    blocks[x][y][z] = (height < 18) ? BlockType::Sand : BlockType::Grass;
                } else if (y > height - STONE_LAYER_DEPTH) {
                    blocks[x][y][z] = BlockType::Dirt;
                } else {
                    blocks[x][y][z] = BlockType::Stone;
                }
            }
        }
    }
}

BlockType Chunk::GetBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return BlockType::Air;
    return blocks[x][y][z];
}

void Chunk::SetBlock(int x, int y, int z, BlockType type) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return;
    blocks[x][y][z] = type;
    // Если блок сломали, нужно перестроить меш
    state = ChunkState::Generated; 
}

void Chunk::BuildMeshData() {
    pendingVertices.clear();
    pendingTexcoords.clear();
    pendingColors.clear();
    pendingIndices.clear();

    unsigned short vertexIndex = 0;
    const int dx[6] = { 1, -1, 0, 0, 0, 0 };
    const int dy[6] = { 0, 0, 1, -1, 0, 0 };
    const int dz[6] = { 0, 0, 0, 0, 1, -1 };
    const unsigned char faceBrightness[6] = { 180, 180, 255, 100, 180, 180 };

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                BlockType current = blocks[x][y][z];
                if (current == BlockType::Air) continue;

                const BlockInfo& info = BlockRegistry::Get(current);

                for (int f = 0; f < 6; f++) {
                    int nx = x + dx[f];
                    int ny = y + dy[f];
                    int nz = z + dz[f];

                    bool neighborSolid = false;
                    if (nx >= 0 && nx < CHUNK_SIZE && ny >= 0 && ny < CHUNK_HEIGHT && nz >= 0 && nz < CHUNK_SIZE) {
                        neighborSolid = (blocks[nx][ny][nz] != BlockType::Air);
                    } else {
                        // Внимание: вызов World::GetBlock из потока! Мы обезопасим его мьютексом в world.cpp
                        BlockType neighbor = World::GetBlock(cx * CHUNK_SIZE + nx, ny, cz * CHUNK_SIZE + nz);
                        neighborSolid = (neighbor != BlockType::Air);
                    }

                    if (neighborSolid) continue;

                    int texIndex = info.texSide;
                    if (f == 2) texIndex = info.texTop;    
                    if (f == 3) texIndex = info.texBottom; 

                    Rectangle uv = TextureAtlas::GetUV(texIndex);
                    float inset = 0.005f; 
                    uv.x += inset; uv.y += inset;
                    uv.width -= inset * 2.0f; uv.height -= inset * 2.0f;

                    float fx = static_cast<float>(x);
                    float fy = static_cast<float>(y);
                    float fz = static_cast<float>(z);
                    float verts[4][3];
                    float uvs[4][2];

                    switch (f) {
                        case 0: // +X
                            verts[0][0] = fx + 1.0f; verts[0][1] = fy;        verts[0][2] = fz;
                            verts[1][0] = fx + 1.0f; verts[1][1] = fy + 1.0f; verts[1][2] = fz;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy + 1.0f; verts[2][2] = fz + 1.0f;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy;        verts[3][2] = fz + 1.0f;
                            uvs[0][0] = uv.x; uvs[0][1] = uv.y + uv.height;
                            uvs[1][0] = uv.x; uvs[1][1] = uv.y;
                            uvs[2][0] = uv.x + uv.width; uvs[2][1] = uv.y;
                            uvs[3][0] = uv.x + uv.width; uvs[3][1] = uv.y + uv.height;
                            break;
                        case 1: // -X
                            verts[0][0] = fx; verts[0][1] = fy;        verts[0][2] = fz + 1.0f;
                            verts[1][0] = fx; verts[1][1] = fy + 1.0f; verts[1][2] = fz + 1.0f;
                            verts[2][0] = fx; verts[2][1] = fy + 1.0f; verts[2][2] = fz;
                            verts[3][0] = fx; verts[3][1] = fy;        verts[3][2] = fz;
                            uvs[0][0] = uv.x + uv.width; uvs[0][1] = uv.y + uv.height;
                            uvs[1][0] = uv.x + uv.width; uvs[1][1] = uv.y;
                            uvs[2][0] = uv.x; uvs[2][1] = uv.y;
                            uvs[3][0] = uv.x; uvs[3][1] = uv.y + uv.height;
                            break;
                        case 2: // +Y
                            verts[0][0] = fx; verts[0][1] = fy + 1.0f; verts[0][2] = fz;
                            verts[1][0] = fx; verts[1][1] = fy + 1.0f; verts[1][2] = fz + 1.0f;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy + 1.0f; verts[2][2] = fz + 1.0f;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy + 1.0f; verts[3][2] = fz;
                            uvs[0][0] = uv.x; uvs[0][1] = uv.y + uv.height;
                            uvs[1][0] = uv.x + uv.width; uvs[1][1] = uv.y + uv.height;
                            uvs[2][0] = uv.x + uv.width; uvs[2][1] = uv.y;
                            uvs[3][0] = uv.x; uvs[3][1] = uv.y;
                            break;
                        case 3: // -Y
                            verts[0][0] = fx; verts[0][1] = fy; verts[0][2] = fz + 1.0f;
                            verts[1][0] = fx; verts[1][1] = fy; verts[1][2] = fz;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy; verts[2][2] = fz;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy; verts[3][2] = fz + 1.0f;
                            uvs[0][0] = uv.x; uvs[0][1] = uv.y + uv.height;
                            uvs[1][0] = uv.x + uv.width; uvs[1][1] = uv.y + uv.height;
                            uvs[2][0] = uv.x + uv.width; uvs[2][1] = uv.y;
                            uvs[3][0] = uv.x; uvs[3][1] = uv.y;
                            break;
                        case 4: // +Z
                            verts[0][0] = fx + 1.0f; verts[0][1] = fy; verts[0][2] = fz + 1.0f;
                            verts[1][0] = fx + 1.0f; verts[1][1] = fy + 1.0f; verts[1][2] = fz + 1.0f;
                            verts[2][0] = fx; verts[2][1] = fy + 1.0f; verts[2][2] = fz + 1.0f;
                            verts[3][0] = fx; verts[3][1] = fy; verts[3][2] = fz + 1.0f;
                            uvs[0][0] = uv.x; uvs[0][1] = uv.y + uv.height;
                            uvs[1][0] = uv.x; uvs[1][1] = uv.y;
                            uvs[2][0] = uv.x + uv.width; uvs[2][1] = uv.y;
                            uvs[3][0] = uv.x + uv.width; uvs[3][1] = uv.y + uv.height;
                            break;
                        case 5: // -Z
                            verts[0][0] = fx; verts[0][1] = fy; verts[0][2] = fz;
                            verts[1][0] = fx; verts[1][1] = fy + 1.0f; verts[1][2] = fz;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy + 1.0f; verts[2][2] = fz;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy; verts[3][2] = fz;
                            uvs[0][0] = uv.x + uv.width; uvs[0][1] = uv.y + uv.height;
                            uvs[1][0] = uv.x + uv.width; uvs[1][1] = uv.y;
                            uvs[2][0] = uv.x; uvs[2][1] = uv.y;
                            uvs[3][0] = uv.x; uvs[3][1] = uv.y + uv.height;
                            break;
                    }

                    for (int i = 0; i < 4; i++) {
                        pendingVertices.push_back(verts[i][0]);
                        pendingVertices.push_back(verts[i][1]);
                        pendingVertices.push_back(verts[i][2]);
                        unsigned char shade = faceBrightness[f];
                        pendingColors.push_back(shade);
                        pendingColors.push_back(shade);
                        pendingColors.push_back(shade);
                        pendingColors.push_back(255);
                        pendingTexcoords.push_back(uvs[i][0]);
                        pendingTexcoords.push_back(uvs[i][1]);
                    }

                    pendingIndices.push_back(vertexIndex);
                    pendingIndices.push_back(vertexIndex + 1);
                    pendingIndices.push_back(vertexIndex + 2);
                    pendingIndices.push_back(vertexIndex);
                    pendingIndices.push_back(vertexIndex + 2);
                    pendingIndices.push_back(vertexIndex + 3);
                    vertexIndex += 4;
                }
            }
        }
    }
}

// ЭТОТ МЕТОД ВЫЗЫВАЕТСЯ ТОЛЬКО В ГЛАВНОМ ПОТОКЕ
void Chunk::UploadToGPU() {
    UnloadMesh();

    if (pendingVertices.empty()) {
        meshBuilt = false;
        state = ChunkState::Loaded;
        return;
    }

    mesh.vertexCount = static_cast<int>(pendingVertices.size() / 3);
    mesh.triangleCount = static_cast<int>(pendingIndices.size() / 3);
    
    // Выделяем память видеокарты и копируем данные
    mesh.vertices = (float*)MemAlloc(pendingVertices.size() * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(pendingTexcoords.size() * sizeof(float));
    mesh.colors = (unsigned char*)MemAlloc(pendingColors.size() * sizeof(unsigned char));
    mesh.indices = (unsigned short*)MemAlloc(pendingIndices.size() * sizeof(unsigned short));

    memcpy(mesh.vertices, pendingVertices.data(), pendingVertices.size() * sizeof(float));
    memcpy(mesh.texcoords, pendingTexcoords.data(), pendingTexcoords.size() * sizeof(float));
    memcpy(mesh.colors, pendingColors.data(), pendingColors.size() * sizeof(unsigned char));
    memcpy(mesh.indices, pendingIndices.data(), pendingIndices.size() * sizeof(unsigned short));

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = TextureAtlas::GetTexture();
    
    meshBuilt = true;
    state = ChunkState::Loaded;
}

void Chunk::Draw() const {
    if (meshBuilt && mesh.vertexCount > 0) {
        Vector3 worldPos = { 
            static_cast<float>(cx * CHUNK_SIZE), 
            0.0f, 
            static_cast<float>(cz * CHUNK_SIZE) 
        };
        DrawModel(model, worldPos, 1.0f, WHITE);
    }
}

void Chunk::UnloadMesh() {
    if (meshBuilt) {
        UnloadModel(model);
        meshBuilt = false;
    }
    mesh = { 0 }; 
}