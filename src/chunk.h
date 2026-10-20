#pragma once

#include "raylib.h"
#include "block.h"
#include "config.h"
#include <vector>

class Chunk {
public:
    Chunk(int cx, int cz);
    ~Chunk();

    void Generate();
    void BuildMesh();
    void Draw() const;

    BlockType GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, BlockType type);

    bool IsDirty() const { return dirty; }
    void MarkDirty() { dirty = true; }
    void ClearDirty() { dirty = false; }

    int GetCX() const { return cx; }
    int GetCZ() const { return cz; }

private:
    int cx, cz;
    BlockType blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    Mesh mesh = {0};
    Model model = {0};
    bool dirty = true;
    bool meshBuilt = false;

    void UnloadMesh();
};