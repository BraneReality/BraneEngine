#pragma once

#include "validationLayers.h"

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <runtime/module.h>
#include <utility/asyncQueue.h>
#include <utility/staticIndexVector.h>
#include <vulkan/vulkan.hpp>

class EntityManager;

class Asset;

class MeshAsset;

class ShaderAsset;

class MaterialAsset;

namespace graphics {
    class Window;

    class GraphicsDevice;

    class SwapChain;

    class ShaderManager;

    class Material;

    class Renderer;

    class Texture;

    class Mesh;

    class Shader;

    class VulkanRuntime : public Module {
        Window* _window;
        VkInstance _instance;
        GraphicsDevice* _device;
        SwapChain* _swapChain;
        std::vector<VkCommandBuffer> _drawBuffers;

        std::recursive_mutex _assetLock;
        std::deque<std::pair<uint32_t, Asset*>> _newAssets;
        std::deque<Asset*> _reloadAssets;
        staticIndexVector<std::unique_ptr<Shader>> _shaders;
        staticIndexVector<std::unique_ptr<Material>> _materials;
        staticIndexVector<std::unique_ptr<Texture>> _textures;
        staticIndexVector<std::unique_ptr<Mesh>> _meshes;

        std::vector<std::unique_ptr<Renderer>> _renderers;

        std::vector<VkSemaphore> _renderFinishedSemaphores;

        bool _validationLayersEnabled = false;
        VkDebugUtilsMessengerEXT _debugMessenger;

        std::vector<const char*> requiredExtensions();

        const int MAX_FRAMES_IN_FLIGHT = 2;
        std::vector<VkFence> _inFlightFences;
        std::vector<VkFence> _imagesInFlight;

        std::function<bool()> _onWindowClosed;

        void init();

        void cleanup();

        void createInstance();

        void createSyncObjects();

        void createDrawBuffers();

        bool validationLayersSupported();

        void setDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        void setupDebugMessenger();

        static VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger);

        static void DestroyDebugUtilsMessengerEXT(
            VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

        void processAsset(uint32_t runtimeID, Asset* graphicalAsset);

      public:
        VulkanRuntime();

        ~VulkanRuntime();

        static const char* name();

        VkInstance instance();

        VkDevice device();

        VkPhysicalDevice physicalDevice();

        Window* window();

        SwapChain* swapChain();

        void updateWindow();

        void onWindowClosed(std::function<bool()> callback);

        // Behold, the only function we care about:
        void draw(EntityManager& em);
        // Bask in its glory!

        // These methods must be called from the main thread
        size_t addTexture(Texture* texture);

        size_t addShader(ShaderAsset* shader);

        size_t addMaterial(MaterialAsset* material);

        size_t addMesh(MeshAsset* mesh);

        void reloadShader(ShaderAsset* shader);

        void reloadMaterial(MaterialAsset* material);

        // Add asset may be called from any thread
        uint32_t addAsset(Asset* graphicalAsset);

        void reloadAsset(Asset* graphicalAsset, bool async = true);

        const staticIndexVector<std::unique_ptr<Material>>& materials();

        Shader* getShader(size_t runtimeID);

        Texture* getTexture(size_t runtimeID);

        template <typename T, typename... Args> T* createRenderer(Args... args)
        {
            static_assert(std::is_base_of<Renderer, T>());
            _renderers.push_back(std::make_unique<T>(*_swapChain, args...));
            return (T*)_renderers[_renderers.size() - 1].get();
        }

        void removeRenderer(Renderer* renderer);

        Material* getMaterial(size_t id);

        const staticIndexVector<std::unique_ptr<Mesh>>& meshes();
    };

} // namespace graphics
