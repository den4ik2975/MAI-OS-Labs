#include <cmath>

extern "C" float SinIntegral(float A, float B, float e) {
    int n = (int)((B - A) / e);
    float result = sin(A) / 2.0f;

    for(int i = 1; i < n; i++) {
        float x = A + i * e;
        result += sin(x);
    }

    result += sin(B) / 2.0f;
    result *= e;

    return result;
}
