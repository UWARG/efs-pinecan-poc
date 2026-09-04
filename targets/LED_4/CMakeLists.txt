cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# ======== Pre-build setup ========
# Default config to use from BuildConfigs.json. This can be overridden from command line.
# IMPORATANT: If you modify this value (or any CACHE value), you must do a clean re-configure of CMake
set(PINECAN_CONFIG_KEY "LED_4"
    CACHE STRING "Key in BuildConfigs.json to use")

# Repo root path. Adjust path to match your layout.
get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Load BuildConfig.json to get config parameters
set(PINECAN_CONFIG_FILE "${ROOT_DIR}/BuildConfigs.json")
if(NOT EXISTS "${PINECAN_CONFIG_FILE}")
    message(FATAL_ERROR "Config file not found: ${PINECAN_CONFIG_FILE}")
endif()

file(READ "${PINECAN_CONFIG_FILE}" _configs_json)

# Ensure the requested key exists
string(JSON _has_key ERROR_VARIABLE _json_err
       GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")

if(_json_err)
    message(FATAL_ERROR
        "Config key '${PINECAN_CONFIG_KEY}' not found in BuildConfigs.json")
endif()

string(JSON _cfg GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")

# Extract board
string(JSON PINECAN_BOARD GET "${_cfg}" board)

# Extract toolchain type
string(JSON TOOLCHAIN GET "${_cfg}" toolchain)

# Get all keys under "symbols"
# Extract the 'symbols' object only
string(JSON _symbols_obj GET "${_cfg}" symbols)

# How many members does 'symbols' have?
string(JSON _sym_count LENGTH "${_symbols_obj}")
math(EXPR _sym_last "${_sym_count} - 1")

set(PINECAN_SYMBOL_DEFS "")

# Iterate members by index and get their names with MEMBER
foreach(_i RANGE 0 ${_sym_last})
    # Get the name of the _i-th member key
    string(JSON _key MEMBER "${_symbols_obj}" ${_i})

    # Value under that key
    string(JSON _val  GET  "${_symbols_obj}" "${_key}")
    string(JSON _type TYPE "${_symbols_obj}" "${_key}")

    # Decide quoting based on JSON type
    if(_type STREQUAL "STRING")
        # Strings need quotes in C preprocessor
        list(APPEND PINECAN_SYMBOL_DEFS "${_key}=\"${_val}\"")
    else()
        # NUMBER / BOOLEAN / NULL / OBJECT / ARRAY -> no quotes
        list(APPEND PINECAN_SYMBOL_DEFS "${_key}=${_val}")
    endif()
endforeach()

message("Using BuildConfig: ${PINECAN_CONFIG_KEY}")
message("  board: ${PINECAN_BOARD}")
message("  symbols: ${PINECAN_SYMBOL_DEFS}")

string(JSON FLASH_OFFSET GET "${_symbols_obj}" FLASH_OFFSET)

# ======= Build setup ========
# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME LED_4)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating ${CMAKE_PROJECT_NAME}.bin"
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USER_VECT_TAB_ADDRESS
    VECT_TAB_OFFSET=${FLASH_OFFSET}
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    "-T${ROOT_DIR}/pinecan/boards/${PINECAN_BOARD}/STM32L433XX_FLASH.ld"
    "-Wl,--defsym=FLASH_OFFSET=${FLASH_OFFSET}"
)


############# NEED TO ADD THE LOCAITON OF THE LINKER FOR ALL PROJECTS OF THIS BOARD ################

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
