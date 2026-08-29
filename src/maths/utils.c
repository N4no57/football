#include "maths.h"

// Clamp float value
float Clamp(float value, float min, float max)
{
    float result = (value < min)? min : value;

    if (result > max) result = max;

    return result;
}

// Calculate linear interpolation between two floats
float Lerp(float start, float end, float amount)
{
    float result = start + amount*(end - start);

    return result;
}

// Normalize input value within input range
float Normalize(float value, float start, float end)
{
    float result = (value - start)/(end - start);

    return result;
}

// Remap input value within input range to output range
float Remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd)
{
    float result = (value - inputStart)/(inputEnd - inputStart)*(outputEnd - outputStart) + outputStart;

    return result;
}

// Wrap input value from min to max
float Wrap(float value, float min, float max)
{
    float result = value - (max - min)*floorf((value - min)/(max - min));

    return result;
}

// Check whether two given floats are almost equal
int FloatEquals(float x, float y)
{
#if !defined(EPSILON)
#define EPSILON 0.000001f
#endif

    int result = (fabsf(x - y)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))));

    return result;
}