
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glad/glad.h>
#include <glad/glad_egl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "camera.h"
#include "error.h"
#include "shader.h"
#include "mesh.h"


struct renderer {
    mesh scene;
    shader s;
    camera cam;
};

struct application {
    int distance;
    char texture_path[64];
    char model_path[64];
    char images_path[64];
    int w, h;

    struct renderer rend;
};

static const EGLint configAttribs[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
};

mrerror initEGL(struct application *app)
{
    EGLint pbufferAttribs[] = {
        EGL_WIDTH,  app->w,
        EGL_HEIGHT, app->h,
        EGL_NONE,
    };

    if (!gladLoadEGL()) {
        return mrerror_new("Can't init EGL");
    }

    EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major, minor;

    eglInitialize(eglDpy, &major, &minor);

    EGLint numConfigs;
    EGLConfig eglCfg;

    eglBindAPI(EGL_OPENGL_API);

    eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);

    EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg,
                                                 pbufferAttribs);


    EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, 
                                       NULL);

    eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

    return nilerr();
}

mrerror initGL(struct application *app)
{   
    if (!gladLoadGLLoader(eglGetProcAddress)) {
        return mrerror_new("Can't init GL");
    }

    glViewport(0, 0, app->w, app->h);

    app->rend.scene = mesh_load_obj(app->model_path, app->texture_path);
    shader_new(&app->rend.s, "assets/vert.glsl", "assets/frag.glsl");

    glEnable(GL_DEPTH_TEST);

    app->rend.cam = (camera){
        {0, app->distance, 0},
        {0, 0, 0},
        45.0
    };

    return nilerr();
}

mrerror export_png(char *filename)
{
    uint32_t viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0];
	int y = viewport[1];
	int width = viewport[2];
	int height = viewport[3];

    char *data = (char*) malloc((size_t) (width * height * 4));

	if (!data)
		return mrerror_new("malloc error");

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_write_png(filename, width, height, 4, data, 0);

    free(data);

    return nilerr();
}

void render_frame(struct application app)
{
    glClearColor(0.5, 0.1, 0.4, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera_update(app.w, app.h, app.rend.s, app.rend.cam);

    struct mat4 temp; // todo: move it to better place
    mat4_identity(&temp);
    mesh_render(app.rend.scene, app.rend.s, temp);
        
}

void app_main(struct application app)
{
    //while(!glfwWindowShouldClose(app.window))
    //{
    for (int x = 0; x < 360; x++)
    {
        for (int y = -45; y <= 45; y++)
        {
            app.rend.scene.rotation[0] = to_radians(x);
            app.rend.scene.rotation[1] = to_radians(y);

            render_frame(app);

            char filename[64];
            snprintf(filename, 64, "%s/%d_%d.png", app.images_path, x, y);
            export_png(filename);

        }
    }
    //}
}

static void rmkdir(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

int main(int argc, char **argv)
{
    mrerror err;
    struct application app;

    app.distance = 13;
    memset(app.texture_path, 0, 64*2);
    strncpy(app.images_path, "tank", 64);

    app.w = 512;
    app.h = 512;

    int opt;
      
    // put ':' in the starting of the
    // string so that program can 
    //distinguish between '?' and ':' 
    while((opt = getopt(argc, argv, "m:t:d:p:w:h:")) != -1) 
    { 
        switch(opt) 
        {
            case 'm': 
                strncpy(app.model_path, optarg, 64);
                break; 
            case 't': 
                strncpy(app.texture_path, optarg, 64);
                break; 
            case 'p': 
                strncpy(app.images_path, optarg, 64);
                break; 
            case 'd': 
                app.distance = atoi(optarg);
                break; 
            case 'w': 
                app.w = atoi(optarg);
                break; 
            case 'h': 
                app.h = atoi(optarg);
                break; 
        } 
    } 

    //printf("%s %s %d\n", model_path, texture_path, distance);
    rmkdir(app.images_path);

    err = initEGL(&app);
    if (err.err) {
        printf("%s\n", err.msg);
        return 1;
    }

    err = initGL(&app);
    if (err.err) {
        printf("%s\n", err.msg);
        return 1;
    }

    app_main(app);
}