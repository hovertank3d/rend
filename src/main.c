
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <glad/glad.h>
#include <glad/glad_egl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#include <libswscale/swscale.h>

#include "camera.h"
#include "error.h"
#include "shader.h"
#include "mesh.h"

struct video_instance {
    AVCodec        *codec;
    AVCodecContext *c;
    FILE           *f;
    AVFrame        *frame;
    AVPacket       *pkt;
    int             frames_num;
};

struct renderer {
    mesh scene;
    shader s;
    camera cam;
};

struct application {
    int distance;
    char texture_path[64];
    char model_path[64];
    char video_path[64];
    int w, h;
    int ps, pe; //pitch start and end
    int ys, ye; 

    int framerate;
    struct video_instance vid;

    struct renderer rend;
};

static const EGLint configAttribs[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_SAMPLES, 4,
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
    if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress)) {
        return mrerror_new("Can't init GL");
    }

    glViewport(0, 0, app->w, app->h);

    app->rend.scene = mesh_load_obj(app->model_path, app->texture_path);
    shader_new(&app->rend.s, "assets/vert.glsl", "assets/frag.glsl");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);  
    glEnable(GL_CULL_FACE);  
    
    app->rend.cam = (camera){
        {0, app->distance, 0},
        {0, 0, 0},
        45.0
    };

    return nilerr();
}

mrerror init_encode(struct application *app)
{
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    char *filename[64];

    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        return mrerror_new("Codec 'libx264' not found");
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
        return mrerror_new("Could not allocate video codec context");
    }
    pkt = av_packet_alloc();
    if (!pkt) {
        return mrerror_new("Could not allocate packet");
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = app->w;
    c->height = app->h;
    /* frames per second */
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){app->framerate, 1};
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        return mrerror_new(av_err2str(ret));
    }

    snprintf(filename, 64, "/tmp/%lu.raw", getpid());
    f = fopen(filename, "wb");

    if (!f) {
        return mrerror_new("Could not open file\n");
    }
    frame = av_frame_alloc();
    if (!frame) {
        return mrerror_new("Could not allocate video frame\n");
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    frame->pts = 0;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        return mrerror_new("Could not allocate the video frame data\n");
    }


    app->vid.c = c;
    app->vid.codec = codec;
    app->vid.f = f;
    app->vid.frame = frame;
    app->vid.pkt = pkt;

    return nilerr();
}

mrerror ffmpeg_encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;

    /* send the frame to the encoder */
    
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        return mrerror_new("Error sending a frame for encoding\n");
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
        return mrerror_new("Error durning encoding\n");
        }

        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

mrerror add_frame(struct video_instance *vid)
{
    uint32_t viewport[4];
    int ret;
    struct SwsContext *sws_ctx; // for rgb-yuv conversion
    uint8_t *prgb24;
    uint8_t *rgb24[1];
    int rgb24_stride[1];

	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0];
	int y = viewport[1];
	int width = viewport[2];
	int height = viewport[3];

    char *data = (char*) malloc((size_t) (width * height * 3));

	if (!data)
		return mrerror_new("malloc error");

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

    printf("rendering: \t%d%%\r", (vid->frame->pts * 100)/vid->frames_num);
    fflush(stdout);

    ret = av_frame_make_writable(vid->frame);
    if (ret < 0)
        exit(1);
    
    rgb24[0] = data;
    rgb24_stride[0] = 3 * vid->c->width;
    
    // Convert from RGB to YUV
    sws_ctx = sws_getContext(
        vid->c->width,
        vid->c->height,
        AV_PIX_FMT_RGB24,
        vid->c->width,
        vid->c->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);
    
    sws_scale(sws_ctx, rgb24, rgb24_stride,
              0, vid->c->height, vid->frame->data,
              vid->frame->linesize);

    free(prgb24);

    vid->frame->pts++;

    /* encode the image */
    ffmpeg_encode(vid->c, vid->frame, vid->pkt, vid->f);

    free(data);
    return nilerr();
}

void render_frame(struct application app)
{
    glClearColor(1, 1, 1, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera_update(app.w, app.h, app.rend.s, app.rend.cam);

    struct mat4 temp; // todo: move it to better place
    mat4_identity(&temp);
    mesh_render(app.rend.scene, app.rend.s, temp);
        
}

void app_main(struct application app)
{
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    render_frame(app);
    
    for (int x = app.ys/3; x <= app.ye/3; x++) {
        for (int y = app.ps/3; y <= app.pe/3; y++) {
            app.rend.scene.rotation[0] = to_radians(x*3);
            app.rend.scene.rotation[1] = to_radians(y*3);

            render_frame(app);
            add_frame(&app.vid);
        }
    }
    printf("\n");
    
    ffmpeg_encode(app.vid.c, NULL, app.vid.pkt, app.vid.f);

    if (app.vid.codec->id == AV_CODEC_ID_MPEG1VIDEO ||
        app.vid.codec->id == AV_CODEC_ID_MPEG2VIDEO) {

        fwrite(endcode, 1, sizeof(endcode), app.vid.f);
    }
    fclose(app.vid.f);
}

void vid_to_container(struct application app)
{
    char command[128];
    snprintf(command, 128, "ffmpeg -i /tmp/%lu.raw -r %d %s", getpid(), app.framerate, app.video_path);
    system(command);
    snprintf(command, 128, "/tmp/%lu.raw", getpid());
    unlink(command);
}

int main(int argc, char **argv)
{
    mrerror err;
    struct application app;

    memset(&app, 0, sizeof(struct application));

    app.distance = 13;
    strncpy(app.video_path, "tank.mp4", 64);

    app.w = 512;
    app.h = 512;
    app.framerate = 1;

    app.ys = 0;
    app.ye = 360;
    app.ps = -45;
    app.pe = 45;

    int opt;
      
    // put ':' in the starting of the
    // string so that program can 
    //distinguish between '?' and ':' 
    while((opt = getopt(argc, argv, "m:t:d:p:w:h:a:r:f:")) != -1) 
    { 
        switch(opt) 
        {
            //framerate
            case 'f': 
                app.framerate = atoi(optarg);
                break;
            //model
            case 'm': 
                strncpy(app.model_path, optarg, 64);
                break;
            //texture 
            case 't': 
                strncpy(app.texture_path, optarg, 64);
                break;
            //path 
            case 'p': 
                strncpy(app.video_path, optarg, 64);
                break;
            //distance 
            case 'd': 
                app.distance = atoi(optarg);
                break;
            //width 
            case 'w': 
                app.w = atoi(optarg);
                app.w = pow(2, ceil(log(app.w)/log(2)));
                break;
            //heigth 
            case 'h': 
                app.h = atoi(optarg);
                app.h = pow(2, ceil(log(app.h)/log(2)));
                break; 
            //angles
            case 'a':
                sscanf(optarg, "%dx%d %dx%d", &app.ys, &app.ye, &app.ps, &app.pe);
                break;

        } 
    } 

    if (!app.model_path[0] ||
        !app.texture_path[0]) {
        
        printf("select model and path\n");
        return 0;
    } 

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

    err = init_encode(&app);
    if (err.err) {
        printf("%s\n", err.msg);
        return 1;
    }

    app.vid.frames_num = (abs(app.ye - app.ys)/3 + 1)*(abs(app.pe - app.ps)/3 + 1);

    app_main(app);
    vid_to_container(app);

    avcodec_free_context(&app.vid.c);
    av_frame_free(&app.vid.frame);
    av_packet_free(&app.vid.pkt);
}