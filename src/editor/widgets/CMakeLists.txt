set(CMAKE_FOLDER ${CMAKE_FOLDER}/Widgets)
add_library(editorWidgets STATIC
        assetBrowserWidget.cpp
        virtualVariableWidgets.cpp
        assetSelectWidget.cpp
        assetSearchWidget.cpp)
    target_link_libraries(editorWidgets PUBLIC ui editorWindows editorAssets)
target_compile_definitions(editorWidgets  PUBLIC CLIENT)
