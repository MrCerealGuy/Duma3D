cmake_minimum_required(VERSION 3.20)

project(Duma3D
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Duma3D
# --------------------------------------------------

add_executable(Duma3D
    src/main.cpp
    src/Engine/Application.cpp
    src/Engine/Graphics/Camera.cpp
    src/Engine/Graphics/Renderer.cpp
    src/Engine/Math/Math.cpp
    src/Engine/Scene/Mesh.cpp
    src/Engine/Scene/Scene.cpp
)

# --------------------------------------------------
# Include directories
# --------------------------------------------------

target_include_directories(Duma3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --------------------------------------------------
# Windows / MinGW
# --------------------------------------------------

if(WIN32)
    target_link_options(Duma3D PRIVATE
        -mconsole
    )

    if(MINGW)
        target_link_options(Duma3D PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
        )
    endif()
endif()

# --------------------------------------------------
# OpenGL / Windows libraries
# --------------------------------------------------

if(WIN32)
    target_link_libraries(Duma3D PRIVATE
        opengl32
        gdi32
        user32
        shell32
    )
endif()

# --------------------------------------------------
# Assets
# --------------------------------------------------

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET Duma3D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Duma3D>/assets"
    )
endif()

# --------------------------------------------------
# Compiler warnings
# --------------------------------------------------

if(MSVC)
    target_compile_options(Duma3D PRIVATE
        /W4
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Duma3D PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
