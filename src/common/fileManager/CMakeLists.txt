# src/common/fileManager
find_package(tinyfiledialogs CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

add_library(fileManager STATIC fileManager.cpp fileWatcher.cpp)

target_link_libraries(fileManager PUBLIC tinyfiledialogs::tinyfiledialogs JsonCpp::JsonCpp)
