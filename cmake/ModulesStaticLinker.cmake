if(DEFINED directory AND NOT DEFINED DIRECTORY)
    set(DIRECTORY "${directory}")
endif()

if(DEFINED target AND NOT DEFINED TARGET)
    set(TARGET "${target}")
endif()

if(NOT DEFINED DIRECTORY OR NOT DEFINED TARGET)
    message(FATAL_ERROR "Missing required execution arguments")
endif()

file(GLOB paths "${DIRECTORY}/*.mtex")
if(NOT paths)
    message(FATAL_ERROR "No files found")
endif()
list(SORT paths)

set(declarations "")
set(entries "")
set(index 0)

foreach(path ${paths})
    get_filename_component(name "${path}" NAME)

    file(READ "${path}" hex_content HEX)
    string(REGEX MATCHALL ".." hex_bytes_list "${hex_content}")
    set(hex_string "")
    foreach(byte ${hex_bytes_list})
        string(APPEND hex_string "0x${byte},")
    endforeach()

    string(APPEND declarations
            "static constexpr char bytes${index}[] = {${hex_string} 0x00};\n"
            "static constexpr std::string_view source${index}(bytes${index}, sizeof(bytes${index}) - 1);\n"
    )

    if(index EQUAL 0)
        set(entries "\"${name}\", source${index}")
    else()
        string(APPEND entries "\n\"${name}\", source${index}")
    endif()

    math(EXPR index "${index} + 1")
endforeach()

set(template [=[%{
#pragma once
#include <cstring>
#include <cstddef>
#include <optional>
#include <string_view>

struct Entry {
    const char* name;
    std::string_view source;
};

@DECLARATIONS@%}
struct Entry;
%%
@ENTRIES@
%%

namespace syntax::modules {

std::optional<std::string_view> find(std::string_view name) noexcept {
    const Entry* result = Modules::query(name.data(), static_cast<size_t>(name.size()));
    if (result != nullptr) {
        return result->source;
    }
    return std::nullopt;
}

}
]=])

string(REPLACE "@DECLARATIONS@" "${declarations}" content "${template}")
string(REPLACE "@ENTRIES@" "${entries}" content "${content}")

file(WRITE "${TARGET}" "${content}")