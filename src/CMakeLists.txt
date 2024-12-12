#file: src

set(CMAKE_FOLDER BraneEngine)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/common
	../include
)

# run child cmake lists
add_subdirectory(common)

# create executables
add_subdirectory(client)
add_subdirectory(editor)
# Not Implemented, buggly half placeholder for now
#add_subdirectory(runtimeServer)
add_subdirectory(assetServer)

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()
