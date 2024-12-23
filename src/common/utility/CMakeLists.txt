# src/common/utlity

find_package(jsoncpp CONFIG REQUIRED)

set(SOURCES
		clock.cpp
		threadPool.cpp
		sharedRecursiveMutex.cpp
		serializedData.cpp
		jsonVersioner.cpp enumNameMap.h)

add_library(utility STATIC ${SOURCES} jsonTypeUtilities.h)

target_link_libraries(utility PUBLIC runtime JsonCpp::JsonCpp)

if(BUILD_TESTS)
 add_library(utility_test STATIC ${SOURCES})
target_compile_definitions(utility_test PUBLIC TEST_BUILD)
target_link_libraries(utility_test PUBLIC runtime JsonCpp::JsonCpp)
endif()
