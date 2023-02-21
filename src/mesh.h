#ifndef __MESH_H__
#define __MESH_H__

#include "mathc.h"
#include "shader.h"

typedef struct vertex {
    mfloat_t position[VEC3_SIZE];
    mfloat_t texture[VEC2_SIZE];
    mfloat_t normal[VEC3_SIZE];
} vertex;

typedef struct mesh {
    mfloat_t position[VEC3_SIZE];
    mfloat_t rotation[VEC3_SIZE];
    mfloat_t scale[VEC3_SIZE];

    vertex   *vertices;
    uint32_t *indices;
    uint32_t  vert_num;
    uint32_t  idx_num;

    uint32_t  texture;

    uint32_t VAO, VBO, EBO;


    char        *name;
    struct mesh *next;
    struct mesh *nested;
    int          transform; //is matrix transform neccesarry     
} mesh;

mesh mesh_new_quad();
mesh mesh_load_obj(const char *file, const char *tex);

void mesh_render(mesh m, shader s, struct mat4 model);
void mesh_render_quad(mesh m, shader s);
#endif