# src/client

set(CMAKE_FOLDER ${CMAKE_FOLDER}/Client)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_executable(BraneSurfer main.cpp client.cpp)
if(UNIX)
set_target_properties(BraneSurfer PROPERTIES
		OUTPUT_NAME "BraneSurfer"
		SUFFIX ".out")
endif()
target_include_directories(BraneSurfer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(BraneSurfer PRIVATE ${COMMON_DEFS} ${CLIENT_DEFS})
target_link_libraries(BraneSurfer PRIVATE runtime ecs networking config graphics assets_client chunks)


install(TARGETS BraneSurfer
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
		COMPONENT Client
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:BraneSurfer> DESTINATION . OPTIONAL COMPONENT Client)
endif()
install(FILES 
        config.json 
        ${CMAKE_SOURCE_DIR}/media/branelogo.png
        DESTINATION .
		COMPONENT Client)

