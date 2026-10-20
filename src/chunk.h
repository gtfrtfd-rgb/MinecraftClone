#pragma once
#include "raylib.h"
#include "block.h"
#include "config.h"
#include <vector>
#include <atomic>
#include <memory>

enum class ChunkState {
    Empty,       // Только создан
    Generating,  // Поток генерирует блоки
    Generated,   // Блоки готовы, нужен меш
    Meshing,     // Поток строит меш
    MeshReady,   // Меш готов, ждет отправки на GPU
    Loaded       // Меш на GPU, чанк отрисовывается
};

class Chunk {
public:
    Chunk(int cx, int cz);
    ~Chunk();

    void GenerateData();  // Вызывается в фоновом потоке
    void BuildMeshData(); // Вызывается в фоновом потоке
    void UploadToGPU();   // Вызывается ТОЛЬКО в главном потоке
    void Draw() const;

    BlockType GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, BlockType type);

    std::atomic<ChunkState> state;
    
    // Буферы для передачи данных из потока в главный поток
    std::vector<float> pendingVertices;
    std::vector<float> pendingTexcoords;
    std::vector<unsigned char> pendingColors;
    std::vector<unsigned short> pendingIndices;

    int GetCX() const { return cx; }
    int GetCZ() const { return cz; }

private:
    int cx, cz;
    BlockType blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    Mesh mesh = {0};
    Model model = {0};
    bool meshBuilt = false;

    void UnloadMesh();
};