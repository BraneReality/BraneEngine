add_library(external_include_dirs INTERFACE)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
if(BUILD_TESTS)
    add_subdirectory(gtest)
endif()
add_subdirectory(SQLite)

add_subdirectory(jsoncpp)

target_include_directories(external_include_dirs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/gtest/googletest/include
        ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/include
        ${CMAKE_CURRENT_SOURCE_DIR})
