//
// Created by eli on 5/21/2022.
//

#ifndef BRANEENGINE_CONSOLEWINDOW_H
#define BRANEENGINE_CONSOLEWINDOW_H

#include <string>
#include "editorWindow.h"
#include <runtime/logging.h>

class ConsoleWindow : public EditorWindow
{
    struct CachedLog
    {
        std::string text = "";
        Logging::LogLevel level = Logging::LogLevel::log;
        float lineCount;
    };

    std::vector<CachedLog> _messages;
    size_t _listenerIndex;
    bool _autoScroll = true;

    void displayContent() override;

  public:
    ConsoleWindow(GUI& ui, Editor& editor);

    ~ConsoleWindow();
};

#endif // BRANEENGINE_CONSOLEWINDOW_H
