# Main CMakeListsfor BraneEngine

cmake_minimum_required (VERSION 3.23)

project ("BraneEngine")
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# This is on for VIM lsp support  
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include util scripts
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_TESTS "Include testing build targets" OFF)
if(BUILD_TESTS)
    enable_testing()
endif()

# Library Complie Definitions
add_compile_definitions(
                GLM_ENABLE_EXPERIMENTAL
                GLM_FORCE_RADIANS
                GLM_FORCE_DEPTH_ZERO_TO_ONE
                ASIO_STANDALONE
                STATIC_CONCPP)


# These are (potentially not) used in the ECS target for struct padding
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    add_compile_definitions(_64BIT)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    add_compile_definitions(_32BIT)
endif()


if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /wd9002 /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    # For clangd support
    option(CLANGD_COMPATABILITY OFF "Add -std=c++20 to compile options so clangd can pick up on the c++ version. Causes warning spam from MSVC.")
    if(${CLANGD_COMPATABILITY})
        add_compile_options("-std=c++20")
    endif()
endif(MSVC)

# Sets an icon for the generated executables that will show up in file browsers
set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/media/brane.res)

# Some libraries have variants for different targets, and are compiled multiple times with different options
set(CLIENT_DEFS CLIENT)
set(RUNTIME_SERVER_DEFS RUNTIME_SERVER)
set(ASSET_SERVER_DEFS ASSET_SERVER)

add_subdirectory(src)


