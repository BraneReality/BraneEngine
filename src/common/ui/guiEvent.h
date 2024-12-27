//
// Created by eli on 5/22/2022.
//

#ifndef BRANEENGINE_GUIEVENT_H
#define BRANEENGINE_GUIEVENT_H

#include <functional>
#include <string>

class GUIWindow;

class GUIEvent
{
    std::string _name;

  public:
    GUIEvent(std::string name);

    virtual ~GUIEvent() = default;

    const std::string& name() const;
};

struct GUIEventListener
{
    GUIWindow* window;
    std::function<void(const GUIEvent*)> callback;
};

#endif // BRANEENGINE_GUIEVENT_H
