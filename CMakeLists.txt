cmake_minimum_required(VERSION 3.1.0)

project(model_renderer C)
set(CMAKE_BUILD_TYPE Debug)

set(MODEL_RENDERER_SOURCES
    "src/main.c"
    "src/error.c"
    "src/mesh.c"
    "src/shader.c"
    "src/texture.c"
    "src/camera.c"
)

set(GLAD_SOURCES
    "deps/glad/src/glad.c"  "deps/glad/include/glad/glad.h"
                            "deps/glad/include/KHR/khrplatform.h"
)

set(ALL_SOURCES
    ${MODEL_RENDERER_SOURCES}
    ${GLAD_SOURCES}
    "deps/obj/obj.c"
)

set(DEPS_INCLUDES
    "deps/glad/include"
    "deps/stb"
    "deps/obj"
)

add_executable(mr ${ALL_SOURCES})
target_link_libraries(mr PUBLIC glfw m cglm)
target_include_directories(mr PUBLIC ${DEPS_INCLUDES})

add_subdirectory(deps/cglm/)