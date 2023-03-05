#include "mesh.h"

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shader.h"
#include "texture.h"
#include "camera.h"

#define CONF_NO_GL
#include "obj.h"

void mesh_init(mesh *m)
{
    glGenVertexArrays(1, &m->VAO);
    glGenBuffers(1, &m->VBO);
    glGenBuffers(1, &m->EBO);

    glBindVertexArray(m->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m->VBO);
    glBufferData(GL_ARRAY_BUFFER, m->vert_num*sizeof(vertex), m->vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->idx_num*sizeof(uint32_t), m->indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float)*5));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
}

mesh mesh_new_quad()
{
    mesh quad = {0};

    float vertices[] = {
        -1.0f,  1.0f,  
        -1.0f, -1.0f,  
         1.0f, -1.0f,  
        -1.0f,  1.0f,  
         1.0f, -1.0f,  
         1.0f,  1.0f, 
    };

    glGenVertexArrays(1, &quad.VAO);
    glGenBuffers(1, &quad.VBO);

    glBindVertexArray(quad.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, quad.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);  

    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    glBindVertexArray(0); 

    quad.vert_num = 6;

    return quad;
}

void mesh_init_push(mesh **head, int vbo, uint32_t *indices, int indices_num, const char *tex)
{
    mesh *last;
    char image_path[64];

    if (!*head) {
        *head = calloc(1, sizeof(mesh));
        last = *head;
    }
    else {
        last = *head;
        while (last->next) {
            last = last->next;
        }

        last->next = malloc(sizeof(mesh));
        last = last->next;
    }

    last->idx_num = indices_num;
    last->next = NULL;
    last->nested = NULL;
    last->rotation[0] = 0;
    last->rotation[1] = 0;
    last->rotation[2] = 0;
    last->renderable = 1;


    if (texture_find(&last->texture, tex).err) {
        printf("Can't load %s, skipping\n", image_path);
    }

    glGenVertexArrays(1, &last->VAO);
    glGenBuffers(1, &last->EBO);

    glBindVertexArray(last->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_num*sizeof(uint32_t), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float)*5));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
}

void get_bounding_box(mesh *m)
{
    vec3 min = {m->vertices[0].position[0], m->vertices[0].position[1], m->vertices[0].position[2]};
    vec3 max = {m->vertices[0].position[0], m->vertices[0].position[1], m->vertices[0].position[2]};
    

    // iterate through all vertices
    for (int i = 1; i < m->vert_num; i++) {
        float x = m->vertices[i].position[0];
        float y = m->vertices[i].position[1];
        float z = m->vertices[i].position[2];
        
        // update min and max values if necessary
        if (x < min[0]) min[0] = x;
        if (x > max[0]) max[0] = x;
        if (y < min[1]) min[1] = y;
        if (y > max[1]) max[1] = y;
        if (z < min[2]) min[2] = z;
        if (z > max[2]) max[2] = z;
    }

    float bounds[] = {
        min[0], min[1], max[2], 1.0,
        max[0], min[1], max[2], 1.0,
        max[0], max[1], max[2], 1.0,
        min[0], max[1], max[2], 1.0,
        min[0], min[1], min[2], 1.0,
        max[0], min[1], min[2], 1.0,
        max[0], max[1], min[2], 1.0,
        min[0], max[1], min[2], 1.0,
    };

    memcpy(m->bound_box, bounds, sizeof(float)*4*8);
    return;
}

mesh mesh_load_obj(const char *file, const char *tex)
{
    mesh *root;
    obj *o;
    vertex *vertices;
    int verts_num;
    int surf_num;
    int vbo;

    root = calloc(1, sizeof(mesh));

    o = obj_create(file);
    surf_num = obj_num_surf(o);
    verts_num = obj_num_vert(o);

    root->vert_num = verts_num;

    vertices = calloc(verts_num, sizeof(vertex));
    for (int i = 0; i < verts_num; i++) {
        obj_get_vert_v(o, i, vertices[i].position);
        obj_get_vert_n(o, i, vertices[i].normal);
        obj_get_vert_t(o, i, vertices[i].texture);
    }
    root->vertices = vertices;

    get_bounding_box(root);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts_num * sizeof(vertex), vertices, GL_STATIC_DRAW);    

    free(vertices);

    for (int i = 0; i < surf_num; i++) {
        int *indices;
        int indices_num;
        int idx_index = 0;

        indices_num = obj_num_poly(o, i)*3;
        indices = malloc(indices_num * sizeof(uint32_t));

        for (int j = 0; j < indices_num/3; j++) {
            obj_get_poly(o, i, j, &indices[idx_index]);
            idx_index += 3;
        }
        mesh_init_push(&root->nested, vbo, indices, indices_num, tex);

        free(indices);
    }

    obj_delete(o);

    return *root;
}

void mesh_render(mesh m, shader s, mat4 model)
{
    mesh *m_temp;

    glUseProgram(s);
    camera_transform_mesh(s, m, model);

    if (m.renderable) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture);
    
        glBindVertexArray(m.VAO); 
        glDrawElements(GL_TRIANGLES, m.idx_num, GL_UNSIGNED_INT, 0);
    }

    m_temp = m.nested;

    while (m_temp) {
        mesh_render(*m_temp, s, model);
        m_temp = m_temp->next;
    }
}

void mesh_render_quad(mesh m, shader s)
{
    glUseProgram(s);

    glBindVertexArray(m.VAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}