find_package(jsoncpp CONFIG REQUIRED)

set(CMAKE_FOLDER ${CMAKE_FOLDER}/Assets)
add_library(editorAssets STATIC
        gltfLoader.cpp
        assemblyBuilder.cpp
        editorAsset.cpp
        assemblyReloadManager.cpp 
        assetCache.cpp 
        jsonVirtualType.cpp)
target_link_libraries(editorAssets PUBLIC assets_client ecs JsonCpp::JsonCpp)
