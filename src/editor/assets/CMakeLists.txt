find_package(jsoncpp CONFIG REQUIRED)

add_library(editorAssets STATIC
        gltfLoader.cpp
        assemblyBuilder.cpp
        editorAsset.cpp
        types/editorShaderAsset.cpp
        types/editorAssemblyAsset.cpp
        jsonVirtualType.cpp
        assetCache.cpp
        types/editorMaterialAsset.cpp
        types/editorChunkAsset.cpp
        assemblyReloadManager.cpp
        types/editorImageAsset.cpp)
target_link_libraries(editorAssets PUBLIC assets_client ecs JsonCpp::JsonCpp)
