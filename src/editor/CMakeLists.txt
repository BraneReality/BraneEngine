
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
        RUNTIME
        DESTINATION .
        COMPONENT Editor)

install(FILES config.json ${PROJECT_SOURCE_DIR}/media/branelogo.png
        DESTINATION .
        COMPONENT Editor)
install(DIRECTORY defaultAssets
        DESTINATION .
        COMPONENT Editor)
install(DIRECTORY fonts
        DESTINATION .
        COMPONENT Editor)
