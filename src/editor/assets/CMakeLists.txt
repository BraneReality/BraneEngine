find_package(efsw CONFIG REQUIRED)

set(CMAKE_FOLDER ${CMAKE_FOLDER}/Assets)
add_library(editorAssets STATIC 
    gltfLoader.cpp 
    assemblyBuilder.cpp 
    editorAsset.cpp 
    assemblyReloadManager.cpp 
    assetCache.cpp 
    assetIndexer.cpp 
    jsonVirtualType.cpp 
    sources/assetSource.cpp
    sources/imageSource.cpp)
target_link_libraries(editorAssets PUBLIC assets_client ecs utility editorState efsw::efsw)
