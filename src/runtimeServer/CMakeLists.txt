# src/runtimeServer
set(CMAKE_FOLDER ${CMAKE_FOLDER}/RuntimeServer)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_executable(RuntimeServer main.cpp
        "runtimeServer.cpp"
        )
target_compile_definitions(RuntimeServer PRIVATE ${COMMON_DEFS})
target_link_libraries(RuntimeServer PRIVATE ecs networking config)

configure_file(config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)
set(INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/runtimeServer)

install(TARGETS RuntimeServer
        RUNTIME
        DESTINATION ${INSTALL_DIR}
        COMPONENT RuntimeServer)
install(FILES config.json
        DESTINATION ${INSTALL_DIR}
        COMPONENT RuntimeServer)
