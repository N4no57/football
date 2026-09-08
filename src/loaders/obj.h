#ifndef FOOTIE_LIFE_OBJ_H
#define FOOTIE_LIFE_OBJ_H

#include <ints.h>

#include "../maths/maths.h"

typedef u16 OBJIndex;

typedef struct OBJFace {
    OBJIndex index[3];
} OBJFace;

typedef struct OBJGroup {
    char *name;

    Vector3 *positions;
    size_t position_count;

    Vector3 *normals;
    size_t normal_count;

    Vector2 *texcoords;
    size_t texcoord_count;

    OBJFace *faces;
    size_t face_count;
} OBJGroup;

typedef struct OBJFile {
    OBJGroup *groups;
    size_t group_count;
} OBJFile;

OBJFile LoadOBJ(const char *fileName);

#endif //FOOTIE_LIFE_OBJ_H
