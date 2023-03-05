#ifndef __MESH_H__
#define __MESH_H__

#include <cglm/cglm.h>
#include "shader.h"

typedef struct vertex {
    vec3 position;
    vec2 texture;
    vec3 normal;
} vertex;

typedef struct mesh {
    vec3 position;
    vec3 rotation;
    vec3 scale;

    vertex   *vertices;
    uint32_t *indices;
    uint32_t  vert_num;
    uint32_t  idx_num;

    uint32_t  texture;

    uint32_t VAO, VBO, EBO;


    float bound_box[4*8];

    char        *name;
    struct mesh *next;
    struct mesh *nested;
    int          renderable; 
} mesh;

mesh mesh_new_quad();
mesh mesh_load_obj(const char *file, const char *tex);

void mesh_render(mesh m, shader s, mat4 model);
void mesh_render_quad(mesh m, shader s);
#endif