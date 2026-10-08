cmake_minimum_required(VERSION 3.20)

project(Duma3D
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reusable engine code. Demo scenes and their application entry points live
# outside this target so other demos can link the same engine library.
add_library(Duma3DEngine STATIC
    src/Engine/Assets/AssetPath.cpp
    src/Engine/Graphics/Camera.cpp
    src/Engine/Graphics/Renderer.cpp
    src/Engine/Math/Math.cpp
    src/Engine/Scene/Mesh.cpp
    src/Engine/Scene/Scene.cpp
)

target_include_directories(Duma3DEngine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(Duma3D
    src/Demos/Demo_1/main.cpp
    src/Demos/Demo_1/Application.cpp
    src/Demos/Demo_1/DemoScene.cpp
)

target_include_directories(Duma3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Demos/Demo_1
)
target_link_libraries(Duma3D PRIVATE Duma3DEngine)

if(WIN32)
    target_link_libraries(Duma3DEngine PUBLIC
        opengl32
        gdi32
        user32
        shell32
        ole32
        windowscodecs
    )

    target_link_options(Duma3D PRIVATE -mconsole)
    if(MINGW)
        target_link_options(Duma3D PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    file(GLOB_RECURSE DUMA3D_ASSET_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/*"
    )
    set_property(TARGET Duma3D APPEND PROPERTY LINK_DEPENDS ${DUMA3D_ASSET_FILES})

    add_custom_command(TARGET Duma3D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Duma3D>/assets"
    )
endif()

if(MSVC)
    target_compile_options(Duma3DEngine PRIVATE /W4)
    target_compile_options(Duma3D PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Duma3DEngine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Duma3D PRIVATE -Wall -Wextra -Wpedantic)
endif()
