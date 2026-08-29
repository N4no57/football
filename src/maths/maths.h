/**********************************************************************************************
*
*   raymath v2.0 - Math functions to work with Vector2, Vector3, Vector4, Matrix and Quaternions
*
*   CONVENTIONS:
*     - Matrix structure is defined as row-major (memory layout) but parameters naming AND all
*       math operations performed by the library consider the structure as it was column-major
*       It is like transposed versions of the matrices are used for all the maths
*       It benefits some functions making them cache-friendly and also avoids matrix
*       transpositions sometimes required by OpenGL
*       Example: In memory order, row0 is [m0 m4 m8 m12] but in semantic math row0 is [m0 m1 m2 m3]
*     - Functions are always self-contained, no function use another raymath function inside,
*       required code is directly re-implemented inside
*     - Functions input parameters are always received by value (2 unavoidable exceptions)
*     - Functions use always a "result" variable for return (except C++ operators)
*     - Functions are always defined inline
*     - Angles are always in radians (DEG2RAD/RAD2DEG macros provided for convenience)
*     - No compound literals used to make sure the library is compatible with C++
*
*   CONFIGURATION:
*
*       #define MATHS_DISABLE_CPP_OPERATORS
*           Disables C++ operator overloads for raymath types.
*
*       #define MATHS_USE_SIMD_INTRINSICS   1
*           Try to enable SIMD intrinsics for MatrixMultiply()
*           Note that users enabling it must be aware of the target platform where application will
*           run to support the selected SIMD intrinsic, for now, only SSE is supported
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2015-2026 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef MATHS_H
#define MATHS_H

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#ifndef PI
    #define PI 3.14159265358979323846f
#endif

#ifndef EPSILON
    #define EPSILON 0.000001f
#endif

#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif

#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

// Get float vector for Matrix
#ifndef MatrixToFloat
    #define MatrixToFloat(mat) (MatrixToFloatV(mat).v)
#endif

// Get float vector for Vector3
#ifndef Vector3ToFloat
    #define Vector3ToFloat(vec) (Vector3ToFloatV(vec).v)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Vector2 type
typedef struct Vector2 {
    float x;
    float y;
} Vector2;

// Vector3 type
typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

// Vector4 type
typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;

// Quaternion type
typedef Vector4 Quaternion;

// Matrix type (OpenGL style 4x4 - right handed, column major)
typedef struct Matrix {
    float m0, m1, m2, m3;      // Matrix first row (4 components)
    float m4, m5, m6, m7;      // Matrix second row (4 components)
    float m8, m9, m10, m11;     // Matrix third row (4 components)
    float m12, m13, m14, m15;     // Matrix fourth row (4 components)
} Matrix;

// NOTE: Helper types to be used instead of array return types for *ToFloat functions
typedef struct float3 {
    float v[3];
} float3;

typedef struct float16 {
    float v[16];
} float16;

#include <math.h>       // Required for: sinf(), cosf(), tan(), atan2f(), sqrtf(), floor(), fminf(), fmaxf(), fabsf()

#if MATHS_USE_SIMD_INTRINSICS
    // SIMD is used on the most costly raymath function MatrixMultiply()
    // NOTE: Only SSE intrinsics support implemented
    // TODO: Consider support for other SIMD intrinsics:
    //  - SSEx, AVX, AVX2, FMA, NEON, RVV
    /*
    #if defined(__SSE4_2__)
        #include <nmmintrin.h>
        #define MATHS_SSE42_ENABLED
    #elif defined(__SSE4_1__)
        #include <smmintrin.h>
        #define MATHS_SSE41_ENABLED
    #elif defined(__SSSE3__)
        #include <tmmintrin.h>
        #define MATHS_SSSE3_ENABLED
    #elif defined(__SSE3__)
        #include <pmmintrin.h>
        #define MATHS_SSE3_ENABLED
    #elif defined(__SSE2__) || (defined(_M_AMD64) || defined(_M_X64)) // SSE2 x64
        #include <emmintrin.h>
        #define MATHS_SSE2_ENABLED
    #endif
    */
    #if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 1))
        #include <xmmintrin.h>
        #define MATHS_SSE_ENABLED
    #endif
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition - Utils math
//----------------------------------------------------------------------------------

float Clamp(float value, float min, float max);// Clamp float value
float Lerp(float start, float end, float amount);// Calculate linear interpolation between two floats
float Normalize(float value, float start, float end);// Normalize input value within input range
float Remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd);// Remap input value within input range to output range
float Wrap(float value, float min, float max);// Wrap input value from min to max
int FloatEquals(float x, float y);// Check whether two given floats are almost equal

//----------------------------------------------------------------------------------
// Module Functions Definition - Vector2 math
//----------------------------------------------------------------------------------

Vector2 Vector2Zero(void); // Vector with components value 0.0f
Vector2 Vector2One(void); // Vector with components value 1.0f
Vector2 Vector2Add(Vector2 v1, Vector2 v2); // Add two vectors (v1 + v2)
Vector2 Vector2AddValue(Vector2 v, float add); // Add vector and float value
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2); // Subtract two vectors (v1 - v2)
Vector2 Vector2SubtractValue(Vector2 v, float sub); // Subtract vector by float value
float Vector2Length(Vector2 v); // Calculate vector length
float Vector2LengthSqr(Vector2 v); // Calculate vector square length
float Vector2DotProduct(Vector2 v1, Vector2 v2); // Calculate two vectors dot product
float Vector2CrossProduct(Vector2 v1, Vector2 v2); // Calculate two vectors cross product
float Vector2Distance(Vector2 v1, Vector2 v2); // Calculate distance between two vectors
float Vector2DistanceSqr(Vector2 v1, Vector2 v2); // Calculate square distance between two vectors

// Calculate the signed angle from v1 to v2, relative to the origin (0, 0)
// NOTE: Coordinate system convention: positive X right, positive Y down
// positive angles appear clockwise, and negative angles appear counterclockwise
float Vector2Angle(Vector2 v1, Vector2 v2);

// Calculate angle defined by a two vectors line
// NOTE: Parameters need to be normalized
// Current implementation should be aligned with glm::angle
float Vector2LineAngle(Vector2 start, Vector2 end);
Vector2 Vector2Scale(Vector2 v, float scale); // Scale vector (multiply by value)
Vector2 Vector2Multiply(Vector2 v1, Vector2 v2); // Multiply vector by vector
Vector2 Vector2Negate(Vector2 v); // Negate vector
Vector2 Vector2Divide(Vector2 v1, Vector2 v2); // Divide vector by vector
Vector2 Vector2Normalize(Vector2 v); // Normalize provided vector
Vector2 Vector2Transform(Vector2 v, Matrix mat); // Transforms a Vector2 by a given Matrix
Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, float amount); // Calculate linear interpolation between two vectors
Vector2 Vector2Reflect(Vector2 v, Vector2 normal); // Calculate reflected vector to normal
Vector2 Vector2Min(Vector2 v1, Vector2 v2); // Get min value for each pair of components
Vector2 Vector2Max(Vector2 v1, Vector2 v2);// Get max value for each pair of components
Vector2 Vector2Rotate(Vector2 v, float angle); // Rotate vector by angle
Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, float maxDistance); // Move Vector towards target
Vector2 Vector2Invert(Vector2 v); // Invert the given vector
Vector2 Vector2Clamp(Vector2 v, Vector2 min, Vector2 max); // Clamp the components of the vector between min and max values specified by the given vectors
Vector2 Vector2ClampValue(Vector2 v, float min, float max); // Clamp the magnitude of the vector between two min and max values
int Vector2Equals(Vector2 p, Vector2 q); // Check whether two given vectors are almost equal

// Compute the direction of a refracted ray
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes
// to the refractive index of the medium on the other side of the surface
Vector2 Vector2Refract(Vector2 v, Vector2 n, float r);

//----------------------------------------------------------------------------------
// Module Functions Definition - Vector3 math
//----------------------------------------------------------------------------------

Vector3 Vector3Zero(void); // Vector with components value 0.0f
Vector3 Vector3One(void); // Vector with components value 1.0f
Vector3 Vector3Add(Vector3 v1, Vector3 v2); // Add two vectors
Vector3 Vector3AddValue(Vector3 v, float add); // Add vector and float value
Vector3 Vector3Subtract(Vector3 v1, Vector3 v2); // Subtract two vectors
Vector3 Vector3SubtractValue(Vector3 v, float sub); // Subtract vector by float value
Vector3 Vector3Scale(Vector3 v, float scalar); // Multiply vector by scalar
Vector3 Vector3Multiply(Vector3 v1, Vector3 v2); // Multiply vector by vector
Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2); // Calculate two vectors cross product
Vector3 Vector3Perpendicular(Vector3 v); // Calculate one vector perpendicular vector
float Vector3Length(Vector3 v); // Calculate vector length
float Vector3LengthSqr(Vector3 v); // Calculate vector square length
float Vector3DotProduct(Vector3 v1, Vector3 v2); // Calculate two vectors dot product
float Vector3Distance(Vector3 v1, Vector3 v2); // Calculate distance between two vectors
float Vector3DistanceSqr(Vector3 v1, Vector3 v2); // Calculate square distance between two vectors
float Vector3Angle(Vector3 v1, Vector3 v2); // Calculate angle between two vectors
Vector3 Vector3Negate(Vector3 v); // Negate provided vector (invert direction)
Vector3 Vector3Divide(Vector3 v1, Vector3 v2); // Divide vector by vector
Vector3 Vector3Normalize(Vector3 v); // Normalize provided vector
Vector3 Vector3Project(Vector3 v1, Vector3 v2); //Calculate the projection of the vector v1 on to v2
Vector3 Vector3Reject(Vector3 v1, Vector3 v2); //Calculate the rejection of the vector v1 on to v2

// Orthonormalize provided vectors
// Makes vectors normalized and orthogonal to each other
// Gram-Schmidt function implementation
void Vector3OrthoNormalize(Vector3 *v1, Vector3 *v2);
Vector3 Vector3Transform(Vector3 v, Matrix mat); // Transforms a Vector3 by a given Matrix
Vector3 Vector3RotateByQuaternion(Vector3 v, Quaternion q); // Transform a vector by quaternion rotation
Vector3 Vector3RotateByAxisAngle(Vector3 v, Vector3 axis, float angle); // Rotates a vector around an axis
Vector3 Vector3MoveTowards(Vector3 v, Vector3 target, float maxDistance); // Move Vector towards target
Vector3 Vector3Lerp(Vector3 v1, Vector3 v2, float amount); // Calculate linear interpolation between two vectors

// Calculate cubic hermite interpolation between two vectors and their tangents
// as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic
Vector3 Vector3CubicHermite(Vector3 v1, Vector3 tangent1, Vector3 v2, Vector3 tangent2, float amount);
Vector3 Vector3Reflect(Vector3 v, Vector3 normal); // Calculate reflected vector to normal
Vector3 Vector3Min(Vector3 v1, Vector3 v2); // Get min value for each pair of components
Vector3 Vector3Max(Vector3 v1, Vector3 v2); // Get max value for each pair of components

// Compute barycenter coordinates (u, v, w) for point p with respect to triangle (a, b, c)
// NOTE: Assumes P is on the plane of the triangle
Vector3 Vector3Barycenter(Vector3 p, Vector3 a, Vector3 b, Vector3 c);

// Projects a Vector3 from screen space into object space
// NOTE: Self-contained function, no other raymath functions are called
Vector3 Vector3Unproject(Vector3 source, Matrix projection, Matrix view);
float3 Vector3ToFloatV(Vector3 v); // Get Vector3 as float array
Vector3 Vector3Invert(Vector3 v); // Invert the given vector
Vector3 Vector3Clamp(Vector3 v, Vector3 min, Vector3 max); // Clamp the components of the vector between min and max values specified by the given vectors
Vector3 Vector3ClampValue(Vector3 v, float min, float max); // Clamp the magnitude of the vector between two values
int Vector3Equals(Vector3 p, Vector3 q); // Check whether two given vectors are almost equal

// Compute the direction of a refracted ray
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes
// to the refractive index of the medium on the other side of the surface
Vector3 Vector3Refract(Vector3 v, Vector3 n, float r);


//----------------------------------------------------------------------------------
// Module Functions Definition - Vector4 math
//----------------------------------------------------------------------------------

Vector4 Vector4Zero(void); // Get  vector zero
Vector4 Vector4One(void); // Get vector one
Vector4 Vector4Add(Vector4 v1, Vector4 v2); // Add two vectors
Vector4 Vector4AddValue(Vector4 v, float add); // Add value to vector components
Vector4 Vector4Subtract(Vector4 v1, Vector4 v2); // Substract vectors
Vector4 Vector4SubtractValue(Vector4 v, float add); // Substract value from vector components
float Vector4Length(Vector4 v); // Vector length
float Vector4LengthSqr(Vector4 v); // Vector square length
float Vector4DotProduct(Vector4 v1, Vector4 v2); // Vectors dot product
float Vector4Distance(Vector4 v1, Vector4 v2); // Calculate distance between two vectors
float Vector4DistanceSqr(Vector4 v1, Vector4 v2); // Calculate square distance between two vectors
Vector4 Vector4Scale(Vector4 v, float scale); // Scale vector components by value (multiply)
Vector4 Vector4Multiply(Vector4 v1, Vector4 v2); // Multiply vector by vector
Vector4 Vector4Negate(Vector4 v); // Negate vector
Vector4 Vector4Divide(Vector4 v1, Vector4 v2); // Divide vector by vector
Vector4 Vector4Normalize(Vector4 v); // Normalize provided vector
Vector4 Vector4Min(Vector4 v1, Vector4 v2); // Get min value for each pair of components
Vector4 Vector4Max(Vector4 v1, Vector4 v2); // Get max value for each pair of components
Vector4 Vector4Lerp(Vector4 v1, Vector4 v2, float amount); // Calculate linear interpolation between two vectors
Vector4 Vector4MoveTowards(Vector4 v, Vector4 target, float maxDistance); // Move Vector towards target
Vector4 Vector4Invert(Vector4 v); // Invert the given vector
int Vector4Equals(Vector4 p, Vector4 q); // Check whether two given vectors are almost equal


//----------------------------------------------------------------------------------
// Module Functions Definition - Matrix math
//----------------------------------------------------------------------------------
float MatrixDeterminant(Matrix mat); // Compute matrix determinant
float MatrixTrace(Matrix mat); // Get the trace of the matrix (sum of the values along the diagonal)
Matrix MatrixTranspose(Matrix mat); // Transposes provided matrix
Matrix MatrixInvert(Matrix mat); // Invert provided matrix
Matrix MatrixIdentity(void); // Get identity matrix
Matrix MatrixAdd(Matrix left, Matrix right); // Add two matrices
Matrix MatrixSubtract(Matrix left, Matrix right); // Subtract two matrices (left - right)

// Get two matrix multiplication
// NOTE: When multiplying matrices... the order matters!
Matrix MatrixMultiply(Matrix left, Matrix right);
Matrix MatrixMultiplyValue(Matrix left, float value); // Multiply matrix components by value
Matrix MatrixTranslate(float x, float y, float z); // Get translation matrix

// Create rotation matrix from axis and angle
// NOTE: Angle should be provided in radians
Matrix MatrixRotate(Vector3 axis, float angle);

// Get x-rotation matrix
// NOTE: Angle must be provided in radians
Matrix MatrixRotateX(float angle);

// Get y-rotation matrix
// NOTE: Angle must be provided in radians
Matrix MatrixRotateY(float angle);

// Get z-rotation matrix
// NOTE: Angle must be provided in radians
Matrix MatrixRotateZ(float angle);


// Get xyz-rotation matrix
// NOTE: Angle must be provided in radians
Matrix MatrixRotateXYZ(Vector3 angle);

// Get zyx-rotation matrix
// NOTE: Angle must be provided in radians
Matrix MatrixRotateZYX(Vector3 angle);
Matrix MatrixScale(float x, float y, float z); // Get scaling matrix
Matrix MatrixFrustum(double left, double right, double bottom, double top, double nearPlane, double farPlane); // Get perspective projection matrix

// Get perspective projection matrix
// NOTE: Fovy angle must be provided in radians
Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane);
Matrix MatrixOrtho(double left, double right, double bottom, double top, double nearPlane, double farPlane); // Get orthographic projection matrix
Matrix MatrixLookAt(Vector3 eye, Vector3 target, Vector3 up); // Get camera look-at matrix (view matrix)
float16 MatrixToFloatV(Matrix mat); // Get float array of matrix data

//----------------------------------------------------------------------------------
// Module Functions Definition - Quaternion math
//----------------------------------------------------------------------------------

Quaternion QuaternionAdd(Quaternion q1, Quaternion q2); // Add two quaternions
Quaternion QuaternionAddValue(Quaternion q, float add); // Add quaternion and float value
Quaternion QuaternionSubtract(Quaternion q1, Quaternion q2); // Subtract two quaternions
Quaternion QuaternionSubtractValue(Quaternion q, float sub); // Subtract quaternion and float value
Quaternion QuaternionIdentity(void); // Get identity quaternion
float QuaternionLength(Quaternion q); // Computes the length of a quaternion
Quaternion QuaternionNormalize(Quaternion q); // Normalize provided quaternion
Quaternion QuaternionInvert(Quaternion q); // Invert provided quaternion
Quaternion QuaternionMultiply(Quaternion q1, Quaternion q2); // Calculate two quaternion multiplication
Quaternion QuaternionScale(Quaternion q, float mul); // Scale quaternion by float value
Quaternion QuaternionDivide(Quaternion q1, Quaternion q2); // Divide two quaternions
Quaternion QuaternionLerp(Quaternion q1, Quaternion q2, float amount); // Calculate linear interpolation between two quaternions
Quaternion QuaternionNlerp(Quaternion q1, Quaternion q2, float amount); // Calculate slerp-optimized interpolation between two quaternions
Quaternion QuaternionSlerp(Quaternion q1, Quaternion q2, float amount); // Calculates spherical linear interpolation between two quaternions

// Calculate quaternion cubic spline interpolation using Cubic Hermite Spline algorithm
// as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic
Quaternion QuaternionCubicHermiteSpline(Quaternion q1, Quaternion outTangent1, Quaternion q2, Quaternion inTangent2, float t);

// Calculate quaternion based on the rotation from one vector to another
Quaternion QuaternionFromVector3ToVector3(Vector3 from, Vector3 to);
Quaternion QuaternionFromMatrix(Matrix mat); // Get a quaternion for a given rotation matrix
Matrix QuaternionToMatrix(Quaternion q); // Get a matrix for a given quaternion

// Get rotation quaternion for an angle and axis
// NOTE: Angle must be provided in radians
Quaternion QuaternionFromAxisAngle(Vector3 axis, float angle);
void QuaternionToAxisAngle(Quaternion q, Vector3 *outAxis, float *outAngle); // Get the rotation angle and axis for a given quaternion

// Get the quaternion equivalent to Euler angles
// NOTE: Rotation order is ZYX
Quaternion QuaternionFromEuler(float pitch, float yaw, float roll);

// Get the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in radians
Vector3 QuaternionToEuler(Quaternion q);
Quaternion QuaternionTransform(Quaternion q, Matrix mat); // Transform a quaternion given a transformation matrix
int QuaternionEquals(Quaternion p, Quaternion q); // Check whether two given quaternions are almost equal
Matrix MatrixCompose(Vector3 translation, Quaternion rotation, Vector3 scale); // Compose a transformation matrix from rotational, translational and scaling components
void MatrixDecompose(Matrix mat, Vector3 *translation, Quaternion *rotation, Vector3 *scale); // Decompose a transformation matrix into its rotational, translational and scaling components and remove shear

#if defined(__cplusplus) && !defined(MATHS_DISABLE_CPP_OPERATORS)

// Optional C++ math operators
//-------------------------------------------------------------------------------

// Vector2 operators
static constexpr Vector2 Vector2Zeros = { 0, 0 };
static constexpr Vector2 Vector2Ones = { 1, 1 };
static constexpr Vector2 Vector2UnitX = { 1, 0 };
static constexpr Vector2 Vector2UnitY = { 0, 1 };

inline Vector2 operator + (const Vector2& lhs, const Vector2& rhs)
{
    return Vector2Add(lhs, rhs);
}

inline const Vector2& operator += (Vector2& lhs, const Vector2& rhs)
{
    lhs = Vector2Add(lhs, rhs);
    return lhs;
}

inline Vector2 operator - (const Vector2& lhs, const Vector2& rhs)
{
    return Vector2Subtract(lhs, rhs);
}

inline const Vector2& operator -= (Vector2& lhs, const Vector2& rhs)
{
    lhs = Vector2Subtract(lhs, rhs);
    return lhs;
}

inline Vector2 operator * (const float& lhs, const Vector2& rhs)
{
    return Vector2Scale(rhs, lhs);
}

inline Vector2 operator * (const Vector2& lhs, const float& rhs)
{
    return Vector2Scale(lhs, rhs);
}

inline const Vector2& operator *= (Vector2& lhs, const float& rhs)
{
    lhs = Vector2Scale(lhs, rhs);
    return lhs;
}

inline Vector2 operator * (const Vector2& lhs, const Vector2& rhs)
{
    return Vector2Multiply(lhs, rhs);
}

inline const Vector2& operator *= (Vector2& lhs, const Vector2& rhs)
{
    lhs = Vector2Multiply(lhs, rhs);
    return lhs;
}

inline Vector2 operator * (const Vector2& lhs, const Matrix& rhs)
{
    return Vector2Transform(lhs, rhs);
}

inline const Vector2& operator *= (Vector2& lhs, const Matrix& rhs)
{
    lhs = Vector2Transform(lhs, rhs);
    return lhs;
}

inline Vector2 operator / (const Vector2& lhs, const float& rhs)
{
    return Vector2Scale(lhs, 1.0f/rhs);
}

inline const Vector2& operator /= (Vector2& lhs, const float& rhs)
{
    lhs = Vector2Scale(lhs, 1.0f/rhs);
    return lhs;
}

inline Vector2 operator / (const Vector2& lhs, const Vector2& rhs)
{
    return Vector2Divide(lhs, rhs);
}

inline const Vector2& operator /= (Vector2& lhs, const Vector2& rhs)
{
    lhs = Vector2Divide(lhs, rhs);
    return lhs;
}

inline bool operator == (const Vector2& lhs, const Vector2& rhs)
{
    return FloatEquals(lhs.x, rhs.x) && FloatEquals(lhs.y, rhs.y);
}

inline bool operator != (const Vector2& lhs, const Vector2& rhs)
{
    return !FloatEquals(lhs.x, rhs.x) || !FloatEquals(lhs.y, rhs.y);
}

// Vector3 operators
static constexpr Vector3 Vector3Zeros = { 0, 0, 0 };
static constexpr Vector3 Vector3Ones = { 1, 1, 1 };
static constexpr Vector3 Vector3UnitX = { 1, 0, 0 };
static constexpr Vector3 Vector3UnitY = { 0, 1, 0 };
static constexpr Vector3 Vector3UnitZ = { 0, 0, 1 };

inline Vector3 operator + (const Vector3& lhs, const Vector3& rhs)
{
    return Vector3Add(lhs, rhs);
}

inline const Vector3& operator += (Vector3& lhs, const Vector3& rhs)
{
    lhs = Vector3Add(lhs, rhs);
    return lhs;
}

inline Vector3 operator - (const Vector3& lhs, const Vector3& rhs)
{
    return Vector3Subtract(lhs, rhs);
}

inline const Vector3& operator -= (Vector3& lhs, const Vector3& rhs)
{
    lhs = Vector3Subtract(lhs, rhs);
    return lhs;
}

inline Vector3 operator * (const float& lhs, const Vector3& rhs)
{
    return Vector3Scale(rhs, lhs);
}

inline Vector3 operator * (const Vector3& lhs, const float& rhs)
{
    return Vector3Scale(lhs, rhs);
}

inline const Vector3& operator *= (Vector3& lhs, const float& rhs)
{
    lhs = Vector3Scale(lhs, rhs);
    return lhs;
}

inline Vector3 operator * (const Vector3& lhs, const Vector3& rhs)
{
    return Vector3Multiply(lhs, rhs);
}

inline const Vector3& operator *= (Vector3& lhs, const Vector3& rhs)
{
    lhs = Vector3Multiply(lhs, rhs);
    return lhs;
}

inline Vector3 operator * (const Vector3& lhs, const Matrix& rhs)
{
    return Vector3Transform(lhs, rhs);
}

inline const Vector3& operator *= (Vector3& lhs, const Matrix& rhs)
{
    lhs = Vector3Transform(lhs, rhs);
    return lhs;
}

inline Vector3 operator / (const Vector3& lhs, const float& rhs)
{
    return Vector3Scale(lhs, 1.0f/rhs);
}

inline const Vector3& operator /= (Vector3& lhs, const float& rhs)
{
    lhs = Vector3Scale(lhs, 1.0f/rhs);
    return lhs;
}

inline Vector3 operator / (const Vector3& lhs, const Vector3& rhs)
{
    return Vector3Divide(lhs, rhs);
}

inline const Vector3& operator /= (Vector3& lhs, const Vector3& rhs)
{
    lhs = Vector3Divide(lhs, rhs);
    return lhs;
}

inline bool operator == (const Vector3& lhs, const Vector3& rhs)
{
    return FloatEquals(lhs.x, rhs.x) && FloatEquals(lhs.y, rhs.y) && FloatEquals(lhs.z, rhs.z);
}

inline bool operator != (const Vector3& lhs, const Vector3& rhs)
{
    return !FloatEquals(lhs.x, rhs.x) || !FloatEquals(lhs.y, rhs.y) || !FloatEquals(lhs.z, rhs.z);
}

// Vector4 operators
static constexpr Vector4 Vector4Zeros = { 0, 0, 0, 0 };
static constexpr Vector4 Vector4Ones = { 1, 1, 1, 1 };
static constexpr Vector4 Vector4UnitX = { 1, 0, 0, 0 };
static constexpr Vector4 Vector4UnitY = { 0, 1, 0, 0 };
static constexpr Vector4 Vector4UnitZ = { 0, 0, 1, 0 };
static constexpr Vector4 Vector4UnitW = { 0, 0, 0, 1 };

inline Vector4 operator + (const Vector4& lhs, const Vector4& rhs)
{
    return Vector4Add(lhs, rhs);
}

inline const Vector4& operator += (Vector4& lhs, const Vector4& rhs)
{
    lhs = Vector4Add(lhs, rhs);
    return lhs;
}

inline Vector4 operator - (const Vector4& lhs, const Vector4& rhs)
{
    return Vector4Subtract(lhs, rhs);
}

inline const Vector4& operator -= (Vector4& lhs, const Vector4& rhs)
{
    lhs = Vector4Subtract(lhs, rhs);
    return lhs;
}

inline Vector4 operator * (const float& lhs, const Vector4& rhs)
{
    return Vector4Scale(rhs, lhs);
}

inline Vector4 operator * (const Vector4& lhs, const float& rhs)
{
    return Vector4Scale(lhs, rhs);
}

inline const Vector4& operator *= (Vector4& lhs, const float& rhs)
{
    lhs = Vector4Scale(lhs, rhs);
    return lhs;
}

inline Vector4 operator * (const Vector4& lhs, const Vector4& rhs)
{
    return Vector4Multiply(lhs, rhs);
}

inline const Vector4& operator *= (Vector4& lhs, const Vector4& rhs)
{
    lhs = Vector4Multiply(lhs, rhs);
    return lhs;
}

inline Vector4 operator / (const Vector4& lhs, const float& rhs)
{
    return Vector4Scale(lhs, 1.0f/rhs);
}

inline const Vector4& operator /= (Vector4& lhs, const float& rhs)
{
    lhs = Vector4Scale(lhs, 1.0f/rhs);
    return lhs;
}

inline Vector4 operator / (const Vector4& lhs, const Vector4& rhs)
{
    return Vector4Divide(lhs, rhs);
}

inline const Vector4& operator /= (Vector4& lhs, const Vector4& rhs)
{
    lhs = Vector4Divide(lhs, rhs);
    return lhs;
}

inline bool operator == (const Vector4& lhs, const Vector4& rhs)
{
    return FloatEquals(lhs.x, rhs.x) && FloatEquals(lhs.y, rhs.y) && FloatEquals(lhs.z, rhs.z) && FloatEquals(lhs.w, rhs.w);
}

inline bool operator != (const Vector4& lhs, const Vector4& rhs)
{
    return !FloatEquals(lhs.x, rhs.x) || !FloatEquals(lhs.y, rhs.y) || !FloatEquals(lhs.z, rhs.z) || !FloatEquals(lhs.w, rhs.w);
}

// Quaternion operators
static constexpr Quaternion QuaternionZeros = { 0, 0, 0, 0 };
static constexpr Quaternion QuaternionOnes = { 1, 1, 1, 1 };
static constexpr Quaternion QuaternionUnitX = { 0, 0, 0, 1 };

inline Quaternion operator + (const Quaternion& lhs, const float& rhs)
{
    return QuaternionAddValue(lhs, rhs);
}

inline const Quaternion& operator += (Quaternion& lhs, const float& rhs)
{
    lhs = QuaternionAddValue(lhs, rhs);
    return lhs;
}

inline Quaternion operator - (const Quaternion& lhs, const float& rhs)
{
    return QuaternionSubtractValue(lhs, rhs);
}

inline const Quaternion& operator -= (Quaternion& lhs, const float& rhs)
{
    lhs = QuaternionSubtractValue(lhs, rhs);
    return lhs;
}

inline Quaternion operator * (const Quaternion& lhs, const Matrix& rhs)
{
    return QuaternionTransform(lhs, rhs);
}

inline const Quaternion& operator *= (Quaternion& lhs, const Matrix& rhs)
{
    lhs = QuaternionTransform(lhs, rhs);
    return lhs;
}

// Matrix operators
static constexpr Matrix MatrixUnit = { 1, 0, 0, 0,
                                       0, 1, 0, 0,
                                       0, 0, 1, 0,
                                       0, 0, 0, 1 };

inline Matrix operator + (const Matrix& lhs, const Matrix& rhs)
{
    return MatrixAdd(lhs, rhs);
}

inline const Matrix& operator += (Matrix& lhs, const Matrix& rhs)
{
    lhs = MatrixAdd(lhs, rhs);
    return lhs;
}

inline Matrix operator - (const Matrix& lhs, const Matrix& rhs)
{
    return MatrixSubtract(lhs, rhs);
}

inline const Matrix& operator -= (Matrix& lhs, const Matrix& rhs)
{
    lhs = MatrixSubtract(lhs, rhs);
    return lhs;
}

inline Matrix operator * (const Matrix& lhs, const Matrix& rhs)
{
    return MatrixMultiply(lhs, rhs);
}

inline const Matrix& operator *= (Matrix& lhs, const Matrix& rhs)
{
    lhs = MatrixMultiply(lhs, rhs);
    return lhs;
}

inline Matrix operator * (const Matrix& lhs, const float value)
{
    return MatrixMultiplyValue(lhs, value);
}

inline const Matrix& operator *= (Matrix& lhs, const float value)
{
    lhs = MatrixMultiplyValue(lhs, value);
    return lhs;
}

//-------------------------------------------------------------------------------
#endif // C++ operators

#endif // MATHS_H