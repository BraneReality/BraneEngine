#src/common/config
 find_package(jsoncpp CONFIG REQUIRED)

add_library(config STATIC config.cpp)
target_link_libraries(config PUBLIC JsonCpp::JsonCpp)
