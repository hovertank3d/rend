#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <glad/glad.h>
#include <glad/glad_egl.h>

#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "camera.h"
#include "error.h"
#include "shader.h"
#include "texture.h"
#include "mesh.h"

#include <cglm/cglm.h>

struct renderer {
    mesh scene;
    mesh background_quad;
    shader s;
    shader bg;
    camera cam;
};

struct application {
    const char name[64];
    int distance;
    char texture_path[64];
    char background_images_path[64];
    char model_path[64];
    char frames_path[64];
    char annotations_path[64];
    char imagesets_path[64];
    char working_dir[64];
    int w, h;
    int ps, pe; //pitch start and end
    int ys, ye; 

    int thermal;

    int bg_count; 
    texture *backgrounds;

    GLFWwindow *wnd;

    struct renderer rend;
};


int is_image(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (ext != NULL)
    {
        if (strcasecmp(ext, ".bmp") == 0 ||
            strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, ".tga") == 0)
        {
            return 1;
        }
    }
    return 0;
}

mrerror load_background_textures(const char *dir, int *count, texture **textures)
{
    char pathbuf[128];
    mrerror err;

    *count = 0;
    *textures = NULL;

    DIR *dirp = opendir(dir);
    if (dirp == NULL) {
        return mrerror_new("opendir");
    }

    struct dirent *entry;
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG && is_image(entry->d_name)) {
            snprintf(pathbuf, 128, "%s/%s", dir, entry->d_name);

            *textures = (texture *)realloc(*textures, (*count + 1) * sizeof(texture));

            err = texture_find(&((*textures)[*count]), pathbuf);
            if (err.err) {
                free(*textures);
                closedir(dirp);
                return err;
            }
            (*count)++;
        }
    }

    closedir(dirp);

    return nilerr();
}

mrerror initGLFW(struct application *app)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow *window = glfwCreateWindow(app->w, app->h, "mr", NULL, NULL);
    if (window == NULL)
    {
        glfwTerminate();
        return mrerror_new("glfwCreateWindow");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    app->wnd = window;
    return nilerr();
}

void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    fprintf(stderr, "OpenGL Debug Message: %s\n", message);
}

mrerror initGL(struct application *app)
{   
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return mrerror_new("Can't init GL");
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_MULTISAMPLE);  
    glEnable(GL_CULL_FACE); 
    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC)debug_callback, 0);

    glClearColor(0.5, 0.1, 0.4, 1.0);

    glViewport(0, 0, app->w, app->h);
    app->rend.scene = mesh_load_obj(app->model_path, app->texture_path);
    app->rend.scene.rotation[0] = glm_rad(90.0f); 
    shader_new(&app->rend.s, "assets/vert.glsl", app->thermal ? "assets/thermal_frag.glsl" : "assets/frag.glsl");

    app->rend.background_quad = mesh_new_quad();
    shader_new(&app->rend.bg, "assets/bg_vert.glsl", app->thermal ? "assets/bg_thermal_frag.glsl" : "assets/bg_frag.glsl");

    app->rend.cam = (camera){
        {0, app->distance, 0},
        {0, 0, 0},
        45.0
    };

    return nilerr();
}

#define SPOS(w, x) ((w/2.0)*(1 + x))

const char annotation_head[] = "<annotation><folder>%s</folder><filename>%s</filename><path>%s</path><source><database>Unknown</database></source><size><width>%d</width><height>%d</height><depth>3</depth></size><segmented>0</segmented>";
const char annotation_object[] = "<object><name>%s</name><pose>Unspecified</pose><truncated>0</truncated><difficult>0</difficult><bndbox><xmin>%d</xmin><ymin>%d</ymin><xmax>%d</xmax><ymax>%d</ymax></bndbox></object>";
const char annotation_tail[] = "</annotation>";

void export_annotation(const char *filename, const char *imagefile, struct application app, mesh m, mat4 model, mat4 view, mat4 proj)
{
    mat4 mvp;
    float min[2], max[2], temp;

    min[0] = min[1] = FLT_MAX;
    max[0] = max[1] = -FLT_MAX;

    glm_mat4_mul(proj, view, mvp);
    glm_mat4_mul(mvp, model, mvp);

    for (int i = 0; i < 8; i++) {
        vec4 vert, result;
        vec2 screen;

        vert[0] = m.bound_box[i*4];
        vert[1] = m.bound_box[i*4+1];
        vert[2] = m.bound_box[i*4+2];
        vert[3] = m.bound_box[i*4+3];


        glm_mat4_mulv(mvp, vert, result);
        screen[0] = result[0] / (result[3]*1.06);
        screen[1] = result[1] / (result[3]*1.06);

        min[0] = min[0] < screen[0] ? min[0] : screen[0];
        min[1] = min[1] < screen[1] ? min[1] : screen[1];
        max[0] = max[0] > screen[0] ? max[0] : screen[0];
        max[1] = max[1] > screen[1] ? max[1] : screen[1];
    }

    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return;
    }    

    int xmin, xmax, ymin, ymax;
    xmin = (int)(SPOS(app.w, min[0]));
    xmax = (int)(SPOS(app.w, max[0]));
    ymin = (int)(SPOS(app.h, min[1]));
    ymax = (int)(SPOS(app.h, max[1]));

    fprintf(f, annotation_head, strrchr(app.frames_path, '/') + 1, strrchr(imagefile, '/') + 1, realpath(imagefile, NULL), app.w, app.h);
    fprintf(f, annotation_object, app.name, xmin, ymin, xmax, ymax);
    fprintf(f, annotation_tail);
    fclose(f);
}

mrerror export_png(char *filename)
{
    uint32_t viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0];
	int y = viewport[1];
	int width = viewport[2];
	int height = viewport[3];

    char *data = (char*) calloc(1, (size_t) (width * height * 3));

	if (!data)
		return mrerror_new("malloc error");

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

    stbi_write_png(filename, width, height, 3, data, 0);

    free(data);

    return nilerr();
}

void render_frame(struct application app)
{
    static int frame_count = 0;
    frame_count++;

    char image_filename[256];
    char annotation_filename[256];
    mat4 model, view, proj;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera_update(app.w, app.h, app.rend.s, app.rend.cam, view, proj);

    glm_mat4_identity(model);
    mesh_render(app.rend.scene, app.rend.s, model);

    app.rend.background_quad.texture = app.backgrounds[rand()%app.bg_count];
    mesh_render_quad(app.rend.background_quad, app.rend.bg);

    // saving result

    snprintf(image_filename, 64, "%s/%d.png", app.frames_path, frame_count);
    export_png(image_filename);
    snprintf(annotation_filename, 64, "%s/%d.xml", app.annotations_path, frame_count);
    export_annotation(annotation_filename, image_filename, app, app.rend.scene, model, view, proj);
}

static void rmkdir(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    strncpy(tmp, dir, sizeof(tmp));
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

void app_main(struct application app)
{
    char filename[64];
    
    int frames_count = 1;

    for (int x = app.ys/5; x <= app.ye/5; x++) {
        for (int y = app.ps/5; y <= app.pe/5; y++) {
            if (glfwWindowShouldClose(app.wnd))
                return;

            app.rend.scene.rotation[0] = glm_rad(x*5);
            app.rend.scene.rotation[1] = glm_rad(y*5);

            frames_count++;

            render_frame(app);

            glfwSwapBuffers(app.wnd);
            glfwPollEvents();  
        }
    }

    frames_count--;

    unsigned char *picked = calloc(frames_count/8 + 1, 1); // bitset of size 100
    int *nums = calloc(frames_count, sizeof(int));

    int i, j, rand_num;

    srand(time(NULL)); // initialize the random seed
    for (i = 0; i < frames_count/8 + 1; i++) {
        picked[i] = 0;
    }

    for (i = 0; i < frames_count; i++) {
        do {
            rand_num = rand() % frames_count + 1;
        } while (picked[rand_num/8] & (1 << rand_num%8)); // check if already picked
        picked[rand_num/8] |= (1 << rand_num%8); // mark as picked
        nums[i] = rand_num;
    }

    FILE *test_iset, *train_iset, *trainval_iset, *val_iset, *labels;
    snprintf(filename, 64, "%s/test.txt", app.imagesets_path);
    test_iset = fopen(filename, "w");
    snprintf(filename, 64, "%s/train.txt", app.imagesets_path);
    train_iset = fopen(filename, "w");
    snprintf(filename, 64, "%s/trainval.txt", app.imagesets_path);
    trainval_iset = fopen(filename, "w");
    snprintf(filename, 64, "%s/val.txt", app.imagesets_path);
    val_iset = fopen(filename, "w");
    snprintf(filename, 64, "%s/labels.txt", app.working_dir);
    labels = fopen(filename, "w");

    fputs(app.name, labels);
    fclose(labels);

    for(int j = 0; j < frames_count; j++) {
        if (j < frames_count/32) {
            fprintf(test_iset, "%d\n", nums[j]);
            fprintf(val_iset, "%d\n", nums[j]);
        }
        fprintf(train_iset, "%d\n", nums[j]);
        fprintf(trainval_iset, "%d\n", nums[j]);
    }

    fclose(test_iset);
    fclose(train_iset);
    fclose(trainval_iset);
    fclose(val_iset);

    printf("\n");
}

int main(int argc, char **argv)
{
    mrerror err;
    struct application app;

    memset(&app, 0, sizeof(struct application));

    app.distance = 13;

    app.w = 512;
    app.h = 512;

    app.ys = 0;
    app.ye = 360;
    app.ps = -45;
    app.pe = 45;

    app.thermal = 0;

    int opt;
      
    // put ':' in the starting of the
    // string so that program can 
    //distinguish between '?' and ':' 
    while((opt = getopt(argc, argv, "m:t:d:o:w:h:a:b:l:n:")) != -1) 
    { 
        switch(opt) 
        {
            //model
            case 'm': 
                strncpy(app.model_path, optarg, 64);
                break;
            //texture 
            case 't': 
                strncpy(app.texture_path, optarg, 64);
                break;
            //path 
            case 'o': 
                strncpy(app.working_dir, optarg, 64);
                snprintf(app.frames_path, 64, "%s/%s", app.working_dir, "JPEGImages");
                snprintf(app.imagesets_path, 64, "%s/%s", app.working_dir, "ImageSets/Main");
                snprintf(app.annotations_path, 64, "%s/%s", app.working_dir, "Annotations");
                break;
            //annotations(z as zones) 
            //distance 
            case 'd': 
                app.distance = atoi(optarg);
                break;
            //width 
            case 'w': 
                app.w = atoi(optarg);
                break;
            //heigth 
            case 'h': 
                app.h = atoi(optarg);
                break; 
            //angles
            case 'a':
                sscanf(optarg, "%dx%d %dx%d", &app.ys, &app.ye, &app.ps, &app.pe);
                break;
            //background images directory
            case 'b':
                strncpy(app.background_images_path, optarg, 64);
                break;
            // thermaL
            case 'l':
                app.thermal = 1;
                break;
            // name
            case 'n':
                strncpy(app.name, optarg, 64);
                break;
            case 'z': 
                strncpy(app.annotations_path, optarg, 64);
                break;
            case 'i': 
                strncpy(app.imagesets_path, optarg, 64);
                break;

        } 
    }

    if (!app.model_path[0]   ||
        !app.texture_path[0] ||
        !app.background_images_path[0])
    {

        printf("select model, backgrund images dir and output path\n");
        return 0;
    }

    rmkdir(app.frames_path);
    rmkdir(app.annotations_path);
    rmkdir(app.imagesets_path);

    err = initGLFW(&app);
    if (err.err) {
        printf("%s\n", err.msg);
        return 1;
    }

    err = initGL(&app);
    if (err.err) {
        printf("%s\n", err.msg);
        return 1;
    }

    load_background_textures(app.background_images_path, &app.bg_count, &app.backgrounds);

    app_main(app);
}
