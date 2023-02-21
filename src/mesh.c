#include "mesh.h"

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

#include "mathc.h"
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

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(mfloat_t)*3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(mfloat_t)*5));
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

void mesh_init_push(mesh **head, int vbo, uint32_t *indices, int indices_num, char *tex)
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
    last->transform = 1;


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

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(mfloat_t)*3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(mfloat_t)*5));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
}

mesh mesh_load_obj(const char *file, const char *tex)
{
    mesh root = {0};
    obj *o;
    vertex *vertices;
    int verts_num;
    int surf_num;
    int vbo;

    o = obj_create(file);
    surf_num = obj_num_surf(o);
    verts_num = obj_num_vert(o);

    vertices = calloc(verts_num, sizeof(vertex));
    for (int i = 0; i < verts_num; i++) {
        obj_get_vert_v(o, i, (float *)&vertices[i].position);
        obj_get_vert_n(o, i, (float *)&vertices[i].normal);
        obj_get_vert_t(o, i, (float *)&vertices[i].texture);
    }

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
        mesh_init_push(&root.nested, vbo, indices, indices_num, tex);

        free(indices);
    }

    obj_delete(o);

    return root;
}

void mesh_render(mesh m, shader s, struct mat4 model)
{
    struct mat4 parent_model;
    mesh *m_temp;

    parent_model = camera_transform_mesh(s, m, model);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.texture);
    
    glUseProgram(s);
    glBindVertexArray(m.VAO); 
    glDrawElements(GL_TRIANGLES, m.idx_num, GL_UNSIGNED_INT, 0);

    m_temp = m.nested;

    while (m_temp) {
        mesh_render(*m_temp, s, parent_model);
        m_temp = m_temp->next;
    } 
}

void mesh_render_quad(mesh m, shader s)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.texture);

    glUseProgram(s);

    glBindVertexArray(m.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}