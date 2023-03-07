cmake_minimum_required(VERSION 3.1.0)

project(model_renderer C)

set(MODEL_RENDERER_SOURCES
    "src/main.c"
    "src/error.c"   "src/error.h"
    "src/mesh.c"    "src/mesh.h"
    "src/shader.c"  "src/shader.h"
    "src/texture.c" "src/texture.h"
    "src/camera.c"  "src/camera.h"
                    "src/getopt.h"
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
    "deps/glfw/include"
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

add_subdirectory(deps/glfw/)
add_subdirectory(deps/cglm/)

add_executable(mr ${ALL_SOURCES})
target_link_libraries(mr  PUBLIC glfw m cglm)
target_include_directories(mr PUBLIC ${DEPS_INCLUDES})
