# src/common/utlity
find_package(tinyfiledialogs CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

set(SOURCES
		clock.cpp
		threadPool.cpp
		sharedRecursiveMutex.cpp
		serializedData.cpp
        jsonSerializer.cpp
        uuid.cpp)

add_library(utility STATIC ${SOURCES} jsonTypeUtilities.h)

target_link_libraries(utility PUBLIC runtime JsonCpp::JsonCpp tinyfiledialogs::tinyfiledialogs OpenSSL::Crypto)

if(BUILD_TESTS)
 add_library(utility_test STATIC ${SOURCES})
target_compile_definitions(utility_test PUBLIC TEST_BUILD)
target_link_libraries(utility_test PUBLIC runtime JsonCpp::JsonCpp)
endif()
