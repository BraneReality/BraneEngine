//
// Created by eli on 4/26/2022.
//

#include "gui.h"
#include "graphics/graphics.h"
#include "graphics/graphicsBuffer.h"
#include "graphics/graphicsDevice.h"
#include "graphics/swapChain.h"
#include "graphics/window.h"

#include "guiPopup.h"
#include "guiRenderer.h"
#include "guiWindow.h"

#include "common/runtime/runtime.h"

#include "IconsFontAwesome6.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

GUI::GUI()
{
    Runtime::timeline().addBlockBefore("gui", "draw");
    Runtime::timeline().addTask(
        "updateUI",
        [&] {
        _windows.erase(std::remove_if(
                           _windows.begin(),
                           _windows.end(),
                           [this](auto& window) {
            if(window->isOpen())
                return false;
            // Remove listeners first
            auto la = _eventListeners.begin();
            while(la != _eventListeners.end())
            {
                // Remove listeners tied to windows
                la->second.erase(std::remove_if(la->second.begin(),
                                                la->second.end(),
                                                [&window](auto& l) { return l.window == window.get(); }),
                                 la->second.end());
                if(la->second.empty())
                    la = _eventListeners.erase(la);
                else
                    ++la;
            }
            return true;
                           }),
                       _windows.end());
        for(auto& w : _windows)
        {
            w->update();
        }
        },
        "gui");
}

const char* GUI::name() { return "editorUI"; }

void GUI::drawUI()
{
    if(_drawMenu)
        _drawMenu();

    ImGui::SetNextWindowPos(ImGui::GetCurrentContext()->CurrentViewport->WorkPos);
    ImGui::SetNextWindowSize(ImGui::GetCurrentContext()->CurrentViewport->WorkSize);
    ImGui::SetNextWindowViewport(ImGui::GetCurrentContext()->CurrentViewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    if(ImGui::Begin("RootWindow",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                        ImGuiWindowFlags_NoCollapse))
    {
        ImGui::PopStyleVar(3);
        ImGui::DockSpace(ImGui::GetID("DockingRoot"), {0, 0}, ImGuiDockNodeFlags_None);

        if(_popup)
            _popup->draw();

        // Events are called in here to allow for docking to be influenced
        callEvents();
    }
    ImGui::End();

    for(auto& w : _windows)
    {
        w->draw();
    }
}

void GUI::sendEvent(std::unique_ptr<GUIEvent>&& event)
{
    _queueLock.lock();
    _queuedEvents.push_back(std::move(event));
    _queueLock.unlock();
}

void GUI::callEvents()
{
    // Avoid mutex locks by moving event queue into local queue
    _queueLock.lock();
    std::deque events = std::move(_queuedEvents);
    assert(_queuedEvents.empty());
    _queueLock.unlock();

    for(auto& event : events)
    {
        if(_eventListeners.count(event->name()))
        {
            for(auto& listener : _eventListeners.at(event->name()))
            {
                listener.callback(event.get());
            }
        }
    }
}

GUI::~GUI() {}

void GUI::setupImGui(graphics::VulkanRuntime& runtime)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable docking
#ifdef _WIN32
    // io.ConfigFlags   |= ImGuiConfigFlags_ViewportsEnable;
#endif
    io.IniFilename = NULL;
    _fonts.push_back(io.Fonts->AddFontFromFileTTF("fonts/default.ttf", 15.0f));

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f;
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromFileTTF("fonts/" FONT_ICON_FILE_NAME_FAS, 11.0f, &config, icon_ranges);

    _fonts.push_back(io.Fonts->AddFontFromFileTTF("fonts/header.ttf", 17.0f));
    _fonts.push_back(io.Fonts->AddFontFromFileTTF("fonts/monospace.ttf", 13.0f));

    // Setup ImGui style TODO move this to config.json
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.50f, 0.43f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.48f, 0.29f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.98f, 0.59f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.98f, 0.59f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.48f, 0.29f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.98f, 0.59f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.88f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.98f, 0.59f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.98f, 0.59f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.98f, 0.59f, 0.70f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.98f, 0.53f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.98f, 0.59f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.98f, 0.59f, 0.40f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.98f, 0.59f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.75f, 0.40f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.75f, 0.40f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.98f, 0.59f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.98f, 0.59f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.98f, 0.59f, 0.95f);
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_DockingPreview].w = 0.75f;
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.20f, 0.19f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.35f, 0.31f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.25f, 0.23f, 1.00f);  // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.98f, 0.59f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.2f, 1.00f, 0.2f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.98f, 0.59f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    // Find queue families
    auto families = graphics::device->queueFamilyIndices();

    // Create a descriptor pool
    {
        VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        if(vkCreateDescriptorPool(graphics::device->get(), &pool_info, nullptr, &_imGuiDescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create Dear ImGui's descriptor pool");
        }
    }

    // Init ImGui integrations
    ImGui_ImplGlfw_InitForVulkan(runtime.window()->window(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = runtime.instance();
    init_info.PhysicalDevice = graphics::device->physicalDevice();
    init_info.Device = graphics::device->get();
    init_info.QueueFamily = *families.graphicsFamily;
    init_info.Queue = graphics::device->graphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = _imGuiDescriptorPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = runtime.swapChain()->size();
    init_info.ImageCount = runtime.swapChain()->size();
    init_info.CheckVkResultFn = [](VkResult r) {
        if(r != VK_SUCCESS)
            Runtime::error("ImGui vulkan returned " + std::to_string(r));
    };
    init_info.RenderPass = _renderer->renderPass();
    ImGui_ImplVulkan_Init(&init_info);

    // Upload Fonts
    // graphics::SingleUseCommandBuffer commandBuffer(graphics::device->graphicsPool());
    // ImGui_ImplVulkan_CreateFontsTexture(commandBuffer.get());
    // commandBuffer.submit(graphics::device->graphicsQueue());
}

void GUI::cleanupImGui()
{
    vkDeviceWaitIdle(graphics::device->get());
    vkDestroyDescriptorPool(graphics::device->get(), _imGuiDescriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

VkDescriptorPool GUI::descriptorPool() { return _imGuiDescriptorPool; }

void GUI::stop()
{
    _windows.resize(0);
    cleanupImGui();
}

void GUI::setMainMenuCallback(std::function<void()> drawMenu) { _drawMenu = drawMenu; }

void GUI::start()
{
    auto* vkr = Runtime::getModule<graphics::VulkanRuntime>();

    _renderer = vkr->createRenderer<GUIRenderer>(this);
    _renderer->setTargetAsSwapChain(false);

    setupImGui(*vkr);
}

void GUI::openPopup(std::unique_ptr<GUIPopup>&& popup) { _popup = std::move(popup); }

void GUI::closePopup() { _popup = nullptr; }

void GUI::clearWindows()
{
    _windows.clear();
    auto la = _eventListeners.begin();
    while(la != _eventListeners.end())
    {
        // Remove listeners tied to windows
        la->second.erase(std::remove_if(la->second.begin(), la->second.end(), [](auto& la) { return la.window; }),
                         la->second.end());
        if(la->second.empty())
            la = _eventListeners.erase(la);
        else
            ++la;
    }
}

void GUI::pushHeaderFont() const { ImGui::PushFont(_fonts[1]); }

void GUI::pushMonoFont() const { ImGui::PushFont(_fonts[2]); }
