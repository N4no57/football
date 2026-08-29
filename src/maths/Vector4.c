#include "maths.h"

Vector4 Vector4Zero(void)
{
    Vector4 result = { 0.0f, 0.0f, 0.0f, 0.0f };
    return result;
}

Vector4 Vector4One(void)
{
    Vector4 result = { 1.0f, 1.0f, 1.0f, 1.0f };
    return result;
}

Vector4 Vector4Add(Vector4 v1, Vector4 v2)
{
    Vector4 result = {
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z,
        v1.w + v2.w
    };
    return result;
}

Vector4 Vector4AddValue(Vector4 v, float add)
{
    Vector4 result = {
        v.x + add,
        v.y + add,
        v.z + add,
        v.w + add
    };
    return result;
}

Vector4 Vector4Subtract(Vector4 v1, Vector4 v2)
{
    Vector4 result = {
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z,
        v1.w - v2.w
    };
    return result;
}

Vector4 Vector4SubtractValue(Vector4 v, float add)
{
    Vector4 result = {
        v.x - add,
        v.y - add,
        v.z - add,
        v.w - add
    };
    return result;
}

float Vector4Length(Vector4 v)
{
    float result = sqrtf((v.x*v.x) + (v.y*v.y) + (v.z*v.z) + (v.w*v.w));
    return result;
}

float Vector4LengthSqr(Vector4 v)
{
    float result = (v.x*v.x) + (v.y*v.y) + (v.z*v.z) + (v.w*v.w);
    return result;
}

float Vector4DotProduct(Vector4 v1, Vector4 v2)
{
    float result = (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w);
    return result;
}

float Vector4Distance(Vector4 v1, Vector4 v2)
{
    float result = sqrtf(
        (v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y) +
        (v1.z - v2.z)*(v1.z - v2.z) + (v1.w - v2.w)*(v1.w - v2.w));
    return result;
}

float Vector4DistanceSqr(Vector4 v1, Vector4 v2)
{
    float result =
        (v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y) +
        (v1.z - v2.z)*(v1.z - v2.z) + (v1.w - v2.w)*(v1.w - v2.w);

    return result;
}

Vector4 Vector4Scale(Vector4 v, float scale)
{
    Vector4 result = { v.x*scale, v.y*scale, v.z*scale, v.w*scale };
    return result;
}

Vector4 Vector4Multiply(Vector4 v1, Vector4 v2)
{
    Vector4 result = { v1.x*v2.x, v1.y*v2.y, v1.z*v2.z, v1.w*v2.w };
    return result;
}

Vector4 Vector4Negate(Vector4 v)
{
    Vector4 result = { -v.x, -v.y, -v.z, -v.w };
    return result;
}

Vector4 Vector4Divide(Vector4 v1, Vector4 v2)
{
    Vector4 result = { v1.x/v2.x, v1.y/v2.y, v1.z/v2.z, v1.w/v2.w };
    return result;
}

Vector4 Vector4Normalize(Vector4 v)
{
    Vector4 result = { 0 };
    float length = sqrtf((v.x*v.x) + (v.y*v.y) + (v.z*v.z) + (v.w*v.w));

    if (length > 0)
    {
        float ilength = 1.0f/length;
        result.x = v.x*ilength;
        result.y = v.y*ilength;
        result.z = v.z*ilength;
        result.w = v.w*ilength;
    }

    return result;
}

Vector4 Vector4Min(Vector4 v1, Vector4 v2)
{
    Vector4 result = { 0 };

    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    result.w = fminf(v1.w, v2.w);

    return result;
}

Vector4 Vector4Max(Vector4 v1, Vector4 v2)
{
    Vector4 result = { 0 };

    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    result.w = fmaxf(v1.w, v2.w);

    return result;
}

Vector4 Vector4Lerp(Vector4 v1, Vector4 v2, float amount)
{
    Vector4 result = { 0 };

    result.x = v1.x + amount*(v2.x - v1.x);
    result.y = v1.y + amount*(v2.y - v1.y);
    result.z = v1.z + amount*(v2.z - v1.z);
    result.w = v1.w + amount*(v2.w - v1.w);

    return result;
}

Vector4 Vector4MoveTowards(Vector4 v, Vector4 target, float maxDistance)
{
    Vector4 result = { 0 };

    float dx = target.x - v.x;
    float dy = target.y - v.y;
    float dz = target.z - v.z;
    float dw = target.w - v.w;
    float value = (dx*dx) + (dy*dy) + (dz*dz) + (dw*dw);

    if ((value == 0) || ((maxDistance >= 0) && (value <= maxDistance*maxDistance))) return target;

    float dist = sqrtf(value);

    result.x = v.x + dx/dist*maxDistance;
    result.y = v.y + dy/dist*maxDistance;
    result.z = v.z + dz/dist*maxDistance;
    result.w = v.w + dw/dist*maxDistance;

    return result;
}

Vector4 Vector4Invert(Vector4 v)
{
    Vector4 result = { 1.0f/v.x, 1.0f/v.y, 1.0f/v.z, 1.0f/v.w };
    return result;
}

int Vector4Equals(Vector4 p, Vector4 q)
{
#if !defined(EPSILON)
    #define EPSILON 0.000001f
#endif

    int result = ((fabsf(p.x - q.x)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(p.x), fabsf(q.x))))) &&
                  ((fabsf(p.y - q.y)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(p.y), fabsf(q.y))))) &&
                  ((fabsf(p.z - q.z)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(p.z), fabsf(q.z))))) &&
                  ((fabsf(p.w - q.w)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(p.w), fabsf(q.w)))));
    return result;
}