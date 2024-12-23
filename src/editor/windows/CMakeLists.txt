
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)
    
set(CMAKE_FOLDER ${CMAKE_FOLDER}/Windows)
add_library(editorWindows STATIC
        dataWindow.cpp
        consoleWindow.cpp
        entitiesWindow.cpp
        renderWindow.cpp
        assetBrowserWindow.cpp
        memoryManagerWindow.cpp
        #createAssetWindow.cpp
        syncWindow.cpp
        selectProjectWindow.cpp editorWindow.cpp editorWindow.h)
target_link_libraries(editorWindows PUBLIC ui editorWidgets imgui::imgui imguizmo::imguizmo tinyfiledialogs::tinyfiledialogs)
target_compile_definitions(editorWindows PUBLIC CLIENT)
