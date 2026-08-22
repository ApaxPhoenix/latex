cmake_minimum_required(VERSION 3.24)
project(latex C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(SK_SUPPORT_PDF=1 SK_ENABLE_SKPDF=1)

find_program(GPERF_EXECUTABLE gperf REQUIRED)

find_package(unofficial-skia CONFIG REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(EXPAT REQUIRED)

set(LOOKUP_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/unicodes.gperf")
set(LOOKUP_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lookup.hpp")

add_custom_command(
        OUTPUT ${LOOKUP_OUTPUT}
        COMMAND ${GPERF_EXECUTABLE} -t -C -N query -Z Lookup -L C++ ${LOOKUP_INPUT} --output-file=${LOOKUP_OUTPUT}
        DEPENDS ${LOOKUP_INPUT}
        VERBATIM
)

add_custom_target(lookup DEPENDS ${LOOKUP_OUTPUT})

file(GLOB_RECURSE PROJECT_SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PROJECT_HEADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

file(GLOB_RECURSE TREE_DIRECTORY_LIST LIST_DIRECTORIES true CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*"
)

set(PROJECT_INCLUDE_PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

foreach(DIRECTORY_ENTRY ${TREE_DIRECTORY_LIST})
    if(IS_DIRECTORY ${DIRECTORY_ENTRY})
        list(APPEND PROJECT_INCLUDE_PATHS ${DIRECTORY_ENTRY})
    endif()
endforeach()

add_executable(latex ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES} ${LOOKUP_OUTPUT})
add_dependencies(latex lookup)

target_include_directories(latex PUBLIC ${PROJECT_INCLUDE_PATHS})

target_link_libraries(latex PRIVATE
        unofficial::skia::skia
        Fontconfig::Fontconfig
        Freetype::Freetype
        harfbuzz::harfbuzz
        EXPAT::EXPAT
        dwrite
        gdi32
        user32
        opengl32
        usp10
)