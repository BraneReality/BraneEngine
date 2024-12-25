#src/client/graphics

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

get_target_property(LIB_PATH Vulkan::Vulkan IMPORTED_LOCATION)
message("VKLIBS: " ${LIB_PATH})

add_library(graphics STATIC
		graphics.cpp
		graphicsDevice.cpp
		"material.cpp"
		"mesh.cpp"
		renderer.cpp
		shader.cpp
		swapChain.cpp
		texture.cpp
		window.cpp
		renderTarget.cpp
		camera.cpp
		sceneRenderer.cpp
		graphicsBuffer.cpp)
target_compile_definitions(graphics PUBLIC GLFW_INCLUDE_VULKAN ${COMMON_DEFS})
target_link_libraries(graphics PUBLIC utility Vulkan::Vulkan glfw)
target_compile_definitions(graphics PRIVATE ICON_IMAGE_PATH="branelogo.png" CLIENT)
target_include_directories(graphics PUBLIC ${Vulkan_INCLUDE_DIRS})
