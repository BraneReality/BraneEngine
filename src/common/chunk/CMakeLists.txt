add_library(chunks STATIC
        chunkLoader.cpp)
target_link_libraries(chunks PUBLIC ecs runtime utility)
