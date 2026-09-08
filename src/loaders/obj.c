#include "obj.h"
#include "generic.h"
#include "../logging.h"
#include "external/tinyobj_loader_c.h"

OBJFile LoadOBJ(const char *fileName) {
    OBJFile obj = {0};

    tinyobj_attrib_t objAttributes = {0};
    tinyobj_shape_t *objShapes = nullptr;
    unsigned int objShapeCount = 0;

    tinyobj_material_t *objMaterials = nullptr;
    unsigned int objMaterialCount = 0;

    size_t data_size;
    char *file_text = LoadFileText(fileName, &data_size);

    const unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    const int ret = tinyobj_parse_obj(&objAttributes, &objShapes, &objShapeCount, &objMaterials, &objMaterialCount, file_text, data_size, flags);

    FreeFileText(file_text);

    if (ret != TINYOBJ_SUCCESS) {
        TraceLog(LOG_WARNING, "OBJ READER: Unable to read obj data %s", fileName);
        return obj;
    }

    unsigned int faceVertIndex = 0;
    unsigned int nextShape = 1;
    int lastMaterial = -1;
    unsigned int meshIndex = 0;

    unsigned int nextShapeEnd = objAttributes.num_face_num_verts;

    if (objShapeCount > 1) nextShapeEnd = objShapes[nextShape].face_offset;

    return obj;
}
