#include "camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void camera_update(int w, int h, shader s, camera cam)
{
    struct mat4 proj, view;

    mat4_perspective_fov(&proj, cam.fov, w, h, 0.1, 100.0);
    glUniformMatrix4fv(glGetUniformLocation(s, "projection"), 1, GL_FALSE, &proj);

    //TODO!!!!
    mfloat_t xd[] = {
        cam.position[0], cam.position[1]-1, cam.position[2]
    };
    mfloat_t xd2[] = {
        0.0, 0.0, 1.0
    };

    mat4_look_at(&view, cam.position, xd, xd2);
    glUniformMatrix4fv(glGetUniformLocation(s, "view"), 1, GL_FALSE, &view);

}

struct mat4 camera_transform_mesh(shader s, mesh m, struct mat4 mdl)
{
    struct vec3 roty = {1.0, 0.0, 0.0};
    struct vec3 rotx = {0.0, 1.0, 0.0};
    
    struct mat4 rot;
    mat4_identity(&rot);

    mat4_rotation_axis(&rot, &roty, m.rotation[1]);
    mat4_multiply(&mdl, &mdl, &rot);
    mat4_rotation_axis(&rot, &rotx, m.rotation[0]);
    mat4_multiply(&mdl, &mdl, &rot);


    glUniformMatrix4fv(glGetUniformLocation(s, "model"), 1, GL_FALSE, &mdl);

    return mdl;
}