find_package(jsoncpp CONFIG REQUIRED)

set(CMAKE_FOLDER ${CMAKE_FOLDER}/Assets)
add_library(editorAssets STATIC
        gltfLoader.cpp
        assemblyBuilder.cpp
        editorAsset.cpp
        types/editorShaderAsset.cpp
        types/editorAssemblyAsset.cpp
        jsonVirtualType.cpp assetCache.cpp assetCache.h types/editorMaterialAsset.cpp types/editorMaterialAsset.h types/editorChunkAsset.cpp types/editorChunkAsset.h assemblyReloadManager.cpp assemblyReloadManager.h types/editorImageAsset.cpp types/editorImageAsset.h)
target_link_libraries(editorAssets PUBLIC assets_client ecs JsonCpp::JsonCpp)
