
SET(CMAKE_FOLDER ${CMAKE_FOLDER}/Editor)

add_subdirectory(widgets)
add_subdirectory(windows)
add_subdirectory(assets)
add_subdirectory(graphics)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_executable(BraneEditor
        ${PROJECT_SOURCE_DIR}/media/brane.rc
        main.cpp
        editor.cpp
        editorEvents.cpp
        braneProject.cpp)
if(WIN32)
    set_target_properties(BraneEditor PROPERTIES WIN32_EXECUTABLE $<IF:$<CONFIG:Release>,TRUE,FALSE>)
endif()
target_include_directories(BraneEditor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )
target_compile_definitions(BraneEditor PRIVATE ${COMMON_DEFS} ${CLIENT_DEFS})
target_link_libraries(BraneEditor PUBLIC ecs networking config graphics assets_client runtime editorWindows systems fileManager editorAssets shaderCompiler)

install(TARGETS BraneEditor
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
        COMPONENT Editor)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:BraneEditor> DESTINATION . OPTIONAL COMPONENT Editor)
endif()
install(FILES config.json ${PROJECT_SOURCE_DIR}/media/branelogo.png
        DESTINATION .
        COMPONENT Editor)
install(DIRECTORY defaultAssets
        DESTINATION .
        COMPONENT Editor)
install(DIRECTORY fonts
        DESTINATION .
        COMPONENT Editor)
