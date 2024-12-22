# src/common/assets
add_subdirectory(types)

find_package(Vulkan REQUIRED)
find_package(OpenSSL REQUIRED)

set(ASSET_SOURCES
        assetID.cpp
        assetManager.cpp
        assetType.h
        assetType.cpp
        asset.cpp
        chunk.cpp
        assembly.cpp)

add_library(assets_server STATIC ${ASSET_SOURCES})

target_link_libraries(assets_server PUBLIC server_asset_types fileManager ecs systems OpenSSL::SSL)
target_compile_definitions(assets_server PUBLIC SERVER)
target_include_directories(assets_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(assets_server PUBLIC ${Vulkan_INCLUDE_DIRS})

add_library(assets_client STATIC ${ASSET_SOURCES})

target_link_libraries(assets_client PUBLIC client_asset_types fileManager ecs systems OpenSSL::SSL graphics)
target_compile_definitions(assets_client PUBLIC CLIENT)
target_include_directories(assets_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(assets_client PUBLIC ${Vulkan_INCLUDE_DIRS})
