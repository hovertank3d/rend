#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <cglm/cglm.h>
#include "mesh.h"
#include "shader.h"

typedef struct camera {
    vec3 position;
    vec3 rotation;
    
    float fov;
} camera;

void camera_update(int w, int h, shader s, camera cam, mat4 view, mat4 proj);
void camera_transform_mesh(shader s, mesh m, mat4 mdl);

#endif