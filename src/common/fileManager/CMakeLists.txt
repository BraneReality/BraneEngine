# src/common/fileManager
find_package(tinyfiledialogs CONFIG REQUIRED)

add_library(fileManager STATIC fileManager.cpp)

target_link_libraries(fileManager PUBLIC config tinyfiledialogs::tinyfiledialogs)
