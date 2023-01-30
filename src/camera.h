#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "mathc.h"
#include "mesh.h"
#include "shader.h"

typedef struct camera {
    mfloat_t position[VEC3_SIZE];
    mfloat_t rotation[VEC3_SIZE];
    
    float fov;
} camera;

void camera_update(int w, int h, shader s, camera cam);
struct mat4 camera_transform_mesh(shader s, mesh m, struct mat4 mdl);

#endif