# Labyrinth 2.0a2 -- reconstructed source.
#
#   cmake --preset x64 && cmake --build --preset x64     -> build/Release (64-bit)
#   cmake --preset x86 && cmake --build --preset x86     -> build32/Release (32-bit)
#
# (run from src/, or pass --preset with -S src)
#
cmake_minimum_required(VERSION 3.16)
project(Labyrinth VERSION 2.0.0.2 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Labyrinth is a Windows program.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- the world compiler's parser ------------------------------------------
# wcparse.cpp is generated from wc.y; a pre-generated copy is kept in the
# tree so that Bison is only needed when the grammar changes.
find_program(BISON_EXECUTABLE NAMES bison win_bison
             PATHS ${CMAKE_SOURCE_DIR}/../tools/winflexbison)
set(WC_PARSER ${CMAKE_CURRENT_SOURCE_DIR}/wc/wcparse.cpp)
if(BISON_EXECUTABLE)
    add_custom_command(
        OUTPUT ${WC_PARSER}
        COMMAND ${BISON_EXECUTABLE} -y -o ${WC_PARSER} ${CMAKE_CURRENT_SOURCE_DIR}/wc/wc.y
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wc/wc.y
        COMMENT "Generating the world compiler's parser")
endif()

# ---- QvLib 1.0 --------------------------------------------------------------
file(GLOB QVLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qvlib/*.cpp)
add_library(qvlib STATIC ${QVLIB_SOURCES})
target_include_directories(qvlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qvlib)
target_compile_definitions(qvlib PRIVATE _CRT_SECURE_NO_WARNINGS PUBLIC _USE_MATH_DEFINES)
if(MSVC)
    target_compile_options(qvlib PRIVATE /W1 /permissive)
endif()

# ---- Reality Lab compatibility layer ----------------------------------------
add_library(rl STATIC rl/rlcore.cpp rl/rlrender.cpp)
target_include_directories(rl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rl)
target_compile_definitions(rl PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_link_libraries(rl PUBLIC opengl32 dwmapi)

# ---- Labyrinth ----------------------------------------------------------------
add_executable(labyrinth WIN32
    app/labmain.cpp
    app/labwin.cpp
    app/mainwin.cpp
    app/drawwin.cpp
    app/wcmodel.cpp
    app/labwww.cpp
    app/gif.cpp
    app/qvglue.cpp
    app/qvbuild.cpp
    app/inflate.cpp
    app/labimage.cpp
    wc/wclex.cpp
    wc/wcact.cpp
    ${WC_PARSER}
    res/labyrinth.rc)
set_target_properties(labyrinth PROPERTIES OUTPUT_NAME Labyrinth)
target_compile_definitions(labyrinth PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_link_libraries(labyrinth PRIVATE qvlib rl glu32 wininet shlwapi comdlg32 shell32 gdi32 user32
                      ole32 windowscodecs)
if(MSVC)
    target_compile_options(labyrinth PRIVATE /W3)
    # res/labyrinth.manifest is embedded by the resource script
    target_link_options(labyrinth PRIVATE /MANIFEST:NO)
    # one static runtime, no redistributable needed
    set_property(TARGET labyrinth qvlib rl PROPERTY
                 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
