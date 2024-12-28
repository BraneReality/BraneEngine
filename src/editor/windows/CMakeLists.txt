
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)
include(FindPkgConfig)
pkg_search_module(LUA luajit REQUIRED IMPORTED_TARGET)

message("Lua ${LUA_MODULE_NAME} ${LUA_CONFIG_EXECUTABLE}")
    
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
    target_link_libraries(editorWindows PUBLIC ui editorWidgets imgui::imgui imguizmo::imguizmo tinyfiledialogs::tinyfiledialogs PkgConfig::LUA)
target_compile_definitions(editorWindows PUBLIC CLIENT)
