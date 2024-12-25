
# CMAKE_CURRENT_SOURCE_DIR doesn't work here because the script is moved
set(SOURCE ${CMAKE_SOURCE_DIR}/src/assetServer/database/data.db)
SET(DEST ${CMAKE_INSTALL_PREFIX}/database)

message("Install prefix: ${CMAKE_INSTALL_PREFIX}")
if(${SOURCE} IS_NEWER_THAN ${DEST}/data.db)
    message("resetting database")
    file(COPY ${SOURCE} DESTINATION ${DEST})
endif()
