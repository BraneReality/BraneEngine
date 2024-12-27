//
// Created by eli on 4/26/2022.
//

#ifndef BRANEENGINE_GUI_H
#define BRANEENGINE_GUI_H

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>
#include "common/runtime/module.h"
#include "guiEvent.h"
#include "vulkan/vulkan_core.h"
#include <unordered_map>

namespace graphics
{
    class VulkanRuntime;
}

class GUIRenderer;

class GUIWindow;

class GUIPopup;

struct ImFont;

class GUI : public Module
{
    GUIRenderer* _renderer;

    friend class GUIRenderer;

    VkDescriptorPool _imGuiDescriptorPool;

    std::vector<ImFont*> _fonts;

    std::vector<std::unique_ptr<GUIWindow>> _windows;
    std::unique_ptr<GUIPopup> _popup;

    void drawUI();

    std::function<void()> _drawMenu;
    std::mutex _queueLock;

    std::deque<std::unique_ptr<GUIEvent>> _queuedEvents;
    std::unordered_map<std::string, std::vector<GUIEventListener>> _eventListeners;

    void callEvents();

  public:
    GUI();

    ~GUI();

    static const char* name();

    void start() override;

    void stop() override;

    template<typename WindowT, typename... Args>
    WindowT* addWindow(Args&... args)
    {
        size_t index = _windows.size();
        _windows.push_back(std::make_unique<WindowT>(*this, args...));
        return static_cast<WindowT*>(_windows[index].get());
    }

    void clearWindows();

    void setMainMenuCallback(std::function<void()> drawMenu);

    void openPopup(std::unique_ptr<GUIPopup>&& popup);

    void closePopup();

    void sendEvent(std::unique_ptr<GUIEvent>&& name);

    template<typename T, typename... Args>
    void sendEvent(const Args&... args)
    {
        static_assert(std::is_base_of<GUIEvent, T>());
        sendEvent(std::make_unique<T>(args...));
    }

    template<typename T>
    void addEventListener(const std::string& name, GUIWindow* window, std::function<void(const T*)> callback)
    {
        static_assert(std::is_base_of<GUIEvent, T>());
        if constexpr(std::is_same<GUIEvent, T>())
            _eventListeners[name].push_back({window, callback});
        else
            _eventListeners[name].push_back({window, [callback](const GUIEvent* event) {
                                                 const T* e = (const T*)(event);
                                                 callback(e);
                                             }});
    }

    VkDescriptorPool descriptorPool();

    void setupImGui(graphics::VulkanRuntime& runtime);

    void cleanupImGui();

    inline const std::vector<ImFont*>& fonts() const { return _fonts; }

    void pushHeaderFont() const;

    void pushMonoFont() const;
};

#endif // BRANEENGINE_GUI_H
