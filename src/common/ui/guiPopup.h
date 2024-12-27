//
// Created by wirewhiz on 21/07/22.
//

#ifndef BRANEENGINE_GUIPOPUP_H
#define BRANEENGINE_GUIPOPUP_H

#include <string>

typedef unsigned int ImGuiID;

class GUIPopup
{
    ImGuiID _id;
    std::string _name;

  protected:
    virtual void drawBody() = 0;

  public:
    GUIPopup(const std::string& name);

    virtual ~GUIPopup() = default;

    void draw();
};

#endif // BRANEENGINE_GUIPOPUP_H
