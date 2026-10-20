#include "block.h"
#include "config.h"
#include "raylib.h"

BlockInfo BlockRegistry::blocks[static_cast<int>(BlockType::Count)];

void BlockRegistry::Init() {
    // Процедурные цвета для каждого типа блока (в реальном проекте можно загрузить атлас)
    blocks[static_cast<int>(BlockType::Air)]   = {"Air",    BLANK,                         false, {0, 0, 0, 0}};
    blocks[static_cast<int>(BlockType::Grass)] = {"Grass",  { 90, 155,  70, 255}, true,  {0, 0, 16, 16}};
    blocks[static_cast<int>(BlockType::Dirt)]  = {"Dirt",   {134,  96,  67, 255}, true,  {16, 0, 16, 16}};
    blocks[static_cast<int>(BlockType::Stone)] = {"Stone",  {128, 128, 128, 255}, true,  {32, 0, 16, 16}};
    blocks[static_cast<int>(BlockType::Sand)]  = {"Sand",   {210, 190, 130, 255}, true,  {48, 0, 16, 16}};
    blocks[static_cast<int>(BlockType::Wood)]  = {"Wood",   {102,  67,  42, 255}, true,  {64, 0, 16, 16}};
    blocks[static_cast<int>(BlockType::Leaves)]= {"Leaves", { 60, 120,  50, 255}, true,  {80, 0, 16, 16}};
}

const BlockInfo& BlockRegistry::Get(BlockType type) {
    return blocks[static_cast<int>(type)];
}

BlockType BlockRegistry::FromIndex(int index) {
    if (index < 0 || index >= static_cast<int>(BlockType::Count)) return BlockType::Air;
    return static_cast<BlockType>(index);
}

int BlockRegistry::ToIndex(BlockType type) {
    return static_cast<int>(type);
}