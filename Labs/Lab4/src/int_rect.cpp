#include <cmath>

extern "C" float SinIntegral(float A, float B, float e) {
    float result = 0.0f;

    int steps = static_cast<int>((B - A) / e);

    for (int i = 0; i < steps; i++) {
        float x = A + i * e + e / 2.0f;
        result += sin(x) * e;
    }

    return result;
}
