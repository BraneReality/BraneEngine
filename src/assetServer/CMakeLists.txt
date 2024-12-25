#src/assetServer

set(CMAKE_FOLDER ${CMAKE_FOLDER}/AssetServer)
add_subdirectory(database)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_executable(AssetServer main.cpp assetServer.cpp assetServer.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AssetServer PRIVATE runtime config networking ecs utility fileManager database assets_server)
target_compile_definitions(AssetServer PRIVATE ${COMMON_DEFS} ${ASSET_SERVER_DEFS} ASIO_STANDALONE)

install(TARGETS AssetServer
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES 
            "api-ms-.*" "ext-ms-.*"  # Windows system DLLs
            "^/lib.*"                # Linux system libraries
            "^/usr/lib.*"
        POST_EXCLUDE_REGEXES 
            ".*system32/.*\\.dll"    # Windows system directory
            "^/lib64/.*"            # Linux system directories
            "^/usr/lib64/.*"
        DESTINATION .
        COMPONENT AssetServer)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:AssetServer> DESTINATION . OPTIONAL COMPONENT AssetServer)
endif()
install(FILES config.json
        DESTINATION .
        COMPONENT AssetServer)
install(CODE "   
    set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/database/data.db)
    SET(DEST \${CMAKE_INSTALL_PREFIX}/database)

    if(\${SOURCE} IS_NEWER_THAN \${DEST}/data.db)
        message(\"resetting database\")
        file(MAKE_DIRECTORY \${DEST})
        file(COPY \${SOURCE} DESTINATION \${DEST})
    endif()" 
    COMPONENT AssetServer)
