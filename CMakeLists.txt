cmake_minimum_required(VERSION 3.1.0)

project(model_renderer C)

set(MODEL_RENDERER_SOURCES
    "src/main.c"
    "src/input.c"
    "src/font.c"
    "src/error.c"
    "src/image.c"
    "src/mesh.c"
    "src/object.c"
    "src/shader.c"
    "src/texture.c"
    "src/camera.c"
)

set(GLAD_SOURCES
    "deps/glad/src/glad.c"  "deps/glad/include/glad/glad.h"
                            "deps/glad/include/KHR/khrplatform.h"
    "deps/glad/src/glad_egl.c"
)

set(MATHC_SOURCES
    "deps/mathc/mathc.c"     "deps/mathc/mathc.h"
)

set(ALL_SOURCES
    ${MODEL_RENDERER_SOURCES}
    ${GLAD_SOURCES}
    ${MATHC_SOURCES}   
    "deps/obj/obj.c"
)

set(DEPS_INCLUDES
    "deps/glad/include"
    "deps/mathc"
    "deps/stb"
    "deps/obj"
)

add_executable(mr ${ALL_SOURCES})
target_link_libraries(mr PUBLIC glfw EGL m)
target_include_directories(mr PUBLIC ${DEPS_INCLUDES})