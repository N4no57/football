#include "maths.h"

// Vector with components value 0.0f
Vector2 Vector2Zero(void) {
    Vector2 result = { 0.0f, 0.0f };

    return result;
}

// Vector with components value 1.0f
Vector2 Vector2One(void) {
    Vector2 result = { 1.0f, 1.0f };

    return result;
}

// Add two vectors (v1 + v2)
Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    Vector2 result = { v1.x + v2.x, v1.y + v2.y };

    return result;
}


// Add vector and float value
Vector2 Vector2AddValue(Vector2 v, float add) {
    Vector2 result = { v.x + add, v.y + add };

    return result;
}

// Subtract two vectors (v1 - v2)
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    Vector2 result = { v1.x - v2.x, v1.y - v2.y };

    return result;
}

// Subtract vector by float value
Vector2 Vector2SubtractValue(Vector2 v, float sub) {
    Vector2 result = { v.x - sub, v.y - sub };

    return result;
}

// Calculate vector length
float Vector2Length(Vector2 v) {
    float result = sqrtf((v.x*v.x) + (v.y*v.y));

    return result;
}

// Calculate vector square length
float Vector2LengthSqr(Vector2 v) {
    float result = (v.x*v.x) + (v.y*v.y);

    return result;
}

// Calculate two vectors dot product
float Vector2DotProduct(Vector2 v1, Vector2 v2) {
    float result = (v1.x*v2.x + v1.y*v2.y);

    return result;
}

// Calculate two vectors cross product
float Vector2CrossProduct(Vector2 v1, Vector2 v2) {
    float result = (v1.x*v2.y - v1.y*v2.x);

    return result;
}

// Calculate distance between two vectors
float Vector2Distance(Vector2 v1, Vector2 v2) {
    float result = sqrtf((v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y));

    return result;
}

// Calculate square distance between two vectors
float Vector2DistanceSqr(Vector2 v1, Vector2 v2) {
    float result = ((v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y));

    return result;
}

// Calculate the signed angle from v1 to v2, relative to the origin (0, 0)
// NOTE: Coordinate system convention: positive X right, positive Y down
// positive angles appear clockwise, and negative angles appear counterclockwise
float Vector2Angle(Vector2 v1, Vector2 v2) {
    float result = 0.0f;

    float dot = v1.x*v2.x + v1.y*v2.y;
    float det = v1.x*v2.y - v1.y*v2.x;

    result = atan2f(det, dot);

    return result;
}

// Calculate angle defined by a two vectors line
// NOTE: Parameters need to be normalized
// Current implementation should be aligned with glm::angle
float Vector2LineAngle(Vector2 start, Vector2 end) {
    float result = 0.0f;

    // TODO(10/9/2023): Currently angles move clockwise, determine if this is wanted behavior
    result = -atan2f(end.y - start.y, end.x - start.x);

    return result;
}

// Scale vector (multiply by value)
Vector2 Vector2Scale(Vector2 v, float scale) {
    Vector2 result = { v.x*scale, v.y*scale };

    return result;
}

// Multiply vector by vector
Vector2 Vector2Multiply(Vector2 v1, Vector2 v2) {
    Vector2 result = { v1.x*v2.x, v1.y*v2.y };

    return result;
}

// Negate vector
Vector2 Vector2Negate(Vector2 v) {
    Vector2 result = { -v.x, -v.y };

    return result;
}

// Divide vector by vector
Vector2 Vector2Divide(Vector2 v1, Vector2 v2) {
    Vector2 result = { v1.x/v2.x, v1.y/v2.y };

    return result;
}

// Normalize provided vector
Vector2 Vector2Normalize(Vector2 v) {
    Vector2 result = { 0 };
    float length = sqrtf((v.x*v.x) + (v.y*v.y));

    if (length > 0)
    {
        float ilength = 1.0f/length;
        result.x = v.x*ilength;
        result.y = v.y*ilength;
    }

    return result;
}

// Transforms a Vector2 by a given Matrix
Vector2 Vector2Transform(Vector2 v, Matrix mat) {
    Vector2 result = { 0 };

    float x = v.x;
    float y = v.y;
    float z = 0;

    result.x = mat.m0*x + mat.m4*y + mat.m8*z + mat.m12;
    result.y = mat.m1*x + mat.m5*y + mat.m9*z + mat.m13;

    return result;
}

// Calculate linear interpolation between two vectors
Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, float amount) {
    Vector2 result = { 0 };

    result.x = v1.x + amount*(v2.x - v1.x);
    result.y = v1.y + amount*(v2.y - v1.y);

    return result;
}

// Calculate reflected vector to normal
Vector2 Vector2Reflect(Vector2 v, Vector2 normal) {
    Vector2 result = { 0 };

    float dotProduct = (v.x*normal.x + v.y*normal.y); // Dot product

    result.x = v.x - (2.0f*normal.x)*dotProduct;
    result.y = v.y - (2.0f*normal.y)*dotProduct;

    return result;
}

// Get min value for each pair of components
Vector2 Vector2Min(Vector2 v1, Vector2 v2) {
    Vector2 result = { 0 };

    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);

    return result;
}

// Get max value for each pair of components
Vector2 Vector2Max(Vector2 v1, Vector2 v2) {
    Vector2 result = { 0 };

    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);

    return result;
}

// Rotate vector by angle
Vector2 Vector2Rotate(Vector2 v, float angle) {
    Vector2 result = { 0 };

    float cosres = cosf(angle);
    float sinres = sinf(angle);

    result.x = v.x*cosres - v.y*sinres;
    result.y = v.x*sinres + v.y*cosres;

    return result;
}

// Move Vector towards target
Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, float maxDistance) {
    Vector2 result = { 0 };

    float dx = target.x - v.x;
    float dy = target.y - v.y;
    float value = (dx*dx) + (dy*dy);

    if ((value == 0) || ((maxDistance >= 0) && (value <= maxDistance*maxDistance))) return target;

    float dist = sqrtf(value);

    result.x = v.x + dx/dist*maxDistance;
    result.y = v.y + dy/dist*maxDistance;

    return result;
}

// Invert the given vector
Vector2 Vector2Invert(Vector2 v) {
    Vector2 result = { 1.0f/v.x, 1.0f/v.y };

    return result;
}

// Clamp the components of the vector between
// min and max values specified by the given vectors
Vector2 Vector2Clamp(Vector2 v, Vector2 min, Vector2 max) {
    Vector2 result = { 0 };

    result.x = fminf(max.x, fmaxf(min.x, v.x));
    result.y = fminf(max.y, fmaxf(min.y, v.y));

    return result;
}

// Clamp the magnitude of the vector between two min and max values
Vector2 Vector2ClampValue(Vector2 v, float min, float max) {
    Vector2 result = v;

    float length = (v.x*v.x) + (v.y*v.y);
    if (length > 0.0f)
    {
        length = sqrtf(length);

        float scale = 1; // By default, 1 as the neutral element
        if (length < min) scale = min/length;
        else if (length > max) scale = max/length;

        result.x = v.x*scale;
        result.y = v.y*scale;
    }

    return result;
}

// Check whether two given vectors are almost equal
int Vector2Equals(Vector2 p, Vector2 q) {
#if !defined(EPSILON)
    #define EPSILON 0.000001f
#endif

    int result = ((fabsf(p.x - q.x)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(p.x), fabsf(q.x))))) &&
                  ((fabsf(p.y - q.y)) <= (EPSILON*fmaxf(1.0f, fmaxf(fabsf(p.y), fabsf(q.y)))));

    return result;
}

// Compute the direction of a refracted ray
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes
// to the refractive index of the medium on the other side of the surface
Vector2 Vector2Refract(Vector2 v, Vector2 n, float r) {
    Vector2 result = { 0 };

    float dot = v.x*n.x + v.y*n.y;
    float d = 1.0f - r*r*(1.0f - dot*dot);

    if (d >= 0.0f)
    {
        d = sqrtf(d);
        v.x = r*v.x - (r*dot + d)*n.x;
        v.y = r*v.y - (r*dot + d)*n.y;

        result = v;
    }

    return result;
}