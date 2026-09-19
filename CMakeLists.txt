cmake_minimum_required(VERSION 3.22)
project(SymoCraft)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(MemoryAllocator include/MemoryAllocator/AmoBase.cpp)

set(Core src/core/global_thread_pool.cpp
        src/core/window.cpp
        src/core/application.cpp)

set(ECS src/core/ECS/internal.cpp
        src/core/ECS/registry.cpp
        src/core/ECS/Systems/transform_system.cpp
        src/core/ECS/Systems/physics_system.cpp
        src/core/ECS/Systems/character_system.cpp
        )

file(GLOB SOURCE_FILE
        "vendor/yaml-cpp/src/*.cpp"
        "vendor/yaml-cpp/src/*.h"
        "include/renderer/batch.hpp"
        "src/renderer/*.cpp"
        "src/world/*.cpp"
        "src/camera/camera.cpp"
        "src/input/*.cpp"
        "src/playercontroller/*.cpp"
        "src/event/*.cpp")


link_directories(${PROJECT_SOURCE_DIR}/lib/irrKlang)

include_directories(vendor include vendor/glfw/include/)

add_executable(SymoCraft src/main.cpp
        vendor/glad/glad.c
        ${SOURCE_FILE}
        ${MemoryAllocator}
        ${Core}
        ${ECS})


# Copying of all necessary dll files on which the executable depends
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/lib/irrKlang"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

add_subdirectory(vendor/glm)
add_subdirectory(vendor/glfw)

target_link_libraries(${PROJECT_NAME}
        glfw
        irrKlang)

