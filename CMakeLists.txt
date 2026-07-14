cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(latex CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(APPLE)
    link_directories(/opt/homebrew/lib)
    include_directories(/opt/homebrew/include)
endif()

find_path(CAIRO_INCLUDE_DIR NAMES cairo.h PATH_SUFFIXES cairo REQUIRED)
find_library(CAIRO_LIBRARY NAMES cairo REQUIRED)

find_path(FONTCONFIG_INCLUDE_DIR NAMES fontconfig/fontconfig.h REQUIRED)
find_library(FONTCONFIG_LIBRARY NAMES fontconfig REQUIRED)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
        core
        support
        native
        mc
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_executable(latex ${SOURCES} ${HEADERS})

set_target_properties(latex PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
        OUTPUT_NAME "latex"
)

target_include_directories(latex PUBLIC
        include
        src
        ${LLVM_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIR}
        ${FONTCONFIG_INCLUDE_DIR}
)

if(MSVC)
    target_link_libraries(latex PRIVATE
            ${LLVM_LIBS}
            ${CAIRO_LIBRARY}
            ${FONTCONFIG_LIBRARY}
    )
else()
    target_link_libraries(latex PRIVATE
            ${LLVM_LIBS}
            ${CAIRO_LIBRARY}
            ${FONTCONFIG_LIBRARY}
            zstd
            m
            dl
            pthread
    )
endif()