add_library(editorState STATIC 
    versioning.cpp 
    trackedValue.cpp 
    trackedObject.cpp)
target_link_libraries(editorState PUBLIC utility)
