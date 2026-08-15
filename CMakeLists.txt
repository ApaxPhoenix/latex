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

set(GPERF_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/unicodes.gperf")
set(GPERF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lookup.hpp")

add_custom_command(
        OUTPUT ${GPERF_OUTPUT}
        COMMAND ${GPERF_EXECUTABLE} -t -C -N query -Z Lookup -L C++ "${GPERF_INPUT}" --output-file=${GPERF_OUTPUT}
        DEPENDS "${GPERF_INPUT}"
        VERBATIM
)

add_custom_target(generate_lookup DEPENDS ${GPERF_OUTPUT})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_executable(latex ${SOURCES} ${HEADERS} ${GPERF_OUTPUT})
add_dependencies(latex generate_lookup)

target_include_directories(latex PUBLIC include src "${CMAKE_CURRENT_BINARY_DIR}")

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