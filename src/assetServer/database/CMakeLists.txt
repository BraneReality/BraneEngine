
find_package(OpenSSL REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

set(CMAKE_FOLDER ${CMAKE_FOLDER}/Database)
add_library(database STATIC database.cpp database.h databaseAsset.cpp databaseAsset.h)

target_link_libraries(database PRIVATE unofficial::sqlite3::sqlite3 OpenSSL::SSL)
