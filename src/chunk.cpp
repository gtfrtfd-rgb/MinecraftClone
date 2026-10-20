#include "chunk.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include <cmath>
#include <cstring>

Chunk::Chunk(int cx, int cz) : cx(cx), cz(cz) {
    memset(blocks, 0, sizeof(blocks));
}

Chunk::~Chunk() {
    UnloadMesh();
}

void Chunk::Generate() {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = cx * CHUNK_SIZE + x;
            int worldZ = cz * CHUNK_SIZE + z;

            float noise = Utils::Noise2D(worldX * TERRAIN_FREQUENCY,
                                          worldZ * TERRAIN_FREQUENCY);
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
    dirty = true;
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
    dirty = true;
}

void Chunk::BuildMesh() {
    if (!dirty) return;
    UnloadMesh();

    std::vector<float> vertices;
    std::vector<float> texcoords;
    std::vector<unsigned char> colors;
    std::vector<unsigned short> indices;
    unsigned short vertexIndex = 0;

    const int dx[6] = { 1, -1, 0, 0, 0, 0 };
    const int dy[6] = { 0, 0, 1, -1, 0, 0 };
    const int dz[6] = { 0, 0, 0, 0, 1, -1 };

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                BlockType current = blocks[x][y][z];
                if (current == BlockType::Air) continue;

                const BlockInfo& info = BlockRegistry::Get(current);
                Color col = info.color;

                for (int f = 0; f < 6; f++) {
                    int nx = x + dx[f];
                    int ny = y + dy[f];
                    int nz = z + dz[f];

                    bool neighborSolid = false;
                    if (nx >= 0 && nx < CHUNK_SIZE && ny >= 0 && ny < CHUNK_HEIGHT && nz >= 0 && nz < CHUNK_SIZE) {
                        neighborSolid = (blocks[nx][ny][nz] != BlockType::Air);
                    } else {
                        BlockType neighbor = World::GetBlock(cx * CHUNK_SIZE + nx, ny, cz * CHUNK_SIZE + nz);
                        neighborSolid = (neighbor != BlockType::Air);
                    }

                    if (neighborSolid) continue;

                    float fx = static_cast<float>(x);
                    float fy = static_cast<float>(y);
                    float fz = static_cast<float>(z);
                    float verts[4][3];

                    switch (f) {
                        case 0: // +X
                            verts[0][0] = fx + 1.0f; verts[0][1] = fy;        verts[0][2] = fz;
                            verts[1][0] = fx + 1.0f; verts[1][1] = fy + 1.0f; verts[1][2] = fz;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy + 1.0f; verts[2][2] = fz + 1.0f;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy;        verts[3][2] = fz + 1.0f;
                            break;
                        case 1: // -X
                            verts[0][0] = fx;        verts[0][1] = fy;        verts[0][2] = fz + 1.0f;
                            verts[1][0] = fx;        verts[1][1] = fy + 1.0f; verts[1][2] = fz + 1.0f;
                            verts[2][0] = fx;        verts[2][1] = fy + 1.0f; verts[2][2] = fz;
                            verts[3][0] = fx;        verts[3][1] = fy;        verts[3][2] = fz;
                            break;
                        case 2: // +Y
                            verts[0][0] = fx;        verts[0][1] = fy + 1.0f; verts[0][2] = fz;
                            verts[1][0] = fx;        verts[1][1] = fy + 1.0f; verts[1][2] = fz + 1.0f;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy + 1.0f; verts[2][2] = fz + 1.0f;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy + 1.0f; verts[3][2] = fz;
                            break;
                        case 3: // -Y
                            verts[0][0] = fx;        verts[0][1] = fy;        verts[0][2] = fz + 1.0f;
                            verts[1][0] = fx;        verts[1][1] = fy;        verts[1][2] = fz;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy;        verts[2][2] = fz;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy;        verts[3][2] = fz + 1.0f;
                            break;
                        case 4: // +Z
                            verts[0][0] = fx + 1.0f; verts[0][1] = fy;        verts[0][2] = fz + 1.0f;
                            verts[1][0] = fx + 1.0f; verts[1][1] = fy + 1.0f; verts[1][2] = fz + 1.0f;
                            verts[2][0] = fx;        verts[2][1] = fy + 1.0f; verts[2][2] = fz + 1.0f;
                            verts[3][0] = fx;        verts[3][1] = fy;        verts[3][2] = fz + 1.0f;
                            break;
                        case 5: // -Z
                            verts[0][0] = fx;        verts[0][1] = fy;        verts[0][2] = fz;
                            verts[1][0] = fx;        verts[1][1] = fy + 1.0f; verts[1][2] = fz;
                            verts[2][0] = fx + 1.0f; verts[2][1] = fy + 1.0f; verts[2][2] = fz;
                            verts[3][0] = fx + 1.0f; verts[3][1] = fy;        verts[3][2] = fz;
                            break;
                    }

                    for (int i = 0; i < 4; i++) {
                        vertices.push_back(verts[i][0]);
                        vertices.push_back(verts[i][1]);
                        vertices.push_back(verts[i][2]);
                        colors.push_back(col.r);
                        colors.push_back(col.g);
                        colors.push_back(col.b);
                        colors.push_back(col.a);
                        texcoords.push_back(0.0f);
                        texcoords.push_back(0.0f);
                    }

                    indices.push_back(vertexIndex);
                    indices.push_back(vertexIndex + 1);
                    indices.push_back(vertexIndex + 2);
                    indices.push_back(vertexIndex);
                    indices.push_back(vertexIndex + 2);
                    indices.push_back(vertexIndex + 3);
                    vertexIndex += 4;
                }
            }
        }
    }

    if (vertices.empty()) {
        dirty = false;
        return;
    }

    mesh.vertexCount = static_cast<int>(vertices.size() / 3);
    mesh.triangleCount = static_cast<int>(indices.size() / 3);
    mesh.vertices = (float*)MemAlloc(vertices.size() * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(texcoords.size() * sizeof(float));
    mesh.colors = (unsigned char*)MemAlloc(colors.size() * sizeof(unsigned char));
    mesh.indices = (unsigned short*)MemAlloc(indices.size() * sizeof(unsigned short));

    memcpy(mesh.vertices, vertices.data(), vertices.size() * sizeof(float));
    memcpy(mesh.texcoords, texcoords.data(), texcoords.size() * sizeof(float));
    memcpy(mesh.colors, colors.data(), colors.size() * sizeof(unsigned char));
    memcpy(mesh.indices, indices.data(), indices.size() * sizeof(unsigned short));

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);
    meshBuilt = true;
    dirty = false;
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
    // ⬇️ КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ ⬇️
    // Полностью обнуляем структуру меша. 
    // Это уберет Warning в консоли и предотвратит вылеты при выходе.
    mesh = { 0 }; 
}