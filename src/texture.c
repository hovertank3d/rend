#include "texture.h"

#include <stdint.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "error.h"

struct texture_entry
{
    char *name;
    texture t;

    struct texture_entry *next;
} textures_head = {0};

mrerror texture_new(texture *t, const char *file)
{
    int w, h, comp;
    int name_len;
    struct texture_entry *temp;
    struct texture_entry *entry;

    uint8_t *data = stbi_load(file, &w, &h, &comp, 4);
    if (data == NULL) {
        stbi_image_free(data);
        printf("KEKEKEKEKEKK LMFAO\n");
        return mrerror_new(stbi_failure_reason());
    }

    glGenTextures(1, t);
    glBindTexture(GL_TEXTURE_2D, *t);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    temp = &textures_head;
    while (temp->next) {
        temp = temp->next;
    }

    temp->next = malloc(sizeof(struct texture_entry));
    
    entry = temp->next;

    name_len = strlen(file);
    entry->name = malloc(name_len + 1);
    entry->name[name_len] = 0;
    strcpy(entry->name, file);

    entry->t = *t;
    entry->next = 0;

    return nilerr();
}

mrerror texture_find(texture *t, const char *name)
{
    struct texture_entry *temp;
    
    temp = textures_head.next;
    while (temp) {
        if (!strcmp(temp->name, name)) {
            *t = temp->t;

            return nilerr();
        }
        temp = temp->next;
    }
    return texture_new(t, name);
}

void texture_free_all()
{
    struct texture_entry *entry;
    struct texture_entry *temp;
    
    entry = textures_head.next;
    while (entry->next) {
        temp = entry;
        entry = entry->next;

        free(temp);
    }
}
