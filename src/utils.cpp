#include "utils.h"
#include <cmath>

namespace Utils {

static int Hash(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
}

static float Fade(float t) { 
    return t * t * t * (t * (t * 6 - 15) + 10); 
}

static float Lerp(float a, float b, float t) { 
    return a + t * (b - a); 
}

static float Grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float Noise2D(float x, float y) {
    int xi = static_cast<int>(floorf(x)) & 255;
    int yi = static_cast<int>(floorf(y)) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);
    
    float u = Fade(xf);
    float v = Fade(yf);
    
    int aa = Hash(xi, yi);
    int ab = Hash(xi, yi + 1);
    int ba = Hash(xi + 1, yi);
    int bb = Hash(xi + 1, yi + 1);
    
    float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1, yf), u);
    float x2 = Lerp(Grad(ab, xf, yf - 1), Grad(bb, xf - 1, yf - 1), u);
    
    return Lerp(x1, x2, v);
}

} // namespace Utils