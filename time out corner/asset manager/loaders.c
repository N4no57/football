#include "loaders.h"

#include <raylib.h>
#include <raymath.h>
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rlgl.h>

#if defined(_WIN32)
    #include <direct.h>     // Required for: _chdir() [Used in LoadOBJ()]
    #define CHDIR _chdir
#else
    #include <unistd.h>     // Required for: chdir() (POSIX) [Used in LoadOBJ()]
    #define CHDIR chdir
#endif

#if SUPPORT_FILEFORMAT_GLTF
    #define CGLTF_IMPLEMENTATION
#endif
#if SUPPORT_FILEFORMAT_GLTF_WRITE
    // NOTE: No need for custom allocators, memory buffer provided
    #define CGLTF_WRITE_IMPLEMENTATION
    #include "external/cgltf_write.h"   // glTF file format writing
#endif

#if SUPPORT_FILEFORMAT_VOX
    #define VOX_LOADER_IMPLEMENTATION
#endif

#if SUPPORT_FILEFORMAT_M3D
    #define M3D_IMPLEMENTATION
#endif

#ifndef MAX_MATERIAL_MAPS
    #define MAX_MATERIAL_MAPS       12      // Maximum number of maps supported
#endif
#ifndef MAX_MESH_VERTEX_BUFFERS
#if SUPPORT_GPU_SKINNING
    // NOTE: Two additional vertex buffers required to store bone indices and bone weights
    // WARNING: Some GPUs could not support more than 8 VBOs
    #define MAX_MESH_VERTEX_BUFFERS  9      // Maximum vertex buffers (VBO) per mesh
#else
    #define MAX_MESH_VERTEX_BUFFERS  7      // Maximum vertex buffers (VBO) per mesh
#endif
#endif
#ifndef MAX_FILEPATH_LENGTH
    #define MAX_FILEPATH_LENGTH   4096      // Maximum length for filepaths (Linux PATH_MAX default value)
#endif

#if SUPPORT_FILEFORMAT_OBJ || SUPPORT_FILEFORMAT_MTL
    #define TINYOBJ_LOADER_C_IMPLEMENTATION
#endif

#include <rextras.h>

#if SUPPORT_FILEFORMAT_OBJ || SUPPORT_FILEFORMAT_MTL
// Process obj materials
static void ProcessMaterialsOBJ(Material *materials, tinyobj_material_t *mats, int materialCount) {
    // Init model mats
    for (int m = 0; m < materialCount; m++) {
        // Init material to default
        // NOTE: Uses default shader, which only supports MATERIAL_MAP_DIFFUSE
        materials[m] = LoadMaterialDefault();

        if (mats == nullptr) continue;

        // Get default texture, in case no texture is defined
        // NOTE: rlgl default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
        materials[m].maps[MATERIAL_MAP_DIFFUSE].texture = (Texture2D){ rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

        if (mats[m].diffuse_texname != nullptr) materials[m].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture(mats[m].diffuse_texname);  //char *diffuse_texname; // map_Kd
        else materials[m].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ (unsigned char)(mats[m].diffuse[0]*255.0f), (unsigned char)(mats[m].diffuse[1]*255.0f), (unsigned char)(mats[m].diffuse[2]*255.0f), 255 }; //float diffuse[3];
        materials[m].maps[MATERIAL_MAP_DIFFUSE].value = 0.0f;

        if (mats[m].specular_texname != nullptr) materials[m].maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture(mats[m].specular_texname);  //char *specular_texname; // map_Ks
        materials[m].maps[MATERIAL_MAP_SPECULAR].color = (Color){ (unsigned char)(mats[m].specular[0]*255.0f), (unsigned char)(mats[m].specular[1]*255.0f), (unsigned char)(mats[m].specular[2]*255.0f), 255 }; //float specular[3];
        materials[m].maps[MATERIAL_MAP_SPECULAR].value = 0.0f;

        if (mats[m].bump_texname != nullptr) materials[m].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture(mats[m].bump_texname);  //char *bump_texname; // map_bump, bump
        materials[m].maps[MATERIAL_MAP_NORMAL].color = WHITE;
        materials[m].maps[MATERIAL_MAP_NORMAL].value = mats[m].shininess;

        materials[m].maps[MATERIAL_MAP_EMISSION].color = (Color){ (unsigned char)(mats[m].emission[0]*255.0f), (unsigned char)(mats[m].emission[1]*255.0f), (unsigned char)(mats[m].emission[2]*255.0f), 255 }; //float emission[3];

        if (mats[m].displacement_texname != nullptr) materials[m].maps[MATERIAL_MAP_HEIGHT].texture = LoadTexture(mats[m].displacement_texname);  //char *displacement_texname; // disp
    }
}

static Model LoadOBJ(const char *fileName) {
    tinyobj_attrib_t objAttributes = {0};
    tinyobj_shape_t *objShapes = nullptr;
    unsigned int objShapeCount = 0;

    tinyobj_material_t *objMaterials = nullptr;
    unsigned int objMaterialCount = 0;

    Model model = { 0 };
    model.transform = MatrixIdentity();

    char *fileText = LoadFileText(fileName);

    if (fileText == nullptr) {
        TraceLog(LOG_WARNING, "MODEL: [%s] Unable to read obj file", fileName);
        return model;
    }

    char currentDir[MAX_FILEPATH_LENGTH] = { 0 };
    snprintf(currentDir, MAX_FILEPATH_LENGTH, "%s", GetWorkingDirectory()); // Save current working directory
    const char *workingDir = GetDirectoryPath(fileName); // Switch to OBJ directory for material path correctness
    if (CHDIR(workingDir) != 0) TraceLog(LOG_WARNING, "MODEL: [%s] Failed to change working directory", workingDir);

    unsigned int dataSize = (unsigned int)strlen(fileText);

    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret = tinyobj_parse_obj(&objAttributes, &objShapes, &objShapeCount, &objMaterials, &objMaterialCount, fileText, dataSize, flags);

    if (ret != TINYOBJ_SUCCESS) {
        TraceLog(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);
        return model;
    }

    UnloadFileText(fileText);

    unsigned int faceVertIndex = 0;
    unsigned int nextShape = 1;
    int lastMaterial = -1;
    unsigned int meshIndex = 0;

    // Count meshes
    unsigned int nextShapeEnd = objAttributes.num_face_num_verts;

    // See how many verts till the next shape
    if (objShapeCount > 1) nextShapeEnd = objShapes[nextShape].face_offset;

    // Walk all the faces
    for (unsigned int faceId = 0; faceId < objAttributes.num_faces; faceId++) {
        if (faceId >= nextShapeEnd) {
            // Try to find the last vert in the next shape
            nextShape++;
            if (nextShape < objShapeCount) nextShapeEnd = objShapes[nextShape].face_offset;
            else nextShapeEnd = objAttributes.num_face_num_verts; // This is actually the total number of face verts in the file, not faces
            meshIndex++;
        } else if ((lastMaterial != -1) && (objAttributes.material_ids[faceId] != lastMaterial)) {
            meshIndex++; // If this is a new material, a new mesh is allocated
        }

        lastMaterial = objAttributes.material_ids[faceId];
        faceVertIndex += objAttributes.face_num_verts[faceId];
    }

    // Allocate the base meshes and materials
    model.meshCount = (int)meshIndex + 1;
    model.meshes = (Mesh *)MemAlloc(sizeof(Mesh) * (unsigned int)model.meshCount);

    if (objMaterialCount > 0) {
        model.materialCount = (int)objMaterialCount;
        model.materials = (Material *)MemAlloc(sizeof(Material)*objMaterialCount);
    } else { // Allocate at least one material
        model.materialCount = 1;
        model.materials = (Material *)MemAlloc(sizeof(Material)*1);
    }

    model.meshMaterial = (int *)MemAlloc(sizeof(int) * (unsigned int)model.meshCount);

    // See how many verts are in each mesh
    unsigned int *localMeshVertexCounts = (unsigned int *)MemAlloc(sizeof(unsigned int) * (unsigned int)model.meshCount);

    faceVertIndex = 0;
    nextShapeEnd = objAttributes.num_face_num_verts;
    lastMaterial = -1;
    meshIndex = 0;
    unsigned int localMeshVertexCount = 0;

    nextShape = 1;
    if (objShapeCount > 1) nextShapeEnd = objShapes[nextShape].face_offset;

    // Walk all the faces
    for (unsigned int faceId = 0; faceId < objAttributes.num_faces; faceId++) {
        bool newMesh = false; // Is a new mesh required?
        if (faceId >= nextShapeEnd) {
            // Try to find the last vert in the next shape
            nextShape++;
            if (nextShape < objShapeCount) nextShapeEnd = objShapes[nextShape].face_offset;
            else nextShapeEnd = objAttributes.num_face_num_verts; // this is actually the total number of face verts in the file, not faces

            newMesh = true;
        } else if ((lastMaterial != -1) && (objAttributes.material_ids[faceId] != lastMaterial)) {
            newMesh = true;
        }

        lastMaterial = objAttributes.material_ids[faceId];

        if (newMesh) {
            localMeshVertexCounts[meshIndex] = localMeshVertexCount;

            localMeshVertexCount = 0;
            meshIndex++;
        }

        faceVertIndex += objAttributes.face_num_verts[faceId];
        localMeshVertexCount += objAttributes.face_num_verts[faceId];
    }

    localMeshVertexCounts[meshIndex] = localMeshVertexCount;

    for (int i = 0; i < model.meshCount; i++) {
        // Allocate the buffers for each mesh
        unsigned int vertexCount = localMeshVertexCounts[i];

        model.meshes[i].vertexCount = (int)vertexCount;
        model.meshes[i].triangleCount = (int)vertexCount/3;

        model.meshes[i].vertices = (float *)MemAlloc(sizeof(float)*vertexCount*3);
        model.meshes[i].normals = (float *)MemAlloc(sizeof(float)*vertexCount*3);
        #if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
        model.meshes[i].texcoords = (float *)MemAlloc(sizeof(float)*vertexCount*2);
        model.meshes[i].colors = (unsigned char *)MemAlloc(sizeof(unsigned char)*vertexCount*4);
        #else
        if (objAttributes.texcoords != nullptr && objAttributes.num_texcoords > 0)
            model.meshes[i].texcoords = (float *)MemAlloc(sizeof(float)*vertexCount*2);
        else model.meshes[i].texcoords = nullptr;
        model.meshes[i].colors = nullptr;
        #endif
    }

    MemFree(localMeshVertexCounts);
    localMeshVertexCounts = nullptr;

    // Fill meshes
    faceVertIndex = 0;

    nextShapeEnd = objAttributes.num_face_num_verts;

    // See how many verts till the next shape
    nextShape = 1;
    if (objShapeCount > 1) nextShapeEnd = objShapes[nextShape].face_offset;
    lastMaterial = -1;
    meshIndex = 0;
    localMeshVertexCount = 0;

    // Walk all the faces
    for (unsigned int faceId = 0; faceId < objAttributes.num_faces; faceId++) {
        bool newMesh = false; // Is a new mesh required?
        if (faceId >= nextShapeEnd) {
            // Try to find the last vert in the next shape
            nextShape++;
            if (nextShape < objShapeCount) nextShapeEnd = objShapes[nextShape].face_offset;
            else nextShapeEnd = objAttributes.num_face_num_verts; // This is actually the total number of face verts in the file, not faces
            newMesh = true;
        }

        // If this is a new material, a new mesh is allocated
        if (lastMaterial != -1 && objAttributes.material_ids[faceId] != lastMaterial) newMesh = true;
        lastMaterial = objAttributes.material_ids[faceId];

        if (newMesh) {
            localMeshVertexCount = 0;
            meshIndex++;
        }

        int matId = 0;
        if ((lastMaterial >= 0) && (lastMaterial < (int)objMaterialCount)) matId = lastMaterial;

        model.meshMaterial[meshIndex] = matId;

        for (int f = 0; f < objAttributes.face_num_verts[faceId]; f++) {
            int vertIndex = objAttributes.faces[faceVertIndex].v_idx;
            int normalIndex = objAttributes.faces[faceVertIndex].vn_idx;
            int texcordIndex = objAttributes.faces[faceVertIndex].vt_idx;

            for (int i = 0; i < 3; i++) model.meshes[meshIndex].vertices[localMeshVertexCount*3 + i] = objAttributes.vertices[vertIndex*3 + i];

            if ((objAttributes.texcoords != nullptr) && (texcordIndex != TINYOBJ_INVALID_INDEX) && (texcordIndex >= 0) && (model.meshes[meshIndex].texcoords)) {
                for (int i = 0; i < 2; i++) model.meshes[meshIndex].texcoords[localMeshVertexCount*2 + i] = objAttributes.texcoords[texcordIndex*2 + i];
                model.meshes[meshIndex].texcoords[localMeshVertexCount*2 + 1] = 1.0f - model.meshes[meshIndex].texcoords[localMeshVertexCount*2 + 1];
            }

            if ((objAttributes.normals != nullptr) && (normalIndex != TINYOBJ_INVALID_INDEX) && (normalIndex >= 0)) {
                for (int i = 0; i < 3; i++) model.meshes[meshIndex].normals[localMeshVertexCount*3 + i] = objAttributes.normals[normalIndex*3 + i];
            } else {
                model.meshes[meshIndex].normals[localMeshVertexCount*3 + 0] = 0.0f;
                model.meshes[meshIndex].normals[localMeshVertexCount*3 + 1] = 1.0f;
                model.meshes[meshIndex].normals[localMeshVertexCount*3 + 2] = 0.0f;
            }
        #if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
            for (int i = 0; i < 4; i++) model.meshes[meshIndex].colors[localMeshVertexCount*4 + i] = 255;
        #endif
            faceVertIndex++;
            localMeshVertexCount++;
        }
    }

    if (objMaterialCount > 0) ProcessMaterialsOBJ(model.materials, objMaterials, objMaterialCount);
    else model.materials[0] = LoadMaterialDefault(); // Set default material for the mesh

    tinyobj_attrib_free(&objAttributes);
    tinyobj_shapes_free(objShapes, objShapeCount);
    tinyobj_materials_free(objMaterials, objMaterialCount);

    // Restore current working directory
    if (CHDIR(currentDir) != 0) {
        TraceLog(LOG_WARNING, "MODEL: [%s] Failed to change working directory", currentDir);
    }

    return model;
}
#endif

#if SUPPORT_FILEFORMAT_IQM || SUPPORT_FILEFORMAT_GLTF
// Build pose from parent joints
// NOTE: Required for animations loading (required by IQM and GLTF)
static void BuildPoseFromParentJoints(BoneInfo *bones, int boneCount, Transform *transforms) {
    for (int i = 0; i < boneCount; i++) {
        if (bones[i].parent >= 0) {
            if (bones[i].parent > i) {
                TraceLog(LOG_WARNING, "Skipping bone not topologically sorted: Bone %d has parent %d", i, bones[i].parent);
                continue;
            }
            transforms[i].rotation = QuaternionMultiply(transforms[bones[i].parent].rotation, transforms[i].rotation);
            transforms[i].scale = Vector3Multiply(transforms[i].scale, transforms[bones[i].parent].scale);
            transforms[i].translation = Vector3Multiply(transforms[i].translation, transforms[bones[i].parent].scale);
            transforms[i].translation = Vector3RotateByQuaternion(transforms[i].translation, transforms[bones[i].parent].rotation);
            transforms[i].translation = Vector3Add(transforms[i].translation, transforms[bones[i].parent].translation);
        }
    }
}
#endif

#if SUPPORT_FILEFORMAT_IQM
// Load IQM mesh data
static Model LoadIQM(const char *fileName) {
    #define IQM_MAGIC        "INTERQUAKEMODEL"  // IQM file magic number
    #define IQM_VERSION              2          // Only IQM version 2 supported

    #define BONE_NAME_LENGTH        32          // BoneInfo name string length
    #define MESH_NAME_LENGTH        32          // Mesh name string length
    #define MATERIAL_NAME_LENGTH    32          // Material name string length

    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);
    unsigned char *fileDataPtr = fileData;

    // IQM file structs
    //-----------------------------------------------------------------------------------
    typedef struct IQMHeader {
        char magic[16];
        unsigned int version;
        unsigned int dataSize;
        unsigned int flags;
        unsigned int num_text, ofs_text;
        unsigned int num_meshes, ofs_meshes;
        unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
        unsigned int num_triangles, ofs_triangles, ofs_adjacency;
        unsigned int num_joints, ofs_joints;
        unsigned int num_poses, ofs_poses;
        unsigned int num_anims, ofs_anims;
        unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
        unsigned int num_comment, ofs_comment;
        unsigned int num_extensions, ofs_extensions;
    } IQMHeader;

    typedef struct IQMMesh {
        unsigned int name;
        unsigned int material;
        unsigned int first_vertex, num_vertexes;
        unsigned int first_triangle, num_triangles;
    } IQMMesh;

    typedef struct IQMTriangle {
        unsigned int vertex[3];
    } IQMTriangle;

    typedef struct IQMJoint {
        unsigned int name;
        int parent;
        float translate[3], rotate[4], scale[3];
    } IQMJoint;

    typedef struct IQMVertexArray {
        unsigned int type;
        unsigned int flags;
        unsigned int format;
        unsigned int size;
        unsigned int offset;
    } IQMVertexArray;

    // NOTE: Below IQM structures are not used but listed for reference
    /*
    typedef struct IQMAdjacency {
        unsigned int triangle[3];
    } IQMAdjacency;

    typedef struct IQMPose {
        int parent;
        unsigned int mask;
        float channeloffset[10];
        float channelscale[10];
    } IQMPose;

    typedef struct IQMAnim {
        unsigned int name;
        unsigned int first_frame, num_frames;
        float framerate;
        unsigned int flags;
    } IQMAnim;

    typedef struct IQMBounds {
        float bbmin[3], bbmax[3];
        float xyradius, radius;
    } IQMBounds;
    */
    //-----------------------------------------------------------------------------------

    // IQM vertex data types
    enum {
        IQM_POSITION     = 0,
        IQM_TEXCOORD     = 1,
        IQM_NORMAL       = 2,
        IQM_TANGENT      = 3,       // NOTE: Tangents unused by default
        IQM_BLENDINDEXES = 4,
        IQM_BLENDWEIGHTS = 5,
        IQM_COLOR        = 6,
        IQM_CUSTOM       = 0x10     // NOTE: Custom vertex values unused by default
    };

    Model model = {0};

    IQMMesh *imesh = nullptr;
    IQMTriangle *tri = nullptr;
    IQMVertexArray *va = nullptr;
    IQMJoint *ijoint = nullptr;

    float *vertex = nullptr;
    float *normal = nullptr;
    float *text = nullptr;
    char *blendi = nullptr;
    unsigned char *blendw = nullptr;
    unsigned char *color = nullptr;

    // In case file can not be read, return an empty model
    if (fileDataPtr == nullptr) return model;

    const char *basePath = GetDirectoryPath(fileName);

    // Read IQM header
    IQMHeader *iqmHeader = (IQMHeader *)fileDataPtr;

    if (memcmp(iqmHeader->magic, IQM_MAGIC, sizeof(IQM_MAGIC)) != 0) {
        TraceLog(LOG_WARNING, "MODEL: [%s] IQM file is not a valid model", fileName);
        UnloadFileData(fileData);
        return model;
    }

    if (iqmHeader->version != IQM_VERSION) {
        TraceLog(LOG_WARNING, "MODEL: [%s] IQM file version not supported (%i)", fileName, iqmHeader->version);
        UnloadFileData(fileData);
        return model;
    }

    //fileDataPtr += sizeof(IQMHeader);       // Move file data pointer

    // Meshes data processing
    imesh = (IQMMesh *)RL_MALLOC(iqmHeader->num_meshes*sizeof(IQMMesh));
    //fseek(iqmFile, iqmHeader->ofs_meshes, SEEK_SET);
    //fread(imesh, sizeof(IQMMesh)*iqmHeader->num_meshes, 1, iqmFile);
    memcpy(imesh, fileDataPtr + iqmHeader->ofs_meshes, iqmHeader->num_meshes*sizeof(IQMMesh));

    model.meshCount = iqmHeader->num_meshes;
    model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));

    model.materialCount = model.meshCount;
    model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
    model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));

    char name[MESH_NAME_LENGTH] = { 0 };
    char material[MATERIAL_NAME_LENGTH] = { 0 };

    for (int i = 0; i < model.meshCount; i++) {
        //fseek(iqmFile, iqmHeader->ofs_text + imesh[a].name, SEEK_SET);
        //fread(name, sizeof(char), MESH_NAME_LENGTH, iqmFile);
        memcpy(name, fileDataPtr + iqmHeader->ofs_text + imesh[i].name, MESH_NAME_LENGTH*sizeof(char));

        //fseek(iqmFile, iqmHeader->ofs_text + imesh[a].material, SEEK_SET);
        //fread(material, sizeof(char), MATERIAL_NAME_LENGTH, iqmFile);
        memcpy(material, fileDataPtr + iqmHeader->ofs_text + imesh[i].material, MATERIAL_NAME_LENGTH*sizeof(char));

        model.materials[i] = LoadMaterialDefault();
        model.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture(TextFormat("%s/%s", basePath, material));

        model.meshMaterial[i] = i;

        TraceLog(LOG_DEBUG, "MODEL: [%s] mesh name (%s), material (%s)", fileName, name, material);

        model.meshes[i].vertexCount = imesh[i].num_vertexes;

        model.meshes[i].vertices = (float *)RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));       // Default vertex positions
        model.meshes[i].normals = (float *)RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));        // Default vertex normals
        model.meshes[i].texcoords = (float *)RL_CALLOC(model.meshes[i].vertexCount*2, sizeof(float));      // Default vertex texcoords

        model.meshes[i].boneIndices = (unsigned char *)RL_CALLOC(model.meshes[i].vertexCount*4, sizeof(unsigned char));  // Up-to 4 bones supported!
        model.meshes[i].boneWeights = (float *)RL_CALLOC(model.meshes[i].vertexCount*4, sizeof(float));      // Up-to 4 bones supported!

        model.meshes[i].triangleCount = imesh[i].num_triangles;
        model.meshes[i].indices = (unsigned short *)RL_CALLOC(model.meshes[i].triangleCount*3, sizeof(unsigned short));

        #if !SUPPORT_GPU_SKINNING
        // Animated vertex data, processed for rendering
        // NOTE: Animated vertex should be re-uploaded to GPU (if not using GPU skinning)
        model.meshes[i].animVertices = (float *)RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));
        model.meshes[i].animNormals = (float *)RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));
        #endif
    }

    // Triangles data processing
    tri = (IQMTriangle *)RL_MALLOC(iqmHeader->num_triangles*sizeof(IQMTriangle));
    //fseek(iqmFile, iqmHeader->ofs_triangles, SEEK_SET);
    //fread(tri, sizeof(IQMTriangle), iqmHeader->num_triangles, iqmFile);
    memcpy(tri, fileDataPtr + iqmHeader->ofs_triangles, iqmHeader->num_triangles*sizeof(IQMTriangle));

    for (int m = 0; m < model.meshCount; m++) {
        int tcounter = 0;

        for (unsigned int i = imesh[m].first_triangle; i < (imesh[m].first_triangle + imesh[m].num_triangles); i++) {
            // IQM triangles indexes are stored in counter-clockwise, but raylib processes the index in linear order,
            // expecting they point to the counter-clockwise vertex triangle, so triangle indexes need to be reversed
            // NOTE: raylib renders vertex data in counter-clockwise order (standard convention) by default
            model.meshes[m].indices[tcounter + 2] = tri[i].vertex[0] - imesh[m].first_vertex;
            model.meshes[m].indices[tcounter + 1] = tri[i].vertex[1] - imesh[m].first_vertex;
            model.meshes[m].indices[tcounter] = tri[i].vertex[2] - imesh[m].first_vertex;
            tcounter += 3;
        }
    }

    // Vertex arrays data processing
    va = (IQMVertexArray *)RL_MALLOC(iqmHeader->num_vertexarrays*sizeof(IQMVertexArray));
    //fseek(iqmFile, iqmHeader->ofs_vertexarrays, SEEK_SET);
    //fread(va, sizeof(IQMVertexArray), iqmHeader->num_vertexarrays, iqmFile);
    memcpy(va, fileDataPtr + iqmHeader->ofs_vertexarrays, iqmHeader->num_vertexarrays*sizeof(IQMVertexArray));

    for (unsigned int i = 0; i < iqmHeader->num_vertexarrays; i++) {
        switch (va[i].type) {
            case IQM_POSITION: {
                vertex = (float *)RL_MALLOC(iqmHeader->num_vertexes*3*sizeof(float));
                //fseek(iqmFile, va[a].offset, SEEK_SET);
                //fread(vertex, iqmHeader->num_vertexes*3*sizeof(float), 1, iqmFile);
                memcpy(vertex, fileDataPtr + va[i].offset, iqmHeader->num_vertexes*3*sizeof(float));

                for (unsigned int m = 0; m < iqmHeader->num_meshes; m++) {
                    int vCounter = 0;
                    for (unsigned int i = imesh[m].first_vertex*3; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*3; i++) {
                        model.meshes[m].vertices[vCounter] = vertex[i];
                        if (model.meshes[m].animVertices != nullptr) model.meshes[m].animVertices[vCounter] = vertex[i];
                        vCounter++;
                    }
                }
            } break;
            case IQM_NORMAL: {
                normal = (float *)RL_MALLOC(iqmHeader->num_vertexes*3*sizeof(float));
                //fseek(iqmFile, va[a].offset, SEEK_SET);
                //fread(normal, iqmHeader->num_vertexes*3*sizeof(float), 1, iqmFile);
                memcpy(normal, fileDataPtr + va[i].offset, iqmHeader->num_vertexes*3*sizeof(float));

                for (unsigned int m = 0; m < iqmHeader->num_meshes; m++) {
                    int vCounter = 0;
                    for (unsigned int i = imesh[m].first_vertex*3; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*3; i++) {
                        model.meshes[m].normals[vCounter] = normal[i];
                        if (model.meshes[m].animNormals != nullptr) model.meshes[m].animNormals[vCounter] = normal[i];
                        vCounter++;
                    }
                }
            } break;
            case IQM_TEXCOORD: {
                text = (float *)RL_MALLOC(iqmHeader->num_vertexes*2*sizeof(float));
                //fseek(iqmFile, va[a].offset, SEEK_SET);
                //fread(text, iqmHeader->num_vertexes*2*sizeof(float), 1, iqmFile);
                memcpy(text, fileDataPtr + va[i].offset, iqmHeader->num_vertexes*2*sizeof(float));

                for (unsigned int m = 0; m < iqmHeader->num_meshes; m++) {
                    int vCounter = 0;
                    for (unsigned int i = imesh[m].first_vertex*2; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*2; i++) {
                        model.meshes[m].texcoords[vCounter] = text[i];
                        vCounter++;
                    }
                }
            } break;
            case IQM_BLENDINDEXES: {
                blendi = (char *)RL_MALLOC(iqmHeader->num_vertexes*4*sizeof(char));
                //fseek(iqmFile, va[a].offset, SEEK_SET);
                //fread(blendi, iqmHeader->num_vertexes*4*sizeof(char), 1, iqmFile);
                memcpy(blendi, fileDataPtr + va[i].offset, iqmHeader->num_vertexes*4*sizeof(char));

                for (unsigned int m = 0; m < iqmHeader->num_meshes; m++) {
                    int boneCounter = 0;
                    for (unsigned int i = imesh[m].first_vertex*4; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*4; i++) {
                        model.meshes[m].boneIndices[boneCounter] = blendi[i];
                        boneCounter++;
                    }
                }
            } break;
            case IQM_BLENDWEIGHTS: {
                blendw = (unsigned char *)RL_MALLOC(iqmHeader->num_vertexes*4*sizeof(unsigned char));
                //fseek(iqmFile, va[a].offset, SEEK_SET);
                //fread(blendw, iqmHeader->num_vertexes*4*sizeof(unsigned char), 1, iqmFile);
                memcpy(blendw, fileDataPtr + va[i].offset, iqmHeader->num_vertexes*4*sizeof(unsigned char));

                for (unsigned int m = 0; m < iqmHeader->num_meshes; m++) {
                    int boneCounter = 0;
                    for (unsigned int i = imesh[m].first_vertex*4; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*4; i++) {
                        model.meshes[m].boneWeights[boneCounter] = blendw[i]/255.0f;
                        boneCounter++;
                    }
                }
            } break;
            case IQM_COLOR: {
                color = (unsigned char *)RL_MALLOC(iqmHeader->num_vertexes*4*sizeof(unsigned char));
                //fseek(iqmFile, va[a].offset, SEEK_SET);
                //fread(blendw, iqmHeader->num_vertexes*4*sizeof(unsigned char), 1, iqmFile);
                memcpy(color, fileDataPtr + va[i].offset, iqmHeader->num_vertexes*4*sizeof(unsigned char));

                for (unsigned int m = 0; m < iqmHeader->num_meshes; m++) {
                    model.meshes[m].colors = (unsigned char *)RL_CALLOC(model.meshes[m].vertexCount*4, sizeof(unsigned char));

                    int vCounter = 0;
                    for (unsigned int i = imesh[m].first_vertex*4; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*4; i++) {
                        model.meshes[m].colors[vCounter] = color[i];
                        vCounter++;
                    }
                }
            } break;
        }
    }

    // Bones (joints) data processing
    ijoint = (IQMJoint *)RL_MALLOC(iqmHeader->num_joints*sizeof(IQMJoint));
    //fseek(iqmFile, iqmHeader->ofs_joints, SEEK_SET);
    //fread(ijoint, sizeof(IQMJoint), iqmHeader->num_joints, iqmFile);
    memcpy(ijoint, fileDataPtr + iqmHeader->ofs_joints, iqmHeader->num_joints*sizeof(IQMJoint));

    model.skeleton.boneCount = iqmHeader->num_joints;
    model.skeleton.bones = (BoneInfo *)RL_CALLOC(iqmHeader->num_joints, sizeof(BoneInfo));
    model.skeleton.bindPose = (Transform *)RL_CALLOC(iqmHeader->num_joints, sizeof(Transform));

    for (unsigned int i = 0; i < iqmHeader->num_joints; i++) {
        // Bones
        model.skeleton.bones[i].parent = ijoint[i].parent;
        //fseek(iqmFile, iqmHeader->ofs_text + ijoint[a].name, SEEK_SET);
        //fread(model.bones[a].name, sizeof(char), BONE_NAME_LENGTH, iqmFile);
        memcpy(model.skeleton.bones[i].name, fileDataPtr + iqmHeader->ofs_text + ijoint[i].name, BONE_NAME_LENGTH*sizeof(char));

        // Bind pose (base pose)
        model.skeleton.bindPose[i].translation.x = ijoint[i].translate[0];
        model.skeleton.bindPose[i].translation.y = ijoint[i].translate[1];
        model.skeleton.bindPose[i].translation.z = ijoint[i].translate[2];

        model.skeleton.bindPose[i].rotation.x = ijoint[i].rotate[0];
        model.skeleton.bindPose[i].rotation.y = ijoint[i].rotate[1];
        model.skeleton.bindPose[i].rotation.z = ijoint[i].rotate[2];
        model.skeleton.bindPose[i].rotation.w = ijoint[i].rotate[3];

        model.skeleton.bindPose[i].scale.x = ijoint[i].scale[0];
        model.skeleton.bindPose[i].scale.y = ijoint[i].scale[1];
        model.skeleton.bindPose[i].scale.z = ijoint[i].scale[2];
    }

    BuildPoseFromParentJoints(model.skeleton.bones, model.skeleton.boneCount, model.skeleton.bindPose);

    // Initialize runtime animation data: current pose and bone matrices
    model.currentPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
    model.boneMatrices = (Matrix *)RL_CALLOC(model.skeleton.boneCount, sizeof(Matrix));
    for (unsigned int j = 0; j < model.skeleton.boneCount; j++) model.boneMatrices[j] = MatrixIdentity();

    UnloadFileData(fileData);

    RL_FREE(imesh);
    RL_FREE(tri);
    RL_FREE(va);
    RL_FREE(vertex);
    RL_FREE(normal);
    RL_FREE(text);
    RL_FREE(blendi);
    RL_FREE(blendw);
    RL_FREE(ijoint);
    RL_FREE(color);

    return model;
}

// Load IQM animation data
static ModelAnimation *LoadModelAnimationsIQM(const char *fileName, int *animCount) {
    #define IQM_MAGIC       "INTERQUAKEMODEL"   // IQM file magic number
    #define IQM_VERSION     2                   // only IQM version 2 supported

    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);
    unsigned char *fileDataPtr = fileData;

    typedef struct IQMHeader {
        char magic[16];
        unsigned int version;
        unsigned int dataSize;
        unsigned int flags;
        unsigned int num_text, ofs_text;
        unsigned int num_meshes, ofs_meshes;
        unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
        unsigned int num_triangles, ofs_triangles, ofs_adjacency;
        unsigned int num_joints, ofs_joints;
        unsigned int num_poses, ofs_poses;
        unsigned int num_anims, ofs_anims;
        unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
        unsigned int num_comment, ofs_comment;
        unsigned int num_extensions, ofs_extensions;
    } IQMHeader;

    typedef struct IQMJoint {
        unsigned int name;
        int parent;
        float translate[3], rotate[4], scale[3];
    } IQMJoint;

    typedef struct IQMPose {
        int parent;
        unsigned int mask;
        float channeloffset[10];
        float channelscale[10];
    } IQMPose;

    typedef struct IQMAnim {
        unsigned int name;
        unsigned int first_frame, num_frames;
        float framerate;
        unsigned int flags;
    } IQMAnim;

    // In case file can not be read, return an empty model
    if (fileDataPtr == nullptr) return nullptr;

    // Read IQM header
    IQMHeader *iqmHeader = (IQMHeader *)fileDataPtr;

    if (memcmp(iqmHeader->magic, IQM_MAGIC, sizeof(IQM_MAGIC)) != 0) {
        TraceLog(LOG_WARNING, "MODEL: [%s] IQM file is not a valid model", fileName);
        UnloadFileData(fileData);
        return nullptr;
    }

    if (iqmHeader->version != IQM_VERSION) {
        TraceLog(LOG_WARNING, "MODEL: [%s] IQM file version not supported (%i)", fileName, iqmHeader->version);
        UnloadFileData(fileData);
        return nullptr;
    }

    // Get bones data
    IQMPose *poses = (IQMPose *)RL_MALLOC(iqmHeader->num_poses*sizeof(IQMPose));
    //fseek(iqmFile, iqmHeader->ofs_poses, SEEK_SET);
    //fread(poses, sizeof(IQMPose), iqmHeader->num_poses, iqmFile);
    memcpy(poses, fileDataPtr + iqmHeader->ofs_poses, iqmHeader->num_poses*sizeof(IQMPose));

    // Get animations data
    *animCount = iqmHeader->num_anims;
    IQMAnim *anim = (IQMAnim *)RL_MALLOC(iqmHeader->num_anims*sizeof(IQMAnim));
    //fseek(iqmFile, iqmHeader->ofs_anims, SEEK_SET);
    //fread(anim, sizeof(IQMAnim), iqmHeader->num_anims, iqmFile);
    memcpy(anim, fileDataPtr + iqmHeader->ofs_anims, iqmHeader->num_anims*sizeof(IQMAnim));

    ModelAnimation *animations = (ModelAnimation *)RL_CALLOC(iqmHeader->num_anims, sizeof(ModelAnimation));

    // frameposes
    unsigned short *framedata = (unsigned short *)RL_CALLOC(iqmHeader->num_frames*iqmHeader->num_framechannels, sizeof(unsigned short));
    //fseek(iqmFile, iqmHeader->ofs_frames, SEEK_SET);
    //fread(framedata, sizeof(unsigned short), iqmHeader->num_frames*iqmHeader->num_framechannels, iqmFile);
    memcpy(framedata, fileDataPtr + iqmHeader->ofs_frames, iqmHeader->num_frames*iqmHeader->num_framechannels*sizeof(unsigned short));

    // joints
    IQMJoint *joints = (IQMJoint *)RL_CALLOC(iqmHeader->num_joints, sizeof(IQMJoint));
    memcpy(joints, fileDataPtr + iqmHeader->ofs_joints, iqmHeader->num_joints*sizeof(IQMJoint));

    for (unsigned int a = 0; a < iqmHeader->num_anims; a++) {
        animations[a].boneCount = iqmHeader->num_poses;
        BoneInfo *bones = (BoneInfo *)RL_CALLOC(iqmHeader->num_poses, sizeof(BoneInfo));

        animations[a].keyframeCount = anim[a].num_frames;
        animations[a].keyframePoses = (Transform **)RL_CALLOC(anim[a].num_frames, sizeof(Transform *));
        memcpy(animations[a].name, fileDataPtr + iqmHeader->ofs_text + anim[a].name, 32);
        // TODO: Use animation framerate data?
        //animations[a].framerate = anim.framerate;

        TraceLog(LOG_INFO, "MODEL: [%s] Loaded animation: %s | Frames: %d | Framerate: %f", fileName, animations[a].name, animations[a].keyframeCount, anim[a].framerate);

        for (unsigned int j = 0; j < iqmHeader->num_poses; j++) {
            bones[j].parent = poses[j].parent;

            // NOTE: No need to store bones names, bones only required to generate keyframe poses
            //if (iqmHeader->num_joints > 0) memcpy(bones[j].name, fileDataPtr + iqmHeader->ofs_text + joints[j].name, BONE_NAME_LENGTH*sizeof(char));
            //else memcpy(bones[j].name, "ANIMJOINTNAME", 13); // Default bone name otherwise
        }

        for (unsigned int j = 0; j < anim[a].num_frames; j++)
            animations[a].keyframePoses[j] = (Transform *)RL_MALLOC(iqmHeader->num_poses*sizeof(Transform));

        int dcounter = anim[a].first_frame*iqmHeader->num_framechannels;

        for (unsigned int frame = 0; frame < anim[a].num_frames; frame++) {
            for (unsigned int i = 0; i < iqmHeader->num_poses; i++) {
                animations[a].keyframePoses[frame][i].translation.x = poses[i].channeloffset[0];

                if (poses[i].mask & 0x01) {
                    animations[a].keyframePoses[frame][i].translation.x += framedata[dcounter]*poses[i].channelscale[0];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].translation.y = poses[i].channeloffset[1];

                if (poses[i].mask & 0x02) {
                    animations[a].keyframePoses[frame][i].translation.y += framedata[dcounter]*poses[i].channelscale[1];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].translation.z = poses[i].channeloffset[2];

                if (poses[i].mask & 0x04) {
                    animations[a].keyframePoses[frame][i].translation.z += framedata[dcounter]*poses[i].channelscale[2];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].rotation.x = poses[i].channeloffset[3];

                if (poses[i].mask & 0x08) {
                    animations[a].keyframePoses[frame][i].rotation.x += framedata[dcounter]*poses[i].channelscale[3];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].rotation.y = poses[i].channeloffset[4];

                if (poses[i].mask & 0x10) {
                    animations[a].keyframePoses[frame][i].rotation.y += framedata[dcounter]*poses[i].channelscale[4];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].rotation.z = poses[i].channeloffset[5];

                if (poses[i].mask & 0x20) {
                    animations[a].keyframePoses[frame][i].rotation.z += framedata[dcounter]*poses[i].channelscale[5];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].rotation.w = poses[i].channeloffset[6];

                if (poses[i].mask & 0x40) {
                    animations[a].keyframePoses[frame][i].rotation.w += framedata[dcounter]*poses[i].channelscale[6];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].scale.x = poses[i].channeloffset[7];

                if (poses[i].mask & 0x80) {
                    animations[a].keyframePoses[frame][i].scale.x += framedata[dcounter]*poses[i].channelscale[7];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].scale.y = poses[i].channeloffset[8];

                if (poses[i].mask & 0x100) {
                    animations[a].keyframePoses[frame][i].scale.y += framedata[dcounter]*poses[i].channelscale[8];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].scale.z = poses[i].channeloffset[9];

                if (poses[i].mask & 0x200) {
                    animations[a].keyframePoses[frame][i].scale.z += framedata[dcounter]*poses[i].channelscale[9];
                    dcounter++;
                }

                animations[a].keyframePoses[frame][i].rotation = QuaternionNormalize(animations[a].keyframePoses[frame][i].rotation);
            }
        }

        // Build frameposes
        for (unsigned int frame = 0; frame < anim[a].num_frames; frame++) {
            for (unsigned int i = 0; i < animations[a].boneCount; i++) {
                if (bones[i].parent >= 0) {
                    animations[a].keyframePoses[frame][i].rotation = QuaternionMultiply(animations[a].keyframePoses[frame][bones[i].parent].rotation, animations[a].keyframePoses[frame][i].rotation);
                    animations[a].keyframePoses[frame][i].translation = Vector3RotateByQuaternion(animations[a].keyframePoses[frame][i].translation, animations[a].keyframePoses[frame][bones[i].parent].rotation);
                    animations[a].keyframePoses[frame][i].translation = Vector3Add(animations[a].keyframePoses[frame][i].translation, animations[a].keyframePoses[frame][bones[i].parent].translation);
                    animations[a].keyframePoses[frame][i].scale = Vector3Multiply(animations[a].keyframePoses[frame][i].scale, animations[a].keyframePoses[frame][bones[i].parent].scale);
                }
            }
        }

        RL_FREE(bones);
    }

    UnloadFileData(fileData);

    RL_FREE(joints);
    RL_FREE(framedata);
    RL_FREE(poses);
    RL_FREE(anim);

    return animations;
}

#endif

#if SUPPORT_FILEFORMAT_GLTF
// Load file data callback for cgltf
static cgltf_result LoadFileGLTFCallback(const struct cgltf_memory_options *memoryOptions, const struct cgltf_file_options *fileOptions, const char *path, cgltf_size *size, void **data) {
    int filesize;
    unsigned char *filedata = LoadFileData(path, &filesize);

    if (filedata == nullptr) return cgltf_result_io_error;

    *size = filesize;
    *data = filedata;

    return cgltf_result_success;
}

// Release file data callback for cgltf
static void ReleaseFileGLTFCallback(const struct cgltf_memory_options *memoryOptions, const struct cgltf_file_options *fileOptions, void *data, cgltf_size size) {
    UnloadFileData((unsigned char *)data);
}

// Load image from different glTF provided methods (uri, path, buffer_view)
static Image LoadImageFromCgltfImage(cgltf_image *cgltfImage, const char *texPath) {
    Image image = {0};

    if (cgltfImage == nullptr) return image;

    if (cgltfImage->uri != nullptr) {   // Check if image data is provided as an uri (base64 or path)
        if ((strlen(cgltfImage->uri) > 5) &&
            (cgltfImage->uri[0] == 'd') &&
            (cgltfImage->uri[1] == 'a') &&
            (cgltfImage->uri[2] == 't') &&
            (cgltfImage->uri[3] == 'a') &&
            (cgltfImage->uri[4] == ':')     // Check if image is provided as base64 text data
        ) {
            // Data URI Format: data:<mediatype>;base64,<data>

            // Find the comma
            int i = 0;
            while ((cgltfImage->uri[i] != ',') && (cgltfImage->uri[i] != 0)) i++;

            if (cgltfImage->uri[i] == 0) TraceLog(LOG_WARNING, "IMAGE: glTF data URI is not a valid image");
            else {
                int base64Size = (int)strlen(cgltfImage->uri + i + 1);
                while (cgltfImage->uri[i + base64Size] == '=') base64Size--;    // Ignore optional paddings
                int numberOfEncodedBits = base64Size*6 - (base64Size*6)%8;      // Encoded bits minus extra bits, so it becomes a multiple of 8 bits
                int outSize = numberOfEncodedBits >> 3;                         // Actual encoded bytes
                void *data = nullptr;

                cgltf_options options = { 0 };
                options.file.read = LoadFileGLTFCallback;
                options.file.release = ReleaseFileGLTFCallback;
                cgltf_result result = cgltf_load_buffer_base64(&options, outSize, cgltfImage->uri + i + 1, &data);

                if (result == cgltf_result_success) {
                    image = LoadImageFromMemory(".png", (unsigned char *)data, outSize);
                    RL_FREE(data);
                }
            }
        } else {   // Check if image is provided as image path
            image = LoadImage(TextFormat("%s/%s", texPath, cgltfImage->uri));
        }
    } else if ((cgltfImage->buffer_view != nullptr) && (cgltfImage->buffer_view->buffer->data != nullptr)) { // Check if image is provided as data buffer
        unsigned char *data = (unsigned char *)RL_MALLOC(cgltfImage->buffer_view->size);
        int offset = (int)cgltfImage->buffer_view->offset;
        int stride = (int)cgltfImage->buffer_view->stride? (int)cgltfImage->buffer_view->stride : 1;

        // Copy buffer data to memory for loading
        for (unsigned int i = 0; i < cgltfImage->buffer_view->size; i++) {
            data[i] = ((unsigned char *)cgltfImage->buffer_view->buffer->data)[offset];
            offset += stride;
        }

        // Check mime_type for image: (cgltfImage->mime_type == "image/png")
        // NOTE: Detected that some models define mime_type as "image\\/png"
        if ((strcmp(cgltfImage->mime_type, "image\\/png") == 0) || (strcmp(cgltfImage->mime_type, "image/png") == 0)) {
            image = LoadImageFromMemory(".png", data, (int)cgltfImage->buffer_view->size);
        } else if ((strcmp(cgltfImage->mime_type, "image\\/jpeg") == 0) || (strcmp(cgltfImage->mime_type, "image/jpeg") == 0)) {
            image = LoadImageFromMemory(".jpg", data, (int)cgltfImage->buffer_view->size);
        } else TraceLog(LOG_WARNING, "MODEL: glTF image data MIME type not recognized", TextFormat("%s/%s", texPath, cgltfImage->uri));

        RL_FREE(data);
    }

    return image;
}

// Load bone info from GLTF skin data
static BoneInfo *LoadBoneInfoGLTF(cgltf_skin skin, unsigned int *boneCount) {
    *boneCount = (unsigned int)skin.joints_count;
    BoneInfo *bones = (BoneInfo *)RL_CALLOC(skin.joints_count, sizeof(BoneInfo));

    for (unsigned int i = 0; i < skin.joints_count; i++) {
        cgltf_node node = *skin.joints[i];
        if (node.name != nullptr) snprintf(bones[i].name, sizeof(bones[i].name), "%s", node.name);

        // Find parent bone index by walking up the node tree past any
        // non-joint ancestors (intermediate transform nodes used by some
        // DCC exporters), until we hit a node that is also in skin.joints.
        int parentIndex = -1;
        cgltf_node *ancestor = node.parent;
        while (ancestor != nullptr && parentIndex == -1) {
            for (unsigned int j = 0; j < skin.joints_count; j++) {
                if (skin.joints[j] == ancestor) { parentIndex = (int)j; break; }
            }
            if (parentIndex == -1) ancestor = ancestor->parent;
        }

        bones[i].parent = parentIndex;
    }

    return bones;
}

// Load glTF file into model struct, .gltf and .glb supported
static Model LoadGLTF(const char *fileName) {
    /*********************************************************************************************

        Function implemented by Wilhem Barbier(@wbrbr), with modifications by Tyler Bezera(@gamerfiend)
        Transform handling implemented by Paul Melis (@paulmelis)
        Reviewed by Ramon Santamaria (@raysan5)

        FEATURES:
          - Supports .gltf and .glb files
          - Supports embedded (base64) or external textures
          - Supports PBR metallic/roughness flow, loads material textures, values and colors
                     PBR specular/glossiness flow and extended texture flows not supported
          - Supports multiple meshes per model (every primitives is loaded as a separate mesh)
          - Supports basic animations
          - Transforms, including parent-child relations, are applied on the mesh data,
            but the hierarchy is not kept (as it can't be represented)
          - Mesh instances in the glTF file (a.e. same mesh linked from multiple nodes)
            are turned into separate raylib Meshes

        RESTRICTIONS:
          - Only triangle meshes supported
          - Vertex attribute types and formats supported:
              > Vertices (position): vec3: float
              > Normals: vec3: float
              > Texcoords: vec2: float
              > Colors: vec4: u8, u16, f32 (normalized)
              > Indices: u16, u32 (truncated to u16)
          - Scenes defined in the glTF file are ignored. All nodes in the file are used

    ***********************************************************************************************/

    // Macro to simplify attributes loading code
    #define LOAD_ATTRIBUTE(accesor, numComp, srcType, dstPtr) LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, srcType)

    #define LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, dstType) \
    { \
        int n = 0; \
        srcType *buffer = (srcType *)accesor->buffer_view->buffer->data + accesor->buffer_view->offset/sizeof(srcType) + accesor->offset/sizeof(srcType); \
        for (unsigned int k = 0; k < accesor->count; k++) {\
            for (int l = 0; l < numComp; l++) {\
                dstPtr[numComp*k + l] = (dstType)buffer[n + l];\
            }\
            n += (int)(accesor->stride/sizeof(srcType));\
        }\
    }

    Model model = {0};

    // glTF file loading
    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);

    if (fileData == nullptr) return model;

    // glTF data loading
    cgltf_options options = { 0 };
    options.file.read = LoadFileGLTFCallback;
    options.file.release = ReleaseFileGLTFCallback;
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

    if (result == cgltf_result_success) {
        if (data->file_type == cgltf_file_type_glb) TraceLog(LOG_INFO, "MODEL: [%s] Model basic data (glb) loaded successfully", fileName);
        else if (data->file_type == cgltf_file_type_gltf) TraceLog(LOG_INFO, "MODEL: [%s] Model basic data (glTF) loaded successfully", fileName);
        else TraceLog(LOG_WARNING, "MODEL: [%s] Model format not recognized", fileName);

        TraceLog(LOG_INFO, "    > Meshes count: %i", data->meshes_count);
        TraceLog(LOG_INFO, "    > Materials count: %i (+1 default)", data->materials_count);
        TraceLog(LOG_DEBUG, "    > Buffers count: %i", data->buffers_count);
        TraceLog(LOG_DEBUG, "    > Images count: %i", data->images_count);
        TraceLog(LOG_DEBUG, "    > Textures count: %i", data->textures_count);

        // Force reading data buffers (fills buffer_view->buffer->data)
        // NOTE: If an uri is defined to base64 data or external path, it's automatically loaded
        result = cgltf_load_buffers(&options, data, fileName);
        if (result != cgltf_result_success) TraceLog(LOG_INFO, "MODEL: [%s] Failed to load mesh/material buffers", fileName);

        int primitivesCount = 0;
        bool dracoCompression = false;

        // NOTE: Load every primitive in the glTF as a separate raylib Mesh
        // Determine total number of meshes needed from the node hierarchy
        for (unsigned int i = 0; i < data->nodes_count; i++) {
            cgltf_node *node = &(data->nodes[i]);
            cgltf_mesh *mesh = node->mesh;
            if (!mesh) continue;

            for (unsigned int p = 0; p < mesh->primitives_count; p++) {
                if (mesh->primitives[p].has_draco_mesh_compression) {
                    dracoCompression = true;
                    TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load mesh data, Draco compression not supported", fileName);
                    break;
                }

                if (mesh->primitives[p].type == cgltf_primitive_type_triangles) primitivesCount++;
            }
        }

        if (dracoCompression) {
            TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load glTF data", fileName);
            cgltf_free(data);
            UnloadFileData(fileData);
            return model;
        }

        TraceLog(LOG_DEBUG, "    > Primitives (triangles only) count based on hierarchy : %i", primitivesCount);

        // Load our model data: meshes and materials
        model.meshCount = primitivesCount;
        model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));

        // NOTE: Keep an extra slot for default material, in case some mesh requires it
        model.materialCount = (int)data->materials_count + 1;
        model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
        model.materials[0] = LoadMaterialDefault();     // Load default material (index: 0)

        // Load mesh-material indices, by default all meshes are mapped to material index: 0
        model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));

        // Load materials data
        //----------------------------------------------------------------------------------------------------
        for (unsigned int i = 0, j = 1; i < data->materials_count; i++, j++) {
            model.materials[j] = LoadMaterialDefault();
            const char *texPath = GetDirectoryPath(fileName);

            // Check glTF material flow: PBR metallic/roughness flow
            // NOTE: Alternatively, materials can follow PBR specular/glossiness flow
            if (data->materials[i].has_pbr_metallic_roughness) {
                // Load base color texture (albedo)
                if (data->materials[i].pbr_metallic_roughness.base_color_texture.texture) {
                    Image imAlbedo = LoadImageFromCgltfImage(data->materials[i].pbr_metallic_roughness.base_color_texture.texture->image, texPath);
                    if (imAlbedo.data != nullptr) {
                        model.materials[j].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(imAlbedo);
                        UnloadImage(imAlbedo);
                    }
                }
                // Load base color factor (tint)
                model.materials[j].maps[MATERIAL_MAP_ALBEDO].color.r = (unsigned char)(data->materials[i].pbr_metallic_roughness.base_color_factor[0]*255);
                model.materials[j].maps[MATERIAL_MAP_ALBEDO].color.g = (unsigned char)(data->materials[i].pbr_metallic_roughness.base_color_factor[1]*255);
                model.materials[j].maps[MATERIAL_MAP_ALBEDO].color.b = (unsigned char)(data->materials[i].pbr_metallic_roughness.base_color_factor[2]*255);
                model.materials[j].maps[MATERIAL_MAP_ALBEDO].color.a = (unsigned char)(data->materials[i].pbr_metallic_roughness.base_color_factor[3]*255);

                // Load metallic/roughness texture
                if (data->materials[i].pbr_metallic_roughness.metallic_roughness_texture.texture) {
                    Image imMetallicRoughness = LoadImageFromCgltfImage(data->materials[i].pbr_metallic_roughness.metallic_roughness_texture.texture->image, texPath);
                    if (imMetallicRoughness.data != nullptr) {
                        Image imMetallic = { 0 };
                        Image imRoughness = { 0 };

                        imMetallic.data = RL_MALLOC(imMetallicRoughness.width*imMetallicRoughness.height);
                        imRoughness.data = RL_MALLOC(imMetallicRoughness.width*imMetallicRoughness.height);

                        imMetallic.width = imRoughness.width = imMetallicRoughness.width;
                        imMetallic.height = imRoughness.height = imMetallicRoughness.height;

                        imMetallic.format = imRoughness.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
                        imMetallic.mipmaps = imRoughness.mipmaps = 1;

                        for (int x = 0; x < imRoughness.width; x++) {
                            for (int y = 0; y < imRoughness.height; y++) {
                                Color color = GetImageColor(imMetallicRoughness, x, y);

                                ((unsigned char *)imRoughness.data)[y*imRoughness.width + x] = color.g; // Roughness color channel
                                ((unsigned char *)imMetallic.data)[y*imMetallic.width + x] = color.b; // Metallic color channel
                            }
                        }

                        model.materials[j].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTextureFromImage(imRoughness);
                        model.materials[j].maps[MATERIAL_MAP_METALNESS].texture = LoadTextureFromImage(imMetallic);

                        UnloadImage(imRoughness);
                        UnloadImage(imMetallic);
                        UnloadImage(imMetallicRoughness);
                    }

                    // Load metallic/roughness material properties
                    float roughness = data->materials[i].pbr_metallic_roughness.roughness_factor;
                    model.materials[j].maps[MATERIAL_MAP_ROUGHNESS].value = roughness;

                    float metallic = data->materials[i].pbr_metallic_roughness.metallic_factor;
                    model.materials[j].maps[MATERIAL_MAP_METALNESS].value = metallic;
                }

                // Load normal texture
                if (data->materials[i].normal_texture.texture) {
                    Image imNormal = LoadImageFromCgltfImage(data->materials[i].normal_texture.texture->image, texPath);
                    if (imNormal.data != nullptr) {
                        model.materials[j].maps[MATERIAL_MAP_NORMAL].texture = LoadTextureFromImage(imNormal);
                        UnloadImage(imNormal);
                    }
                }

                // Load ambient occlusion texture
                if (data->materials[i].occlusion_texture.texture) {
                    Image imOcclusion = LoadImageFromCgltfImage(data->materials[i].occlusion_texture.texture->image, texPath);
                    if (imOcclusion.data != nullptr) {
                        model.materials[j].maps[MATERIAL_MAP_OCCLUSION].texture = LoadTextureFromImage(imOcclusion);
                        UnloadImage(imOcclusion);
                    }
                }

                // Load emissive texture
                if (data->materials[i].emissive_texture.texture) {
                    Image imEmissive = LoadImageFromCgltfImage(data->materials[i].emissive_texture.texture->image, texPath);
                    if (imEmissive.data != nullptr) {
                        model.materials[j].maps[MATERIAL_MAP_EMISSION].texture = LoadTextureFromImage(imEmissive);
                        UnloadImage(imEmissive);
                    }

                    // Load emissive color factor
                    model.materials[j].maps[MATERIAL_MAP_EMISSION].color.r = (unsigned char)(data->materials[i].emissive_factor[0]*255);
                    model.materials[j].maps[MATERIAL_MAP_EMISSION].color.g = (unsigned char)(data->materials[i].emissive_factor[1]*255);
                    model.materials[j].maps[MATERIAL_MAP_EMISSION].color.b = (unsigned char)(data->materials[i].emissive_factor[2]*255);
                    model.materials[j].maps[MATERIAL_MAP_EMISSION].color.a = 255;
                }
            }

            // Other possible materials not supported by raylib pipeline:
            // has_clearcoat, has_transmission, has_volume, has_ior, has specular, has_sheen
        }
        //----------------------------------------------------------------------------------------------------

        // Load meshes data
        //
        // NOTE: Visit each node in the hierarchy and process any mesh linked from it
        //  - Each primitive within a glTF node becomes a raylib Mesh
        //  - The local-to-world transform of each node is used to transform the points/normals/tangents of the created Mesh(es)
        //  - Any glTF mesh linked from more than one Node (a.e. instancing) is turned into multiple Mesh's, as each Node will have its own transform applied
        //
        // WARNING: The code below disregards the scenes defined in the file, all nodes are used
        //----------------------------------------------------------------------------------------------------
        int meshIndex = 0;
        for (unsigned int i = 0; i < data->nodes_count; i++) {
            cgltf_node *node = &(data->nodes[i]);

            cgltf_mesh *mesh = node->mesh;
            if (!mesh) continue;

            cgltf_float worldTransform[16];
            cgltf_node_transform_world(node, worldTransform);

            Matrix worldMatrix = {
                worldTransform[0], worldTransform[4], worldTransform[8], worldTransform[12],
                worldTransform[1], worldTransform[5], worldTransform[9], worldTransform[13],
                worldTransform[2], worldTransform[6], worldTransform[10], worldTransform[14],
                worldTransform[3], worldTransform[7], worldTransform[11], worldTransform[15]
            };

            Matrix worldMatrixNormals = MatrixTranspose(MatrixInvert(worldMatrix));

            for (unsigned int p = 0; p < mesh->primitives_count; p++) {
                // NOTE: Only support primitives defined by triangles
                // Other alternatives: points, lines, line_strip, triangle_strip
                if (mesh->primitives[p].type != cgltf_primitive_type_triangles) continue;

                // NOTE: Attributes data could be provided in several data formats (8, 8u, 16u, 32...),
                // Only some formats for each attribute type are supported, read info at the top of this function!

                for (unsigned int j = 0; j < mesh->primitives[p].attributes_count; j++) {
                    // Check the different attributes for every primitive
                    if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_position) {     // POSITION, vec3, float
                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        // WARNING: SPECS: POSITION accessor MUST have its min and max properties defined

                        if (model.meshes[meshIndex].vertices != nullptr) TraceLog(LOG_WARNING, "MODEL: [%s] Vertices attribute data already loaded", fileName);
                        else {
                            if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_32f)) {
                                // Init raylib mesh vertices to copy glTF attribute data
                                model.meshes[meshIndex].vertexCount = (int)attribute->count;
                                model.meshes[meshIndex].vertices = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load 3 components of float data type into mesh.vertices
                                LOAD_ATTRIBUTE(attribute, 3, float, model.meshes[meshIndex].vertices)

                                // Transform the vertices
                                float *vertices = model.meshes[meshIndex].vertices;
                                for (unsigned int k = 0; k < attribute->count; k++)
                                {
                                    Vector3 vt = Vector3Transform((Vector3){ vertices[3*k], vertices[3*k+1], vertices[3*k+2] }, worldMatrix);
                                    vertices[3*k] = vt.x;
                                    vertices[3*k+1] = vt.y;
                                    vertices[3*k+2] = vt.z;
                                }
                            } else if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_16u)) {
                                // Init raylib mesh vertices to copy glTF attribute data
                                model.meshes[meshIndex].vertexCount = (int)attribute->count;
                                model.meshes[meshIndex].vertices = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned short *temp = (unsigned short *)RL_MALLOC(attribute->count*3*sizeof(unsigned short));
                                LOAD_ATTRIBUTE(attribute, 3, unsigned short, temp);

                                // Convert data to raylib vertex data type (float) the matrix will scale it to the correct size as a float
                                for (unsigned int t = 0; t < attribute->count*3; t++) model.meshes[meshIndex].vertices[t] = (float)temp[t];

                                RL_FREE(temp);

                                // Transform the vertices
                                float *vertices = model.meshes[meshIndex].vertices;
                                for (unsigned int k = 0; k < attribute->count; k++) {
                                    Vector3 vt = Vector3Transform((Vector3){ vertices[3*k], vertices[3*k + 1], vertices[3*k + 2] }, worldMatrix);
                                    vertices[3*k] = vt.x;
                                    vertices[3*k + 1] = vt.y;
                                    vertices[3*k + 2] = vt.z;
                                }
                            } else if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_16)) {
                                // Init raylib mesh vertices to copy glTF attribute data
                                model.meshes[meshIndex].vertexCount = (int)attribute->count;
                                model.meshes[meshIndex].vertices = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                short *temp = (short *)RL_MALLOC(attribute->count*3*sizeof(short));
                                LOAD_ATTRIBUTE(attribute, 3, short, temp);

                                // Convert data to raylib vertex data type (float) the matrix will scale it to the correct size as a float
                                for (unsigned int t = 0; t < attribute->count*3; t++) model.meshes[meshIndex].vertices[t] = (float)temp[t];

                                RL_FREE(temp);

                                // Transform the vertices
                                float *vertices = model.meshes[meshIndex].vertices;
                                for (unsigned int k = 0; k < attribute->count; k++) {
                                    Vector3 vt = Vector3Transform((Vector3){ vertices[3*k], vertices[3*k + 1], vertices[3*k + 2] }, worldMatrix);
                                    vertices[3*k] = vt.x;
                                    vertices[3*k + 1] = vt.y;
                                    vertices[3*k + 2] = vt.z;
                                }
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Vertices attribute data format not supported, use vec3 float", fileName);
                        }
                    } else if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_normal) { // NORMAL, vec3, float
                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        if (model.meshes[meshIndex].normals != nullptr) TraceLog(LOG_WARNING, "MODEL: [%s] Normals attribute data already loaded", fileName);
                        else {
                            if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_32f)) {
                                // Init raylib mesh normals to copy glTF attribute data
                                model.meshes[meshIndex].normals = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load 3 components of float data type into mesh.normals
                                LOAD_ATTRIBUTE(attribute, 3, float, model.meshes[meshIndex].normals)

                                // Transform the normals
                                float *normals = model.meshes[meshIndex].normals;
                                for (unsigned int k = 0; k < attribute->count; k++) {
                                    Vector3 nt = Vector3Transform((Vector3){ normals[3*k], normals[3*k+1], normals[3*k+2] }, worldMatrixNormals);
                                    normals[3*k] = nt.x;
                                    normals[3*k+1] = nt.y;
                                    normals[3*k+2] = nt.z;
                                }
                            } else if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_16)) {
                                // Init raylib mesh normals to copy glTF attribute data
                                model.meshes[meshIndex].normals = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                short *temp = (short *)RL_MALLOC(attribute->count*3*sizeof(short));
                                LOAD_ATTRIBUTE(attribute, 3, short, temp);

                                // Convert data to raylib normal data type (float)
                                for (unsigned int t = 0; t < attribute->count*3; t++) model.meshes[meshIndex].normals[t] = (float)temp[t];

                                RL_FREE(temp);

                                // Transform the normals
                                float *normals = model.meshes[meshIndex].normals;
                                for (unsigned int k = 0; k < attribute->count; k++) {
                                    Vector3 nt = Vector3Normalize(Vector3Transform((Vector3){ normals[3*k], normals[3*k + 1], normals[3*k + 2] }, worldMatrixNormals));
                                    normals[3*k] = nt.x;
                                    normals[3*k + 1] = nt.y;
                                    normals[3*k + 2] = nt.z;
                                }
                            } else if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_8u)) {
                                // Init raylib mesh normals to copy glTF attribute data
                                model.meshes[meshIndex].normals = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned char *temp = (unsigned char *)RL_MALLOC(attribute->count*3*sizeof(unsigned char));
                                LOAD_ATTRIBUTE(attribute, 3, unsigned char, temp);

                                // Convert data to raylib normal data type (float)
                                for (unsigned int t = 0; t < attribute->count*3; t++) model.meshes[meshIndex].normals[t] = (float)temp[t];

                                RL_FREE(temp);

                                // Transform the normals
                                float *normals = model.meshes[meshIndex].normals;
                                for (unsigned int k = 0; k < attribute->count; k++) {
                                    Vector3 nt = Vector3Normalize(Vector3Transform((Vector3){ normals[3*k], normals[3*k + 1], normals[3*k + 2] }, worldMatrixNormals));
                                    normals[3*k] = nt.x;
                                    normals[3*k + 1] = nt.y;
                                    normals[3*k + 2] = nt.z;
                                }
                            } else if ((attribute->type == cgltf_type_vec3) && (attribute->component_type == cgltf_component_type_r_8)) {
                                // Init raylib mesh normals to copy glTF attribute data
                                model.meshes[meshIndex].normals = (float *)RL_MALLOC(attribute->count*3*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                char *temp = (char *)RL_MALLOC(attribute->count*3*sizeof(char));
                                LOAD_ATTRIBUTE(attribute, 3, char, temp);

                                // Convert data to raylib normal data type (float)
                                for (unsigned int t = 0; t < attribute->count*3; t++) model.meshes[meshIndex].normals[t] = (float)temp[t];

                                RL_FREE(temp);

                                // Transform the normals
                                float *normals = model.meshes[meshIndex].normals;
                                for (unsigned int k = 0; k < attribute->count; k++)
                                {
                                    Vector3 nt = Vector3Normalize(Vector3Transform((Vector3){ normals[3*k], normals[3*k + 1], normals[3*k + 2] }, worldMatrixNormals));
                                    normals[3*k] = nt.x;
                                    normals[3*k + 1] = nt.y;
                                    normals[3*k + 2] = nt.z;
                                }
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Normals attribute data format not supported, use vec3 float", fileName);
                        }
                    } else if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_tangent) { // TANGENT, vec4, float, w is tangent basis sign
                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        if (model.meshes[meshIndex].tangents != nullptr) TraceLog(LOG_WARNING, "MODEL: [%s] Tangents attribute data already loaded", fileName);
                        else {
                            if ((attribute->type == cgltf_type_vec4) && (attribute->component_type == cgltf_component_type_r_32f)) {
                                // Init raylib mesh tangent to copy glTF attribute data
                                model.meshes[meshIndex].tangents = (float *)RL_MALLOC(attribute->count*4*sizeof(float));

                                // Load 4 components of float data type into mesh.tangents
                                LOAD_ATTRIBUTE(attribute, 4, float, model.meshes[meshIndex].tangents)

                                // Transform the tangents
                                float *tangents = model.meshes[meshIndex].tangents;
                                for (unsigned int k = 0; k < attribute->count; k++) {
                                    Vector3 tt = Vector3Transform((Vector3){ tangents[4*k], tangents[4*k+1], tangents[4*k+2] }, worldMatrix);
                                    tangents[4*k] = tt.x;
                                    tangents[4*k+1] = tt.y;
                                    tangents[4*k+2] = tt.z;
                                }
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Tangents attribute data format not supported, use vec4 float", fileName);
                        }
                    } else if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_texcoord) { // TEXCOORD_n, vec2, float/u8n/u16n
                        // Support up to 2 texture coordinates attributes
                        float *texcoordPtr = nullptr;

                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        if (attribute->type == cgltf_type_vec2) {
                            if (attribute->component_type == cgltf_component_type_r_32f) { // vec2, float
                                // Init raylib mesh texcoords to copy glTF attribute data
                                texcoordPtr = (float *)RL_MALLOC(attribute->count*2*sizeof(float));

                                // Load 3 components of float data type into mesh.texcoords
                                LOAD_ATTRIBUTE(attribute, 2, float, texcoordPtr)
                            } else if (attribute->component_type == cgltf_component_type_r_8u) { // vec2, u8n
                                // Init raylib mesh texcoords to copy glTF attribute data
                                texcoordPtr = (float *)RL_MALLOC(attribute->count*2*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned char *temp = (unsigned char *)RL_MALLOC(attribute->count*2*sizeof(unsigned char));
                                LOAD_ATTRIBUTE(attribute, 2, unsigned char, temp);

                                // Convert data to raylib texcoord data type (float)
                                for (unsigned int t = 0; t < attribute->count*2; t++) texcoordPtr[t] = (float)temp[t]/255.0f;

                                RL_FREE(temp);
                            } else if (attribute->component_type == cgltf_component_type_r_16u) { // vec2, u16n
                                // Init raylib mesh texcoords to copy glTF attribute data
                                texcoordPtr = (float *)RL_MALLOC(attribute->count*2*sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned short *temp = (unsigned short *)RL_MALLOC(attribute->count*2*sizeof(unsigned short));
                                LOAD_ATTRIBUTE(attribute, 2, unsigned short, temp);

                                // Convert data to raylib texcoord data type (float)
                                for (unsigned int t = 0; t < attribute->count*2; t++) texcoordPtr[t] = (float)temp[t]/65535.0f;

                                RL_FREE(temp);
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Texcoords attribute data format not supported", fileName);
                        }
                        else TraceLog(LOG_WARNING, "MODEL: [%s] Texcoords attribute data format not supported, use vec2 float", fileName);

                        int index = mesh->primitives[p].attributes[j].index;
                        if (index == 0) model.meshes[meshIndex].texcoords = texcoordPtr;
                        else if (index == 1) model.meshes[meshIndex].texcoords2 = texcoordPtr;
                        else {
                            TraceLog(LOG_WARNING, "MODEL: [%s] No more than 2 texture coordinates attributes supported", fileName);
                            if (texcoordPtr != nullptr) RL_FREE(texcoordPtr);
                        }
                    } else if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_color) { // COLOR_n, vec3/vec4, float/u8n/u16n
                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        // WARNING: SPECS: All components of each COLOR_n accessor element MUST be clamped to [0.0, 1.0] range

                        if (model.meshes[meshIndex].colors != nullptr) TraceLog(LOG_WARNING, "MODEL: [%s] Colors attribute data already loaded", fileName);
                        else {
                            if (attribute->type == cgltf_type_vec3) { // RGB
                                if (attribute->component_type == cgltf_component_type_r_8u) {
                                    // Init raylib mesh color to copy glTF attribute data
                                    model.meshes[meshIndex].colors = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));

                                    // Load data into a temp buffer to be converted to raylib data type
                                    unsigned char *temp = (unsigned char *)RL_MALLOC(attribute->count*3*sizeof(unsigned char));
                                    LOAD_ATTRIBUTE(attribute, 3, unsigned char, temp);

                                    // Convert data to raylib color data type (4 bytes)
                                    for (unsigned int c = 0, k = 0; c < (attribute->count*4 - 3); c += 4, k += 3) {
                                        model.meshes[meshIndex].colors[c] = temp[k];
                                        model.meshes[meshIndex].colors[c + 1] = temp[k + 1];
                                        model.meshes[meshIndex].colors[c + 2] = temp[k + 2];
                                        model.meshes[meshIndex].colors[c + 3] = 255;
                                    }

                                    RL_FREE(temp);
                                } else if (attribute->component_type == cgltf_component_type_r_16u) {
                                    // Init raylib mesh color to copy glTF attribute data
                                    model.meshes[meshIndex].colors = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));

                                    // Load data into a temp buffer to be converted to raylib data type
                                    unsigned short *temp = (unsigned short *)RL_MALLOC(attribute->count*3*sizeof(unsigned short));
                                    LOAD_ATTRIBUTE(attribute, 3, unsigned short, temp);

                                    // Convert data to raylib color data type (4 bytes)
                                    for (unsigned int c = 0, k = 0; c < (attribute->count*4 - 3); c += 4, k += 3) {
                                        model.meshes[meshIndex].colors[c] = (unsigned char)(((float)temp[k]/65535.0f)*255.0f);
                                        model.meshes[meshIndex].colors[c + 1] = (unsigned char)(((float)temp[k + 1]/65535.0f)*255.0f);
                                        model.meshes[meshIndex].colors[c + 2] = (unsigned char)(((float)temp[k + 2]/65535.0f)*255.0f);
                                        model.meshes[meshIndex].colors[c + 3] = 255;
                                    }

                                    RL_FREE(temp);
                                } else if (attribute->component_type == cgltf_component_type_r_32f) {
                                    // Init raylib mesh color to copy glTF attribute data
                                    model.meshes[meshIndex].colors = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));

                                    // Load data into a temp buffer to be converted to raylib data type
                                    float *temp = (float *)RL_MALLOC(attribute->count*3*sizeof(float));
                                    LOAD_ATTRIBUTE(attribute, 3, float, temp);

                                    // Convert data to raylib color data type (4 bytes)
                                    for (unsigned int c = 0, k = 0; c < (attribute->count*4 - 3); c += 4, k += 3) {
                                        model.meshes[meshIndex].colors[c] = (unsigned char)(temp[k]*255.0f);
                                        model.meshes[meshIndex].colors[c + 1] = (unsigned char)(temp[k + 1]*255.0f);
                                        model.meshes[meshIndex].colors[c + 2] = (unsigned char)(temp[k + 2]*255.0f);
                                        model.meshes[meshIndex].colors[c + 3] = 255;
                                    }

                                    RL_FREE(temp);
                                }
                                else TraceLog(LOG_WARNING, "MODEL: [%s] Color attribute data format not supported", fileName);
                            } else if (attribute->type == cgltf_type_vec4) { // RGBA
                                if (attribute->component_type == cgltf_component_type_r_8u) {
                                    // Init raylib mesh color to copy glTF attribute data
                                    model.meshes[meshIndex].colors = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));

                                    // Load 4 components of unsigned char data type into mesh.colors
                                    LOAD_ATTRIBUTE(attribute, 4, unsigned char, model.meshes[meshIndex].colors)
                                } else if (attribute->component_type == cgltf_component_type_r_16u) {
                                    // Init raylib mesh color to copy glTF attribute data
                                    model.meshes[meshIndex].colors = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));

                                    // Load data into a temp buffer to be converted to raylib data type
                                    unsigned short *temp = (unsigned short *)RL_MALLOC(attribute->count*4*sizeof(unsigned short));
                                    LOAD_ATTRIBUTE(attribute, 4, unsigned short, temp);

                                    // Convert data to raylib color data type (4 bytes)
                                    for (unsigned int c = 0; c < attribute->count*4; c++) model.meshes[meshIndex].colors[c] = (unsigned char)(((float)temp[c]/65535.0f)*255.0f);

                                    RL_FREE(temp);
                                } else if (attribute->component_type == cgltf_component_type_r_32f) {
                                    // Init raylib mesh color to copy glTF attribute data
                                    model.meshes[meshIndex].colors = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));

                                    // Load data into a temp buffer to be converted to raylib data type
                                    float *temp = (float *)RL_MALLOC(attribute->count*4*sizeof(float));
                                    LOAD_ATTRIBUTE(attribute, 4, float, temp);

                                    // Convert data to raylib color data type (4 bytes), color data must be normalized
                                    for (unsigned int c = 0; c < attribute->count*4; c++) model.meshes[meshIndex].colors[c] = (unsigned char)(temp[c]*255.0f);

                                    RL_FREE(temp);
                                }
                                else TraceLog(LOG_WARNING, "MODEL: [%s] Color attribute data format not supported", fileName);
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Color attribute data format not supported", fileName);
                        }
                    }

                    // NOTE: Attributes related to animations data are processed after mesh data loading
                }

                // Load primitive indices data (if provided)
                if ((mesh->primitives[p].indices != nullptr) && (mesh->primitives[p].indices->buffer_view != nullptr)) {
                    cgltf_accessor *attribute = mesh->primitives[p].indices;

                    model.meshes[meshIndex].triangleCount = (int)attribute->count/3;

                    if (model.meshes[meshIndex].indices != nullptr) TraceLog(LOG_WARNING, "MODEL: [%s] Indices attribute data already loaded", fileName);
                    else {
                        if (attribute->component_type == cgltf_component_type_r_16u) {
                            // Init raylib mesh indices to copy glTF attribute data
                            model.meshes[meshIndex].indices = (unsigned short *)RL_MALLOC(attribute->count*sizeof(unsigned short));

                            // Load unsigned short data type into mesh.indices
                            LOAD_ATTRIBUTE(attribute, 1, unsigned short, model.meshes[meshIndex].indices)
                        } else if (attribute->component_type == cgltf_component_type_r_8u) {
                            // Init raylib mesh indices to copy glTF attribute data
                            model.meshes[meshIndex].indices = (unsigned short *)RL_MALLOC(attribute->count*sizeof(unsigned short));
                            LOAD_ATTRIBUTE_CAST(attribute, 1, unsigned char, model.meshes[meshIndex].indices, unsigned short)
                        } else if (attribute->component_type == cgltf_component_type_r_32u) {
                            // Init raylib mesh indices to copy glTF attribute data
                            model.meshes[meshIndex].indices = (unsigned short *)RL_MALLOC(attribute->count*sizeof(unsigned short));
                            LOAD_ATTRIBUTE_CAST(attribute, 1, unsigned int, model.meshes[meshIndex].indices, unsigned short);

                            TraceLog(LOG_WARNING, "MODEL: [%s] Indices data converted from u32 to u16, possible loss of data", fileName);
                        }
                        else TraceLog(LOG_WARNING, "MODEL: [%s] Indices data format not supported, use u16", fileName);
                    }
                }
                else model.meshes[meshIndex].triangleCount = model.meshes[meshIndex].vertexCount/3;    // Unindexed mesh

                // Assign to the primitive mesh the corresponding material index
                // NOTE: If no material defined, mesh uses the already assigned default material (index: 0)
                for (unsigned int m = 0; m < data->materials_count; m++) {
                    // The primitive actually keeps the pointer to the corresponding material,
                    // raylib instead assigns to the mesh the by its index, as loaded in model.materials array
                    // To get the index, check if material pointers match, and assign the corresponding index,
                    // skipping index 0, the default material
                    if (&data->materials[m] == mesh->primitives[p].material) {
                        model.meshMaterial[meshIndex] = m + 1;
                        break;
                    }
                }

                meshIndex++;       // Move to next mesh
            }
        }
        //----------------------------------------------------------------------------------------------------

        // Load animation data
        // REF: https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#skins
        // REF: https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#skinned-mesh-attributes
        //
        // LIMITATIONS:
        //  - Only supports 1 armature per file, and skips loading it if there are multiple armatures
        //  - Only supports linear interpolation (default method in Blender when checked "Always Sample Animations" when exporting a GLTF file)
        //  - Only supports translation/rotation/scale animation channel.path, weights not considered (a.e. morph targets)
        //----------------------------------------------------------------------------------------------------
        if (data->skins_count > 0)
        {
            cgltf_skin skin = data->skins[0];
            model.skeleton.bones = LoadBoneInfoGLTF(skin, &model.skeleton.boneCount);
            model.skeleton.bindPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));

            for (unsigned int i = 0; i < model.skeleton.boneCount; i++)
            {
                Matrix bindMatrix = { 0 };
                cgltf_float inverseBindTransform[16] = { 0 };

                if ((skin.inverse_bind_matrices != nullptr) &&
                    (skin.inverse_bind_matrices->count >= skin.joints_count) &&
                    cgltf_accessor_read_float(skin.inverse_bind_matrices, i, inverseBindTransform, 16))
                {
                    Matrix inverseBindMatrix = {
                        inverseBindTransform[0], inverseBindTransform[4], inverseBindTransform[8], inverseBindTransform[12],
                        inverseBindTransform[1], inverseBindTransform[5], inverseBindTransform[9], inverseBindTransform[13],
                        inverseBindTransform[2], inverseBindTransform[6], inverseBindTransform[10], inverseBindTransform[14],
                        inverseBindTransform[3], inverseBindTransform[7], inverseBindTransform[11], inverseBindTransform[15]
                    };
                    bindMatrix = MatrixInvert(inverseBindMatrix);
                }
                else
                {
                    cgltf_float worldTransform[16] = { 0 };
                    cgltf_node_transform_world(skin.joints[i], worldTransform);

                    Matrix worldMatrix = {
                        worldTransform[0], worldTransform[4], worldTransform[8], worldTransform[12],
                        worldTransform[1], worldTransform[5], worldTransform[9], worldTransform[13],
                        worldTransform[2], worldTransform[6], worldTransform[10], worldTransform[14],
                        worldTransform[3], worldTransform[7], worldTransform[11], worldTransform[15]
                    };
                    bindMatrix = worldMatrix;
                }

                MatrixDecompose(bindMatrix,
                    &(model.skeleton.bindPose[i].translation),
                    &(model.skeleton.bindPose[i].rotation),
                    &(model.skeleton.bindPose[i].scale));
            }

            if (data->skins_count > 1) TraceLog(LOG_WARNING, "MODEL: [%s] can only load one skin (armature) per model, but gltf skins_count == %i", fileName, data->skins_count);
        }

        meshIndex = 0;
        for (unsigned int i = 0; i < data->nodes_count; i++)
        {
            cgltf_node *node = &(data->nodes[i]);

            cgltf_mesh *mesh = node->mesh;
            if (!mesh) continue;

            for (unsigned int p = 0; p < mesh->primitives_count; p++)
            {
                bool hasJoints = false;

                // NOTE: Only support primitives defined by triangles
                if (mesh->primitives[p].type != cgltf_primitive_type_triangles) continue;

                for (unsigned int j = 0; j < mesh->primitives[p].attributes_count; j++) {
                    // NOTE: JOINTS_1 + WEIGHT_1 will be used for +4 joints influencing a vertex -> Not supported by raylib
                    if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_joints) { // JOINTS_n (vec4: 4 bones max per vertex / u8, u16)
                        hasJoints = true;
                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        // NOTE: JOINTS_n can only be vec4 and u8/u16
                        // SPECS: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview

                        // WARNING: raylib only supports model.meshes[].boneIndices as u8 (unsigned char),
                        // if data is provided in any other format, it is converted to supported format but
                        // it could imply data loss (a warning message is issued in that case)

                        if (attribute->type == cgltf_type_vec4) {
                            if (attribute->component_type == cgltf_component_type_r_8u) {
                                // Init raylib mesh boneIndices to copy glTF attribute data
                                model.meshes[meshIndex].boneIndices = (unsigned char *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(unsigned char));

                                // Load attribute: vec4, u8 (unsigned char)
                                LOAD_ATTRIBUTE(attribute, 4, unsigned char, model.meshes[meshIndex].boneIndices)
                            } else if (attribute->component_type == cgltf_component_type_r_16u) {
                                // Init raylib mesh boneIndices to copy glTF attribute data
                                model.meshes[meshIndex].boneIndices = (unsigned char *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(unsigned char));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned short *temp = (unsigned short *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(unsigned short));
                                LOAD_ATTRIBUTE(attribute, 4, unsigned short, temp);

                                // Convert data to raylib color data type (4 bytes)
                                bool boneIdOverflowWarning = false;
                                for (int b = 0; b < model.meshes[meshIndex].vertexCount*4; b++) {
                                    if ((temp[b] > 255) && !boneIdOverflowWarning) {
                                        TraceLog(LOG_WARNING, "MODEL: [%s] Joint attribute data format (u16) overflow", fileName);
                                        boneIdOverflowWarning = true;
                                    }

                                    // Despite the possible overflow, convert data to unsigned char
                                    model.meshes[meshIndex].boneIndices[b] = (unsigned char)temp[b];
                                }

                                RL_FREE(temp);
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Joint attribute data format not supported", fileName);
                        }
                        else TraceLog(LOG_WARNING, "MODEL: [%s] Joint attribute data format not supported", fileName);
                    } else if (mesh->primitives[p].attributes[j].type == cgltf_attribute_type_weights) { // WEIGHTS_n (vec4, u8n/u16n/f32)
                        cgltf_accessor *attribute = mesh->primitives[p].attributes[j].data;

                        if (attribute->type == cgltf_type_vec4) {
                            if (attribute->component_type == cgltf_component_type_r_8u) {
                                // Init raylib mesh bone weight to copy glTF attribute data
                                model.meshes[meshIndex].boneWeights = (float *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned char *temp = (unsigned char *)RL_MALLOC(attribute->count*4*sizeof(unsigned char));
                                LOAD_ATTRIBUTE(attribute, 4, unsigned char, temp);

                                // Convert data to raylib bone weight data type (4 bytes)
                                for (unsigned int b = 0; b < attribute->count*4; b++) model.meshes[meshIndex].boneWeights[b] = (float)temp[b]/255.0f;

                                RL_FREE(temp);
                            } else if (attribute->component_type == cgltf_component_type_r_16u) {
                                // Init raylib mesh bone weight to copy glTF attribute data
                                model.meshes[meshIndex].boneWeights = (float *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(float));

                                // Load data into a temp buffer to be converted to raylib data type
                                unsigned short *temp = (unsigned short *)RL_MALLOC(attribute->count*4*sizeof(unsigned short));
                                LOAD_ATTRIBUTE(attribute, 4, unsigned short, temp);

                                // Convert data to raylib bone weight data type
                                for (unsigned int b = 0; b < attribute->count*4; b++) model.meshes[meshIndex].boneWeights[b] = (float)temp[b]/65535.0f;

                                RL_FREE(temp);
                            } else if (attribute->component_type == cgltf_component_type_r_32f) {
                                // Init raylib mesh bone weight to copy glTF attribute data
                                model.meshes[meshIndex].boneWeights = (float *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(float));

                                // Load 4 components of float data type into mesh.boneWeights
                                // for cgltf_attribute_type_weights:
                                //   - data.meshes[0] (256 vertices)
                                //   - 256 values, provided as cgltf_type_vec4 of float (4 byte per joint, stride 16)
                                LOAD_ATTRIBUTE(attribute, 4, float, model.meshes[meshIndex].boneWeights)
                            }
                            else TraceLog(LOG_WARNING, "MODEL: [%s] Joint weight attribute data format not supported, use vec4 float", fileName);
                        }
                        else TraceLog(LOG_WARNING, "MODEL: [%s] Joint weight attribute data format not supported, use vec4 float", fileName);
                    }
                }

                // Check if animated, and the mesh was not given any bone assignments, but is the child of a bone node
                // in this case, all the verts need to be attached to the parent bone so it will animate with the bone
                if ((data->skins_count > 0) && !hasJoints && (node->parent != nullptr) && (node->parent->mesh == nullptr)) {
                    int parentBoneId = -1;
                    for (unsigned int joint = 0; joint < model.skeleton.boneCount; joint++) {
                        if (data->skins[0].joints[joint] == node->parent) {
                            parentBoneId = joint;
                            break;
                        }
                    }

                    if (parentBoneId >= 0) {
                        model.meshes[meshIndex].boneIndices = (unsigned char *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(unsigned char));
                        model.meshes[meshIndex].boneWeights = (float *)RL_CALLOC(model.meshes[meshIndex].vertexCount*4, sizeof(float));

                        for (int vertexIndex = 0; vertexIndex < model.meshes[meshIndex].vertexCount*4; vertexIndex += 4) {
                            model.meshes[meshIndex].boneIndices[vertexIndex] = (unsigned char)parentBoneId;
                            model.meshes[meshIndex].boneWeights[vertexIndex] = 1.0f;
                        }
                    }
                }

#if !SUPPORT_GPU_SKINNING
                // Animated vertex data (CPU skinning)
                model.meshes[meshIndex].animVertices = (float *)RL_CALLOC(model.meshes[meshIndex].vertexCount*3, sizeof(float));
                memcpy(model.meshes[meshIndex].animVertices, model.meshes[meshIndex].vertices, model.meshes[meshIndex].vertexCount*3*sizeof(float));
                model.meshes[meshIndex].animNormals = (float *)RL_CALLOC(model.meshes[meshIndex].vertexCount*3, sizeof(float));
                if (model.meshes[meshIndex].normals != nullptr) memcpy(model.meshes[meshIndex].animNormals, model.meshes[meshIndex].normals, model.meshes[meshIndex].vertexCount*3*sizeof(float));
#endif
                model.meshes[meshIndex].boneCount = model.skeleton.boneCount;

                meshIndex++; // Move to next mesh
            }
        }

        // Initialize runtime animation data: current pose and bone matrices
        model.currentPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
        model.boneMatrices = (Matrix *)RL_CALLOC(model.skeleton.boneCount, sizeof(Matrix));
        for (unsigned int j = 0; j < model.skeleton.boneCount; j++) model.boneMatrices[j] = MatrixIdentity();
        //----------------------------------------------------------------------------------------------------

        // Free unused allocated memory in case of no bones defined
        if (model.skeleton.boneCount == 0) {
            RL_FREE(model.currentPose);
            RL_FREE(model.boneMatrices);
            model.currentPose = nullptr;
            model.boneMatrices = nullptr;
        }

        // Free all cgltf loaded data
        cgltf_free(data);
    }
    else TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load glTF data", fileName);

    // WARNING: cgltf requires the file pointer available while reading data
    UnloadFileData(fileData);

    return model;
}

// Get interpolated pose for bone sampler at a specific time. Returns true on success
static bool GetPoseAtTimeGLTF(cgltf_interpolation_type interpolationType, cgltf_accessor *input, cgltf_accessor *output, float time, void *data) {
    if (interpolationType >= cgltf_interpolation_type_max_enum) return false;

    // Input and output should have the same count
    float tstart = 0.0f;
    float tend = 0.0f;
    int keyframe = 0;       // Defaults to first pose
    bool found = false;

    for (int i = 0; i < (int)input->count - 1; i++) {
        cgltf_bool r1 = cgltf_accessor_read_float(input, i, &tstart, 1);
        if (!r1) return false;

        cgltf_bool r2 = cgltf_accessor_read_float(input, i + 1, &tend, 1);
        if (!r2) return false;

        if ((tstart <= time) && (time < tend)) {
            keyframe = i;
            found = true;
            break;
        }
    }

    // No interval contains a time at (or past) the last keyframe, because the
    // search above requires time < tend: clamp to the edge interval instead of
    // falling back to keyframe 0, which returns a pose from the start
    if (!found && ((int)input->count >= 2)) {
        keyframe = (int)input->count - 2;

        float tfirst = 0.0f;
        if (!cgltf_accessor_read_float(input, 0, &tfirst, 1)) return false;
        if (time < tfirst) keyframe = 0;

        if (!cgltf_accessor_read_float(input, keyframe, &tstart, 1)) return false;
        if (!cgltf_accessor_read_float(input, keyframe + 1, &tend, 1)) return false;
    }

    // Constant animation, no need to interpolate
    if (FloatEquals(tend, tstart)) {
        interpolationType = cgltf_interpolation_type_step;
    }

    float duration = fmaxf((tend - tstart), EPSILON);
    float t = (time - tstart)/duration;
    t = (t < 0.0f)? 0.0f : t;
    t = (t > 1.0f)? 1.0f : t;

    if (output->component_type != cgltf_component_type_r_32f) return false;

    if (output->type == cgltf_type_vec3) {
        switch (interpolationType) {
            case cgltf_interpolation_type_step: {
                float tmp[3] = { 0.0f };
                cgltf_accessor_read_float(output, keyframe, tmp, 3);
                Vector3 v1 = {tmp[0], tmp[1], tmp[2]};
                Vector3 *r = (Vector3 *)data;

                *r = v1;
            } break;
            case cgltf_interpolation_type_linear: {
                float tmp[3] = { 0.0f };
                cgltf_accessor_read_float(output, keyframe, tmp, 3);
                Vector3 v1 = {tmp[0], tmp[1], tmp[2]};
                cgltf_accessor_read_float(output, keyframe+1, tmp, 3);
                Vector3 v2 = {tmp[0], tmp[1], tmp[2]};
                Vector3 *r = (Vector3 *)data;

                *r = Vector3Lerp(v1, v2, t);
            } break;
            case cgltf_interpolation_type_cubic_spline: {
                float tmp[3] = { 0.0f };
                cgltf_accessor_read_float(output, 3*keyframe+1, tmp, 3);
                Vector3 v1 = {tmp[0], tmp[1], tmp[2]};
                cgltf_accessor_read_float(output, 3*keyframe+2, tmp, 3);
                Vector3 tangent1 = {tmp[0], tmp[1], tmp[2]};
                cgltf_accessor_read_float(output, 3*(keyframe+1)+1, tmp, 3);
                Vector3 v2 = {tmp[0], tmp[1], tmp[2]};
                cgltf_accessor_read_float(output, 3*(keyframe+1), tmp, 3);
                Vector3 tangent2 = {tmp[0], tmp[1], tmp[2]};
                Vector3 *r = (Vector3 *)data;

                *r = Vector3CubicHermite(v1, tangent1, v2, tangent2, t);
            } break;
            default: break;
        }
    } else if (output->type == cgltf_type_vec4) {
        // Only v4 is for rotations, so it's a quaternion
        switch (interpolationType) {
            case cgltf_interpolation_type_step: {
                float tmp[4] = { 0.0f };
                cgltf_accessor_read_float(output, keyframe, tmp, 4);
                Vector4 v1 = {tmp[0], tmp[1], tmp[2], tmp[3]};
                Vector4 *r = (Vector4 *)data;

                *r = v1;
            } break;
            case cgltf_interpolation_type_linear: {
                float tmp[4] = { 0.0f };
                cgltf_accessor_read_float(output, keyframe, tmp, 4);
                Vector4 v1 = {tmp[0], tmp[1], tmp[2], tmp[3]};
                cgltf_accessor_read_float(output, keyframe+1, tmp, 4);
                Vector4 v2 = {tmp[0], tmp[1], tmp[2], tmp[3]};
                Vector4 *r = (Vector4 *)data;

                *r = QuaternionSlerp(v1, v2, t);
            } break;
            case cgltf_interpolation_type_cubic_spline: {
                float tmp[4] = { 0.0f };
                cgltf_accessor_read_float(output, 3*keyframe+1, tmp, 4);
                Vector4 v1 = {tmp[0], tmp[1], tmp[2], tmp[3]};
                cgltf_accessor_read_float(output, 3*keyframe+2, tmp, 4);
                Vector4 outTangent1 = {tmp[0], tmp[1], tmp[2], 0.0f};
                cgltf_accessor_read_float(output, 3*(keyframe+1)+1, tmp, 4);
                Vector4 v2 = {tmp[0], tmp[1], tmp[2], tmp[3]};
                cgltf_accessor_read_float(output, 3*(keyframe+1), tmp, 4);
                Vector4 inTangent2 = {tmp[0], tmp[1], tmp[2], 0.0f};
                Vector4 *r = (Vector4 *)data;

                v1 = QuaternionNormalize(v1);
                v2 = QuaternionNormalize(v2);

                if (Vector4DotProduct(v1, v2) < 0.0f)
                {
                    v2 = Vector4Negate(v2);
                }

                outTangent1 = Vector4Scale(outTangent1, duration);
                inTangent2 = Vector4Scale(inTangent2, duration);

                *r = QuaternionCubicHermiteSpline(v1, outTangent1, v2, inTangent2, t);
            } break;
            default: break;
        }
    }

    return true;
}

#define GLTF_FRAMERATE 60.0f    // glTF animation framerate (frames per second)

static ModelAnimation *LoadModelAnimationsGLTF(const char *fileName, int *animCount) {
    // glTF file loading
    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);

    ModelAnimation *animations = nullptr;

    // glTF data loading
    cgltf_options options = { 0 };
    options.file.read = LoadFileGLTFCallback;
    options.file.release = ReleaseFileGLTFCallback;
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

    if (result != cgltf_result_success) {
        TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load glTF data", fileName);
        *animCount = 0;
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, fileName);
    if (result != cgltf_result_success) TraceLog(LOG_INFO, "MODEL: [%s] Failed to load animation buffers", fileName);

    if (result == cgltf_result_success) {
        if (data->skins_count > 0) {
            cgltf_skin skin = data->skins[0];

            // Precompute, per joint, the static transform contributed by any
            // intermediate non-joint nodes between the joint and its nearest
            // joint ancestor. This handles exporters (e.g. wow.export) that
            // store bone offsets on dummy parent nodes rather than on the
            // joints themselves. Depends only on the skin, not the animation.
            int jointCount = (int)skin.joints_count;
            Matrix *extOffset = (Matrix *)RL_MALLOC(jointCount*sizeof(Matrix));

            if (extOffset == nullptr) {
                // Allocation failed: abort animation loading at the source rather than
                // propagating a nullptr pointer into the per-frame transform loop below
                TraceLog(LOG_WARNING, "MODEL: [%s] Failed to allocate joint offset buffer", fileName);
                cgltf_free(data);
                UnloadFileData(fileData);
                *animCount = 0;
                return nullptr;
            }

            *animCount = (int)data->animations_count;
            animations = (ModelAnimation *)RL_CALLOC(data->animations_count, sizeof(ModelAnimation));

            for (int k = 0; k < jointCount; k++) {
                extOffset[k] = MatrixIdentity();
                cgltf_node *n = skin.joints[k]->parent;

                while (n != nullptr) {
                    bool isJoint = false;
                    for (int jj = 0; jj < jointCount; jj++) {
                        if (skin.joints[jj] == n) {
                            isJoint = true;
                            break;
                        }
                    }

                    if (isJoint) break;

                    // Compose the intermediate node's local TRS (scale, then rotation, then translation)
                    Matrix nodeScale = MatrixScale(n->scale[0], n->scale[1], n->scale[2]);
                    Matrix nodeRotation = QuaternionToMatrix((Quaternion){ n->rotation[0], n->rotation[1], n->rotation[2], n->rotation[3] });
                    Matrix nodeTranslation = MatrixTranslate(n->translation[0], n->translation[1], n->translation[2]);
                    Matrix nodeTransform = MatrixMultiply(MatrixMultiply(nodeScale, nodeRotation), nodeTranslation);

                    extOffset[k] = MatrixMultiply(extOffset[k], nodeTransform);
                    n = n->parent;
                }
            }

            for (unsigned int a = 0; a < data->animations_count; a++) {
                BoneInfo *bones = LoadBoneInfoGLTF(skin, &animations[a].boneCount);

                cgltf_animation animData = data->animations[a];

                struct Channels {
                    cgltf_animation_channel *translate;
                    cgltf_animation_channel *rotate;
                    cgltf_animation_channel *scale;
                };

                struct Channels *boneChannels = (struct Channels *)RL_CALLOC(animations[a].boneCount, sizeof(struct Channels));
                float animDuration = 0.0f;

                for (unsigned int j = 0; j < animData.channels_count; j++) {
                    cgltf_animation_channel channel = animData.channels[j];
                    int boneIndex = -1;

                    for (unsigned int k = 0; k < skin.joints_count; k++) {
                        if (animData.channels[j].target_node == skin.joints[k]) {
                            boneIndex = k;
                            break;
                        }
                    }

                    if (boneIndex == -1) continue; // Animation channel for a node not in the skeleton

                    if (animData.channels[j].sampler->interpolation != cgltf_interpolation_type_max_enum) {
                        if (channel.target_path == cgltf_animation_path_type_translation) {
                            boneChannels[boneIndex].translate = &animData.channels[j];
                        } else if (channel.target_path == cgltf_animation_path_type_rotation) {
                            boneChannels[boneIndex].rotate = &animData.channels[j];
                        } else if (channel.target_path == cgltf_animation_path_type_scale) {
                            boneChannels[boneIndex].scale = &animData.channels[j];
                        } else {
                            TraceLog(LOG_WARNING, "MODEL: [%s] Unsupported target_path on channel %d's sampler for animation %d. Skipping.", fileName, j, a);
                        }
                    }
                    else TraceLog(LOG_WARNING, "MODEL: [%s] Invalid interpolation curve encountered for GLTF animation.", fileName);

                    float time = 0.0f;
                    cgltf_bool result = cgltf_accessor_read_float(channel.sampler->input, channel.sampler->input->count - 1, &time, 1);

                    if (!result) {
                        TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load input time", fileName);
                        continue;
                    }

                    animDuration = (time > animDuration)? time : animDuration;
                }

                if (animData.name != nullptr) snprintf(animations[a].name, sizeof(animations[a].name), "%s", animData.name);

                animations[a].keyframeCount = (int)(animDuration*GLTF_FRAMERATE) + 1;
                animations[a].keyframePoses = (Transform **)RL_CALLOC(animations[a].keyframeCount, sizeof(Transform *));

                for (int j = 0; j < animations[a].keyframeCount; j++) {
                    animations[a].keyframePoses[j] = (Transform *)RL_CALLOC(animations[a].boneCount, sizeof(Transform));
                    float time = (float)j / GLTF_FRAMERATE;

                    for (unsigned int k = 0; k < animations[a].boneCount; k++) {
                        Vector3 translation = {skin.joints[k]->translation[0], skin.joints[k]->translation[1], skin.joints[k]->translation[2]};
                        Quaternion rotation = {skin.joints[k]->rotation[0], skin.joints[k]->rotation[1], skin.joints[k]->rotation[2], skin.joints[k]->rotation[3]};
                        Vector3 scale = {skin.joints[k]->scale[0], skin.joints[k]->scale[1], skin.joints[k]->scale[2]};

                        if (boneChannels[k].translate) {
                            if (!GetPoseAtTimeGLTF(boneChannels[k].translate->sampler->interpolation, boneChannels[k].translate->sampler->input, boneChannels[k].translate->sampler->output, time, &translation)) {
                                TraceLog(LOG_INFO, "MODEL: [%s] Failed to load translate pose data for bone %s", fileName, bones[k].name);
                            }
                        }

                        if (boneChannels[k].rotate) {
                            if (!GetPoseAtTimeGLTF(boneChannels[k].rotate->sampler->interpolation, boneChannels[k].rotate->sampler->input, boneChannels[k].rotate->sampler->output, time, &rotation)) {
                                TraceLog(LOG_INFO, "MODEL: [%s] Failed to load rotate pose data for bone %s", fileName, bones[k].name);
                            }
                        }

                        if (boneChannels[k].scale) {
                            if (!GetPoseAtTimeGLTF(boneChannels[k].scale->sampler->interpolation, boneChannels[k].scale->sampler->input, boneChannels[k].scale->sampler->output, time, &scale)) {
                                TraceLog(LOG_INFO, "MODEL: [%s] Failed to load scale pose data for bone %s", fileName, bones[k].name);
                            }
                        }

                        // Compose joint local TRS, then prepend the static
                        // intermediate non-joint offsets so the final TRS is
                        // expressed relative to the joint's skeleton parent.
                        Matrix S = MatrixScale(scale.x, scale.y, scale.z);
                        Matrix R = QuaternionToMatrix(rotation);
                        Matrix T = MatrixTranslate(translation.x, translation.y, translation.z);
                        Matrix jointLocal = MatrixMultiply(MatrixMultiply(S, R), T);
                        Matrix combined = MatrixMultiply(jointLocal, extOffset[k]);

                        Transform tr;
                        MatrixDecompose(combined, &tr.translation, &tr.rotation, &tr.scale);
                        animations[a].keyframePoses[j][k] = tr;
                    }

                    BuildPoseFromParentJoints(bones, animations[a].boneCount, animations[a].keyframePoses[j]);
                }

                TraceLog(LOG_INFO, "MODEL: [%s] Loaded animation: %s | Frames: %d | Duration: %fs", fileName, (animData.name != nullptr)? animData.name : "nullptr", animations[a].keyframeCount, animDuration);
                RL_FREE(boneChannels);
                RL_FREE(bones);
            }

            RL_FREE(extOffset);
        }

        if (data->skins_count > 1) {
            TraceLog(LOG_WARNING, "MODEL: [%s] Expected one unique skin to load animation data from, but found %i", fileName, data->skins_count);
        }

        cgltf_free(data);
    }

    UnloadFileData(fileData);

    return animations;
}
#endif

#if SUPPORT_FILEFORMAT_VOX
// Load VOX (MagicaVoxel) mesh data
static Model LoadVOX(const char *fileName) {
    Model model = {0};

    int nbvertices = 0;
    int meshescount = 0;

    // Read vox file into buffer
    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);

    if (fileData == 0) {
        TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load VOX file", fileName);
        return model;
    }

    // Read and build voxarray description
    VoxArray3D voxarray = {0};
    int ret = Vox_LoadFromMemory(fileData, dataSize, &voxarray);

    if (ret != VOX_SUCCESS) {
        // Error
        UnloadFileData(fileData);

        TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load VOX data", fileName);
        return model;
    }
    // Success: Compute meshes count
    nbvertices = voxarray.vertices.used;
    meshescount = 1 + (nbvertices/65536);

    TraceLog(LOG_INFO, "MODEL: [%s] VOX data loaded successfully : %i vertices/%i meshes", fileName, nbvertices, meshescount);

    // Build models from meshes
    model.transform = MatrixIdentity();

    model.meshCount = meshescount;
    model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));

    model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));

    model.materialCount = 1;
    model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
    model.materials[0] = LoadMaterialDefault();

    // Init model meshes
    int verticesRemain = voxarray.vertices.used;
    int verticesMax = 65532; // 5461 voxels x 12 vertices per voxel -> 65532 (must be inf 65536)

    // 6*4 = 12 vertices per voxel
    Vector3 *pvertices = (Vector3 *)voxarray.vertices.array;
    Vector3 *pnormals = (Vector3 *)voxarray.normals.array;
    Color *pcolors = (Color *)voxarray.colors.array;

    unsigned short *pindices = voxarray.indices.array;    // 5461*6*6 = 196596 indices max per mesh

    int size = 0;

    for (int i = 0; i < meshescount; i++) {
        Mesh *pmesh = &model.meshes[i];
        memset(pmesh, 0, sizeof(Mesh));

        // Copy vertices
        pmesh->vertexCount = (int)fmin(verticesMax, verticesRemain);

        size = pmesh->vertexCount*sizeof(float)*3;
        pmesh->vertices = (float *)RL_MALLOC(size);
        memcpy(pmesh->vertices, pvertices, size);

        // Copy normals
        pmesh->normals = (float *)RL_MALLOC(size);
        memcpy(pmesh->normals, pnormals, size);

        // Copy indices
        size = voxarray.indices.used*sizeof(unsigned short);
        pmesh->indices = (unsigned short *)RL_MALLOC(size);
        memcpy(pmesh->indices, pindices, size);

        pmesh->triangleCount = (pmesh->vertexCount/4)*2;

        // Copy colors
        size = pmesh->vertexCount*sizeof(Color);
        pmesh->colors = (unsigned char *)RL_MALLOC(size);
        memcpy(pmesh->colors, pcolors, size);

        // First material index
        model.meshMaterial[i] = 0;

        verticesRemain -= verticesMax;
        pvertices += verticesMax;
        pnormals += verticesMax;
        pcolors += verticesMax;
    }

    // Free buffers
    Vox_FreeArrays(&voxarray);
    UnloadFileData(fileData);

    return model;
}
#endif

#if SUPPORT_FILEFORMAT_M3D

// Load M3D mesh data
static Model LoadM3D(const char *fileName) {
    Model model = { 0 };

    m3d_t *m3d = nullptr;
    m3dp_t *prop = nullptr;
    int i, j, k, l, n, mi = -2, vcolor = 0;

    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);

    if (fileData != nullptr) {
        m3d = m3d_load(fileData, m3d_loaderhook, m3d_freehook, nullptr);

        if (!m3d || M3D_ERR_ISFATAL(m3d->errcode)) {
            TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load M3D data, error code %d", fileName, m3d? m3d->errcode : -2);
            if (m3d) m3d_free(m3d);
            UnloadFileData(fileData);
            return model;
        }
        TraceLog(LOG_INFO, "MODEL: [%s] M3D data loaded successfully: %i faces/%i materials", fileName, m3d->numface, m3d->nummaterial);

        // Check if face is found, if not, probably just a material library
        if (!m3d->numface) {
            m3d_free(m3d);
            UnloadFileData(fileData);
            return model;
        }

        if (m3d->nummaterial > 0) {
            model.meshCount = model.materialCount = m3d->nummaterial;
            TraceLog(LOG_INFO, "MODEL: model has %i material meshes", model.materialCount);
        } else {
            model.meshCount = 1; model.materialCount = 0;
            TraceLog(LOG_INFO, "MODEL: No materials, putting all meshes in a default material");
        }

        // A default material is always required, so adding +1
        model.materialCount++;

        // Faces must be in non-decreasing materialid order. Verify that quickly, sorting them otherwise
        // WARNING: Sorting is not needed, valid M3D model files should already be sorted
        // Keeping the sorting function for reference (Check PR #3363 #3385)
        /*
        for (a = 1; a < m3d->numface; a++)
        {
            if (m3d->face[a-1].materialid <= m3d->face[a].materialid) continue;

            // face[a-1] > face[a].  slide face[a] lower
            m3df_t slider = m3d->face[a];
            j = a-1;

            do
            {   // face[j] > slider, face[j+1] is svailable vacant gap
                m3d->face[j+1] = m3d->face[j];
                j = j-1;
            }
            while (j >= 0 && m3d->face[j].materialid > slider.materialid);

            m3d->face[j+1] = slider;
        }
        */

        model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));
        model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));
        model.materials = (Material *)RL_CALLOC(model.materialCount + 1, sizeof(Material));

        // Map no material to index 0 with default shader, everything else materialid + 1
        model.materials[0] = LoadMaterialDefault();

        for (i = l = 0, k = -1; i < (int)m3d->numface; i++, l++) {
            // Materials are grouped together
            if (mi != m3d->face[i].materialid) {
                // There should be only one material switch per material kind,
                // but be bulletproof for non-optimal model files
                if (k + 1 >= model.meshCount) {
                    model.meshCount++;

                    // Create a second buffer for mesh re-allocation
                    Mesh *tempMeshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));
                    memcpy(tempMeshes, model.meshes, (model.meshCount - 1)*sizeof(Mesh));
                    RL_FREE(model.meshes);
                    model.meshes = tempMeshes;

                    // Create a second buffer for material re-allocation
                    int *tempMeshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));
                    memcpy(tempMeshMaterial, model.meshMaterial, (model.meshCount - 1)*sizeof(int));
                    RL_FREE(model.meshMaterial);
                    model.meshMaterial = tempMeshMaterial;
                }

                k++;
                mi = m3d->face[i].materialid;

                // Only allocate colors VertexBuffer if there's a color vertex in the model for this material batch
                // if all colors are fully transparent black for all vertices of this material, then assuming no vertex colors
                for (j = i, l = vcolor = 0; (j < (int)m3d->numface) && (mi == m3d->face[j].materialid); j++, l++) {
                    if (!m3d->vertex[m3d->face[j].vertex[0]].color ||
                        !m3d->vertex[m3d->face[j].vertex[1]].color ||
                        !m3d->vertex[m3d->face[j].vertex[2]].color) vcolor = 1;
                }

                model.meshes[k].vertexCount = l*3;
                model.meshes[k].triangleCount = l;
                model.meshes[k].vertices = (float *)RL_CALLOC(model.meshes[k].vertexCount*3, sizeof(float));
                model.meshes[k].texcoords = (float *)RL_CALLOC(model.meshes[k].vertexCount*2, sizeof(float));
                model.meshes[k].normals = (float *)RL_CALLOC(model.meshes[k].vertexCount*3, sizeof(float));

                // If no map is provided, or colors are defined, allocate storage for vertex colors
                // M3D specs only consider vertex colors if no material is provided, however raylib uses both and mixes the colors
                if ((mi == M3D_UNDEF) || vcolor) model.meshes[k].colors = (unsigned char *)RL_CALLOC(model.meshes[k].vertexCount*4, sizeof(unsigned char));

                // If no map is provided and vertex colors are allocated, set them to white
                if ((mi == M3D_UNDEF) && (model.meshes[k].colors != nullptr)) {
                    for (int c = 0; c < model.meshes[k].vertexCount*4; c++) model.meshes[k].colors[c] = 255;
                }

                if (m3d->numbone && m3d->numskin) {
                    model.meshes[k].boneIndices = (unsigned char *)RL_CALLOC(model.meshes[k].vertexCount*4, sizeof(unsigned char));
                    model.meshes[k].boneWeights = (float *)RL_CALLOC(model.meshes[k].vertexCount*4, sizeof(float));
                    #if !SUPPORT_GPU_SKINNING
                    model.meshes[k].animVertices = (float *)RL_CALLOC(model.meshes[k].vertexCount*3, sizeof(float));
                    model.meshes[k].animNormals = (float *)RL_CALLOC(model.meshes[k].vertexCount*3, sizeof(float));
                    #endif
                }

                model.meshMaterial[k] = mi + 1;
                l = 0;
            }

            // Process meshes per material, add triangles
            model.meshes[k].vertices[l*9 + 0] = m3d->vertex[m3d->face[i].vertex[0]].x*m3d->scale;
            model.meshes[k].vertices[l*9 + 1] = m3d->vertex[m3d->face[i].vertex[0]].y*m3d->scale;
            model.meshes[k].vertices[l*9 + 2] = m3d->vertex[m3d->face[i].vertex[0]].z*m3d->scale;
            model.meshes[k].vertices[l*9 + 3] = m3d->vertex[m3d->face[i].vertex[1]].x*m3d->scale;
            model.meshes[k].vertices[l*9 + 4] = m3d->vertex[m3d->face[i].vertex[1]].y*m3d->scale;
            model.meshes[k].vertices[l*9 + 5] = m3d->vertex[m3d->face[i].vertex[1]].z*m3d->scale;
            model.meshes[k].vertices[l*9 + 6] = m3d->vertex[m3d->face[i].vertex[2]].x*m3d->scale;
            model.meshes[k].vertices[l*9 + 7] = m3d->vertex[m3d->face[i].vertex[2]].y*m3d->scale;
            model.meshes[k].vertices[l*9 + 8] = m3d->vertex[m3d->face[i].vertex[2]].z*m3d->scale;

            // Without vertex color (full transparency), using the default color
            if (model.meshes[k].colors != nullptr) {
                if (m3d->vertex[m3d->face[i].vertex[0]].color & 0xff000000)
                    memcpy(&model.meshes[k].colors[l*12 + 0], &m3d->vertex[m3d->face[i].vertex[0]].color, 4);
                if (m3d->vertex[m3d->face[i].vertex[1]].color & 0xff000000)
                    memcpy(&model.meshes[k].colors[l*12 + 4], &m3d->vertex[m3d->face[i].vertex[1]].color, 4);
                if (m3d->vertex[m3d->face[i].vertex[2]].color & 0xff000000)
                    memcpy(&model.meshes[k].colors[l*12 + 8], &m3d->vertex[m3d->face[i].vertex[2]].color, 4);
            }

            if (m3d->face[i].texcoord[0] != M3D_UNDEF) {
                model.meshes[k].texcoords[l*6 + 0] = m3d->tmap[m3d->face[i].texcoord[0]].u;
                model.meshes[k].texcoords[l*6 + 1] = 1.0f - m3d->tmap[m3d->face[i].texcoord[0]].v;
                model.meshes[k].texcoords[l*6 + 2] = m3d->tmap[m3d->face[i].texcoord[1]].u;
                model.meshes[k].texcoords[l*6 + 3] = 1.0f - m3d->tmap[m3d->face[i].texcoord[1]].v;
                model.meshes[k].texcoords[l*6 + 4] = m3d->tmap[m3d->face[i].texcoord[2]].u;
                model.meshes[k].texcoords[l*6 + 5] = 1.0f - m3d->tmap[m3d->face[i].texcoord[2]].v;
            }

            if (m3d->face[i].normal[0] != M3D_UNDEF) {
                model.meshes[k].normals[l*9 + 0] = m3d->vertex[m3d->face[i].normal[0]].x;
                model.meshes[k].normals[l*9 + 1] = m3d->vertex[m3d->face[i].normal[0]].y;
                model.meshes[k].normals[l*9 + 2] = m3d->vertex[m3d->face[i].normal[0]].z;
                model.meshes[k].normals[l*9 + 3] = m3d->vertex[m3d->face[i].normal[1]].x;
                model.meshes[k].normals[l*9 + 4] = m3d->vertex[m3d->face[i].normal[1]].y;
                model.meshes[k].normals[l*9 + 5] = m3d->vertex[m3d->face[i].normal[1]].z;
                model.meshes[k].normals[l*9 + 6] = m3d->vertex[m3d->face[i].normal[2]].x;
                model.meshes[k].normals[l*9 + 7] = m3d->vertex[m3d->face[i].normal[2]].y;
                model.meshes[k].normals[l*9 + 8] = m3d->vertex[m3d->face[i].normal[2]].z;
            }

            // Add skin (vertex / bone weight pairs)
            if (m3d->numbone && m3d->numskin) {
                for (n = 0; n < 3; n++) {
                    int skinid = m3d->vertex[m3d->face[i].vertex[n]].skinid;

                    // Check if there is a skin for this mesh
                    if ((skinid != M3D_UNDEF) && (skinid < (int)m3d->numskin)) {
                        for (j = 0; j < 4; j++) {
                            model.meshes[k].boneIndices[l*12 + n*4 + j] = m3d->skin[skinid].boneid[j];
                            model.meshes[k].boneWeights[l*12 + n*4 + j] = m3d->skin[skinid].weight[j];
                        }
                    } else {
                        // Boneless meshes with skeletal animations are not supported, so
                        // putting all vertices without a bone into a special "no bone" bone
                        model.meshes[k].boneIndices[l*12 + n*4] = m3d->numbone;
                        model.meshes[k].boneWeights[l*12 + n*4] = 1.0f;
                    }
                }
            }
        }

        // Load materials
        for (i = 0; i < (int)m3d->nummaterial; i++) {
            model.materials[i + 1] = LoadMaterialDefault();

            for (j = 0; j < m3d->material[i].numprop; j++) {
                prop = &m3d->material[i].prop[j];

                switch (prop->type) {
                    case m3dp_Kd: {
                        memcpy(&model.materials[i + 1].maps[MATERIAL_MAP_DIFFUSE].color, &prop->value.color, 4);
                        model.materials[i + 1].maps[MATERIAL_MAP_DIFFUSE].value = 0.0f;
                    } break;
                    case m3dp_Ks: {
                        memcpy(&model.materials[i + 1].maps[MATERIAL_MAP_SPECULAR].color, &prop->value.color, 4);
                    } break;
                    case m3dp_Ns: {
                        model.materials[i + 1].maps[MATERIAL_MAP_SPECULAR].value = prop->value.fnum;
                    } break;
                    case m3dp_Ke: {
                        memcpy(&model.materials[i + 1].maps[MATERIAL_MAP_EMISSION].color, &prop->value.color, 4);
                        model.materials[i + 1].maps[MATERIAL_MAP_EMISSION].value = 0.0f;
                    } break;
                    case m3dp_Pm: {
                        model.materials[i + 1].maps[MATERIAL_MAP_METALNESS].value = prop->value.fnum;
                    } break;
                    case m3dp_Pr: {
                        model.materials[i + 1].maps[MATERIAL_MAP_ROUGHNESS].value = prop->value.fnum;
                    } break;
                    case m3dp_Ps: {
                        model.materials[i + 1].maps[MATERIAL_MAP_NORMAL].color = WHITE;
                        model.materials[i + 1].maps[MATERIAL_MAP_NORMAL].value = prop->value.fnum;
                    } break;
                    default: {
                        if (prop->type >= 128) {
                            Image image = { 0 };
                            image.data = m3d->texture[prop->value.textureid].d;
                            image.width = m3d->texture[prop->value.textureid].w;
                            image.height = m3d->texture[prop->value.textureid].h;
                            image.mipmaps = 1;
                            image.format = (m3d->texture[prop->value.textureid].f == 4)? PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 :
                                           ((m3d->texture[prop->value.textureid].f == 3)? PIXELFORMAT_UNCOMPRESSED_R8G8B8 :
                                           ((m3d->texture[prop->value.textureid].f == 2)? PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA : PIXELFORMAT_UNCOMPRESSED_GRAYSCALE));

                            switch (prop->type) {
                                case m3dp_map_Kd: model.materials[i + 1].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTextureFromImage(image); break;
                                case m3dp_map_Ks: model.materials[i + 1].maps[MATERIAL_MAP_SPECULAR].texture = LoadTextureFromImage(image); break;
                                case m3dp_map_Ke: model.materials[i + 1].maps[MATERIAL_MAP_EMISSION].texture = LoadTextureFromImage(image); break;
                                case m3dp_map_Km: model.materials[i + 1].maps[MATERIAL_MAP_NORMAL].texture = LoadTextureFromImage(image); break;
                                case m3dp_map_Ka: model.materials[i + 1].maps[MATERIAL_MAP_OCCLUSION].texture = LoadTextureFromImage(image); break;
                                case m3dp_map_Pm: model.materials[i + 1].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTextureFromImage(image); break;
                                default: break;
                            }
                        }
                    } break;
                }
            }
        }

        // Load bones
        if (m3d->numbone) {
            model.skeleton.boneCount = m3d->numbone + 1;
            model.skeleton.bones = (BoneInfo *)RL_CALLOC(model.skeleton.boneCount, sizeof(BoneInfo));
            model.skeleton.bindPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));

            for (i = 0; i < (int)m3d->numbone; i++) {
                model.skeleton.bones[i].parent = m3d->bone[i].parent;
                snprintf(model.skeleton.bones[i].name, sizeof(model.skeleton.bones[i].name), "%s", m3d->bone[i].name);
                model.skeleton.bindPose[i].translation.x = m3d->vertex[m3d->bone[i].pos].x*m3d->scale;
                model.skeleton.bindPose[i].translation.y = m3d->vertex[m3d->bone[i].pos].y*m3d->scale;
                model.skeleton.bindPose[i].translation.z = m3d->vertex[m3d->bone[i].pos].z*m3d->scale;
                model.skeleton.bindPose[i].rotation.x = m3d->vertex[m3d->bone[i].ori].x;
                model.skeleton.bindPose[i].rotation.y = m3d->vertex[m3d->bone[i].ori].y;
                model.skeleton.bindPose[i].rotation.z = m3d->vertex[m3d->bone[i].ori].z;
                model.skeleton.bindPose[i].rotation.w = m3d->vertex[m3d->bone[i].ori].w;

                // NOTE: If the orientation quaternion is not normalized, then that's encoding scaling
                model.skeleton.bindPose[i].rotation = QuaternionNormalize(model.skeleton.bindPose[i].rotation);
                model.skeleton.bindPose[i].scale.x = model.skeleton.bindPose[i].scale.y = model.skeleton.bindPose[i].scale.z = 1.0f;

                // Child bones are stored in parent bone relative space, convert that into model space
                if (model.skeleton.bones[i].parent >= 0) {
                    model.skeleton.bindPose[i].rotation = QuaternionMultiply(model.skeleton.bindPose[model.skeleton.bones[i].parent].rotation, model.skeleton.bindPose[i].rotation);
                    model.skeleton.bindPose[i].translation = Vector3RotateByQuaternion(model.skeleton.bindPose[i].translation, model.skeleton.bindPose[model.skeleton.bones[i].parent].rotation);
                    model.skeleton.bindPose[i].translation = Vector3Add(model.skeleton.bindPose[i].translation, model.skeleton.bindPose[model.skeleton.bones[i].parent].translation);
                    model.skeleton.bindPose[i].scale = Vector3Multiply(model.skeleton.bindPose[i].scale, model.skeleton.bindPose[model.skeleton.bones[i].parent].scale);
                }
            }

            // Add a special "no bone" bone
            model.skeleton.bones[i].parent = -1;
            memcpy(model.skeleton.bones[i].name, "NO BONE", 7);
            model.skeleton.bindPose[i].translation.x = 0.0f;
            model.skeleton.bindPose[i].translation.y = 0.0f;
            model.skeleton.bindPose[i].translation.z = 0.0f;
            model.skeleton.bindPose[i].rotation.x = 0.0f;
            model.skeleton.bindPose[i].rotation.y = 0.0f;
            model.skeleton.bindPose[i].rotation.z = 0.0f;
            model.skeleton.bindPose[i].rotation.w = 1.0f;
            model.skeleton.bindPose[i].scale.x = model.skeleton.bindPose[i].scale.y = model.skeleton.bindPose[i].scale.z = 1.0f;
        }

        // Load bone-pose default mesh into animation vertices. These will be updated when UpdateModelAnimation gets
        // called, but not before, however DrawMesh uses these if they exist (so not good if they are left empty)
        if (m3d->numbone && m3d->numskin) {
            for (i = 0; i < model.meshCount; i++) {
                model.meshes[i].boneCount = model.skeleton.boneCount;

                #if !SUPPORT_GPU_SKINNING
                // Initialize vertex buffers for CPU skinning
                memcpy(model.meshes[i].animVertices, model.meshes[i].vertices, model.meshes[i].vertexCount*3*sizeof(float));
                memcpy(model.meshes[i].animNormals, model.meshes[i].normals, model.meshes[i].vertexCount*3*sizeof(float));
                #endif
            }

            // Initialize runtime animation data: current pose and bone matrices
            model.currentPose = (Transform *)RL_CALLOC(model.skeleton.boneCount, sizeof(Transform));
            model.boneMatrices = (Matrix *)RL_CALLOC(model.skeleton.boneCount, sizeof(Matrix));
            for (unsigned int j = 0; j < model.skeleton.boneCount; j++) model.boneMatrices[j] = MatrixIdentity();
        }

        m3d_free(m3d);
        UnloadFileData(fileData);
    }

    return model;
}

#define M3D_ANIMDELAY 17    // Animation frames delay, (~1000 ms/60 FPS = 16.666666 ms)

// Load M3D animation data
static ModelAnimation *LoadModelAnimationsM3D(const char *fileName, int *animCount) {
    ModelAnimation *animations = nullptr;

    m3d_t *m3d = nullptr;
    int i = 0, j = 0;
    *animCount = 0;

    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);

    if (fileData != nullptr) {
        m3d = m3d_load(fileData, m3d_loaderhook, m3d_freehook, nullptr);

        if (!m3d || M3D_ERR_ISFATAL(m3d->errcode)) {
            TraceLog(LOG_WARNING, "MODEL: [%s] Failed to load M3D data, error code %d", fileName, m3d? m3d->errcode : -2);
            UnloadFileData(fileData);
            return nullptr;
        }
        TraceLog(LOG_INFO, "MODEL: [%s] M3D data loaded successfully: %i animations, %i bones, %i skins", fileName, m3d->numaction, m3d->numbone, m3d->numskin);

        // No animation or bones, exit out. skins are not required because some people use one animation for N models
        if (!m3d->numaction || !m3d->numbone) {
            m3d_free(m3d);
            UnloadFileData(fileData);
            return nullptr;
        }

        animations = (ModelAnimation *)RL_CALLOC(m3d->numaction, sizeof(ModelAnimation));
        *animCount = m3d->numaction;

        for (unsigned int a = 0; a < m3d->numaction; a++) {
            animations[a].boneCount = m3d->numbone + 1;
            BoneInfo *bones = (BoneInfo *)RL_CALLOC((m3d->numbone + 1), sizeof(BoneInfo));

            animations[a].keyframeCount = m3d->action[a].durationmsec/M3D_ANIMDELAY;
            animations[a].keyframePoses = (Transform **)RL_CALLOC(animations[a].keyframeCount, sizeof(Transform *));
            snprintf(animations[a].name, sizeof(animations[a].name), "%s", m3d->action[a].name);

            TraceLog(LOG_INFO, "MODEL: [%s] Loaded animation: %s | Frames: %d | Duration: %fs", fileName, animations[a].name, animations[a].keyframeCount, m3d->action[a].durationmsec);

            for (i = 0; i < (int)m3d->numbone; i++) {
                bones[i].parent = m3d->bone[i].parent;
                snprintf(bones[i].name, sizeof(bones[i].name), "%s", m3d->bone[i].name);
            }

            // A special, never transformed "no bone" bone, used for boneless vertices
            bones[i].parent = -1;
            memcpy(bones[i].name, "NO BONE", 7);

            // M3D stores frames at arbitrary intervals with sparse skeletons; Full skeletons is required at
            // regular intervals, so let the M3D SDK do the heavy lifting and calculate interpolated bones
            for (i = 0; i < animations[a].keyframeCount; i++) {
                animations[a].keyframePoses[i] = (Transform *)RL_CALLOC((m3d->numbone + 1), sizeof(Transform));

                m3db_t *pose = m3d_pose(m3d, a, i*M3D_ANIMDELAY);

                if (pose != nullptr) {
                    for (j = 0; j < (int)m3d->numbone; j++) {
                        animations[a].keyframePoses[i][j].translation.x = m3d->vertex[pose[j].pos].x*m3d->scale;
                        animations[a].keyframePoses[i][j].translation.y = m3d->vertex[pose[j].pos].y*m3d->scale;
                        animations[a].keyframePoses[i][j].translation.z = m3d->vertex[pose[j].pos].z*m3d->scale;
                        animations[a].keyframePoses[i][j].rotation.x = m3d->vertex[pose[j].ori].x;
                        animations[a].keyframePoses[i][j].rotation.y = m3d->vertex[pose[j].ori].y;
                        animations[a].keyframePoses[i][j].rotation.z = m3d->vertex[pose[j].ori].z;
                        animations[a].keyframePoses[i][j].rotation.w = m3d->vertex[pose[j].ori].w;
                        animations[a].keyframePoses[i][j].rotation = QuaternionNormalize(animations[a].keyframePoses[i][j].rotation);
                        animations[a].keyframePoses[i][j].scale.x = animations[a].keyframePoses[i][j].scale.y = animations[a].keyframePoses[i][j].scale.z = 1.0f;

                        // Child bones are stored in parent bone relative space, convert that into model space
                        if (bones[j].parent >= 0) {
                            animations[a].keyframePoses[i][j].rotation = QuaternionMultiply(animations[a].keyframePoses[i][bones[j].parent].rotation, animations[a].keyframePoses[i][j].rotation);
                            animations[a].keyframePoses[i][j].translation = Vector3RotateByQuaternion(animations[a].keyframePoses[i][j].translation, animations[a].keyframePoses[i][bones[j].parent].rotation);
                            animations[a].keyframePoses[i][j].translation = Vector3Add(animations[a].keyframePoses[i][j].translation, animations[a].keyframePoses[i][bones[j].parent].translation);
                            animations[a].keyframePoses[i][j].scale = Vector3Multiply(animations[a].keyframePoses[i][j].scale, animations[a].keyframePoses[i][bones[j].parent].scale);
                        }
                    }

                    // Default transform for the "no bone" bone
                    animations[a].keyframePoses[i][j].translation.x = 0.0f;
                    animations[a].keyframePoses[i][j].translation.y = 0.0f;
                    animations[a].keyframePoses[i][j].translation.z = 0.0f;
                    animations[a].keyframePoses[i][j].rotation.x = 0.0f;
                    animations[a].keyframePoses[i][j].rotation.y = 0.0f;
                    animations[a].keyframePoses[i][j].rotation.z = 0.0f;
                    animations[a].keyframePoses[i][j].rotation.w = 1.0f;
                    animations[a].keyframePoses[i][j].scale.x = 1.0f;
                    animations[a].keyframePoses[i][j].scale.y = 1.0f;
                    animations[a].keyframePoses[i][j].scale.z = 1.0f;

                    RL_FREE(pose);
                }
            }

            RL_FREE(bones);
        }

        m3d_free(m3d);
        UnloadFileData(fileData);
    }

    return animations;
}
#endif

void *LoadModelCPU(const char *path) {
    Model *model = malloc(sizeof(Model));

    if (IsFileExtension(path, ".obj")) *model = LoadOBJ(path);
    else if (IsFileExtension(path, ".iqm")) *model = LoadIQM(path);
    else if (IsFileExtension(path, ".gltf") || IsFileExtension(path, ".glb")) *model = LoadGLTF(path);
    else if (IsFileExtension(path, ".vox")) *model = LoadVOX(path);
    else if (IsFileExtension(path, ".m3d")) *model = LoadM3D(path);
    else {
        TraceLog(LOG_WARNING, "MODEL: [%s] Unknown model extension", path);
        free(model);
        return nullptr;
    }

    model->transform = MatrixIdentity();

    return model;
}

void *LoadModelGPU(void *data) {
    Model *model = data;

    if ((model->meshCount != 0) && (model->meshes != NULL)) {
        // Upload vertex data to GPU (static meshes)
        for (int i = 0; i < model->meshCount; i++) UploadMesh(&model->meshes[i], false);
    } else TraceLog(LOG_WARNING, "MESH: Failed to load model mesh(es) data");

    if (model->materialCount == 0) {
        TraceLog(LOG_WARNING, "MATERIAL: Failed to load model material data, default to white material");

        model->materialCount = 1;
        model->materials = (Material *)RL_CALLOC(model->materialCount, sizeof(Material));
        model->materials[0] = LoadMaterialDefault();

        if (model->meshMaterial == nullptr) model->meshMaterial = (int *)RL_CALLOC(model->meshCount, sizeof(int));
    }
}

// Default values for ttf font generation
#ifndef FONT_TTF_DEFAULT_SIZE
#define FONT_TTF_DEFAULT_SIZE           32      // TTF font generation default char size (char-height)
#endif
#ifndef FONT_TTF_DEFAULT_NUMCHARS
#define FONT_TTF_DEFAULT_NUMCHARS       95      // TTF font generation default charset: 95 glyphs (ASCII 32..126)
#endif
#ifndef FONT_TTF_DEFAULT_FIRST_CHAR
#define FONT_TTF_DEFAULT_FIRST_CHAR     32      // TTF font generation default first char for image sprite font (32-Space)
#endif
#ifndef FONT_TTF_DEFAULT_CHARS_PADDING
#define FONT_TTF_DEFAULT_CHARS_PADDING   4      // TTF font generation default glyphs padding
#endif

typedef struct FontCPU {
    const char *path;

    GlyphInfo *glyphs;
    int glyphCount;

    Rectangle *recs;

    int baseSize;
    int glyphPadding;

    Image atlas;
} FontCPU;

void *LoadFontCPU(const char *path) {
    FontCPU *font = malloc(sizeof(*font));
    memset(font, 0, sizeof(*font));

    #if SUPPORT_FILEFORMAT_TTF
    if (IsFileExtension(path, ".ttf") || IsFileExtension(path, ".otf")) {
        int dataSize = 0;
        unsigned char *fileData = LoadFileData(path, &dataSize);

        if (fileData != nullptr) {
            char fileExtLower[16] = { 0};
            snprintf(fileExtLower, 16, "%s", TextToLower(GetFileExtension(path)));

            font->baseSize = FONT_TTF_DEFAULT_SIZE;
            font->glyphPadding = 0;

            int codepoints[95];
            for (int i = 0; i < 95; i++) {
                codepoints[i] = 32 + i;
            }

            font->glyphs = LoadFontData(fileData, dataSize, font->baseSize, codepoints, 95, FONT_DEFAULT, &font->glyphCount);

            font->glyphCount = 95;

            UnloadFileData(fileData);

            font->atlas = GenImageFontAtlas(font->glyphs, &font->recs, font->glyphCount, font->baseSize, font->glyphPadding, 0);
        }
    } else
    #endif
    {
        // const Image image = LoadImage(path);
        // if (image.data != nullptr) font = LoadFontFromImage(image, MAGENTA, FONT_TTF_DEFAULT_FIRST_CHAR);
        // else font = GetFontDefault();
        // UnloadImage(image);
    }

    font->path = path;

    TraceLog(LOG_INFO, "FONT: [%s] Successfully loaded font", path);

    return font;
}

void FreeFontCPU(void *data) {
    const FontCPU *font = data;

    TraceLog(LOG_INFO, "FONT: [%s] Successfully unloaded font", font->path);

    free(data);
}

void *LoadFontGPU(void *data) {
    const FontCPU *font_data = data;

    Font *font = malloc(sizeof(Font));
    font->baseSize = font_data->baseSize;
    font->glyphCount = font_data->glyphCount;
    font->glyphPadding = font_data->glyphPadding;
    font->recs = font_data->recs;
    font->glyphs = font_data->glyphs;

    font->texture = LoadTextureFromImage(font_data->atlas);

    #if SUPPORT_FILEFORMAT_TTF || SUPPORT_FILEFORMAT_BDF
    if (font->glyphs != nullptr) {
        font->texture = LoadTextureFromImage(font_data->atlas);

        // Update glyphs[i].image to use alpha, required to be used on ImageDrawText()
        for (int i = 0; i < font->glyphCount; i++) {
            UnloadImage(font->glyphs[i].image);
            font->glyphs[i].image = ImageFromImage(font_data->atlas, font->recs[i]);
        }

        TraceLog(LOG_INFO, "FONT: Data loaded successfully (%i pixel size | %i glyphs)", font->baseSize, font->glyphCount);
    } else {
        TraceLog(LOG_WARNING, "FONT: Font is not supported by LoadFontEx/LoadFontFromMemory or no glyphs found, reverted to default font");
        *font = GetFontDefault();
    }
    #else
    TraceLog(LOG_WARNING, "FONT: Font is not supported by LoadFontEx/LoadFontFromMemory or no glyphs found, reverted to default font");
    font = GetFontDefault();
    #endif

    if (font->texture.id == 0) TraceLog(LOG_WARNING, "FONT: [%s] Failed to upload font texture -> Using default font", font_data->path);
    else {
        SetTextureFilter(font->texture, TEXTURE_FILTER_POINT); // By default, set point filter (the best performance)
        TraceLog(LOG_INFO, "FONT: Successfully uploaded font");
    }

    return font;
}

void FreeFontGPU(void *data) {
    Font *font = data;

    if (font->texture.id != GetFontDefault().texture.id) {
        UnloadFontData(font->glyphs, font->glyphCount);
        UnloadTexture(font->texture);
        free(font->recs);

        TraceLog(LOG_DEBUG, "FONT: Unloaded font data from VRAM");

        free(data);
    }
}