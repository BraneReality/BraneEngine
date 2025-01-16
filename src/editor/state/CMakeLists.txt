find_package(jsoncpp CONFIG REQUIRED)

add_library(state STATIC versioning.cpp)
target_link_libraries(state PUBLIC JsonCpp::JsonCpp utility)
