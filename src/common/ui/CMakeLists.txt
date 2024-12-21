find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

add_library(ui STATIC
        gui.cpp
        guiWindow.cpp
        guiEvent.cpp
        guiRenderer.cpp
        guiPopup.cpp)
target_link_libraries(ui PUBLIC imgui::imgui imguizmo::imguizmo runtime)
