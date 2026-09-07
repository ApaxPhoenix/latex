file(GLOB_RECURSE tests CONFIGURE_DEPENDS "*.cpp")

foreach(test ${tests})
    get_filename_component(name ${test} NAME_WE)

    add_executable(${name} ${test} ${engine} ${headers} ${output} ${header})
    add_dependencies(${name} lookup modules)
    target_include_directories(${name} PUBLIC ${includes})

    target_link_libraries(${name} PRIVATE
            unofficial::skia::skia
            Fontconfig::Fontconfig
            Freetype::Freetype
            EXPAT::EXPAT
    )

    if(TARGET harfbuzz::harfbuzz)
        target_link_libraries(${name} PRIVATE harfbuzz::harfbuzz)
    else()
        target_link_libraries(${name} PRIVATE ${HARFBUZZ_LIBRARIES})
        target_include_directories(${name} PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
    endif()

    if(WIN32)
        target_link_libraries(${name} PRIVATE
                dwrite
                gdi32
                user32
                opengl32
                usp10
        )
    endif()

    add_test(NAME ${name} COMMAND ${name})
endforeach()