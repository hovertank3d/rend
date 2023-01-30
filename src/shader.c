#include "shader.h"

#include <glad/glad.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>

#include "error.h"

mrerror shader_compile(uint32_t *s, int type, const char *file)
{
    int shader_fd;
    struct stat file_stat;
    char *shader_source;

    shader_fd = open(file, O_RDONLY);
    if (shader_fd <= 0) {
        return mrerror_new("cant load shader lmao");
    }
    fstat(shader_fd, &file_stat);

    shader_source = malloc(file_stat.st_size + 1);
    if (read(shader_fd, shader_source, file_stat.st_size) != file_stat.st_size) {
        goto err_read;
    }
    shader_source[file_stat.st_size] = 0;

    *s = glCreateShader(type);
    glShaderSource(*s, 1, &shader_source, NULL);
    glCompileShader(*s);

    free(shader_source);

    int success;
    char infoLog[512];
    glGetShaderiv(*s, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(*s, 512, NULL, infoLog);
        printf("info %s\n\n", infoLog);
        return mrerror_new(infoLog);
    }

    return nilerr();

err_read:
    free(shader_source);
    printf("xxxn\\n\n\n");
    return mrerror_new("cant read file lmao");
}

mrerror shader_new(shader *s, const char *vert_path, const char *frag_path)
{
    uint32_t vert, frag;

    shader_compile(&vert, GL_VERTEX_SHADER, vert_path);
    shader_compile(&frag, GL_FRAGMENT_SHADER, frag_path);

    *s = glCreateProgram();
    glAttachShader(*s, vert);
    glAttachShader(*s, frag);
    glLinkProgram(*s);

    int success;
    char infoLog[512];
    glGetProgramiv(*s, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(*s, 512, NULL, infoLog);
        printf("info %s\n\n", infoLog);
        return mrerror_new(infoLog);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return nilerr();
}
