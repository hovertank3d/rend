#include "camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void camera_update(int w, int h, shader s, camera cam, mat4 view, mat4 proj)
{
    glUseProgram(s);
    
    glm_perspective(cam.fov, (float)w / (float)h, 0.1f, 100.0f, proj);
    glUniformMatrix4fv(glGetUniformLocation(s, "projection"), 1, GL_FALSE, proj[0]);

    vec3 xd = {0, 0, 0};
    vec3 xd2 = {0, 0, 1};

    glm_lookat(cam.position, xd, xd2, view);
    glUniformMatrix4fv(glGetUniformLocation(s, "view"), 1, GL_FALSE, view[0]);

}

void camera_transform_mesh(shader s, mesh m, mat4 mdl)
{
    glUseProgram(s);

    glm_rotate_x(mdl, m.rotation[1], mdl);
    glm_rotate_y(mdl, m.rotation[0], mdl);

    glUniformMatrix4fv(glGetUniformLocation(s, "model"), 1, GL_FALSE, mdl[0]);
}