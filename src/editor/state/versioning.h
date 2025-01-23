#pragma once

#include <deque>
#include "utility/option.h"
#include "utility/result.h"

struct EditorActionContext
{
};

enum class EditorActionType
{
    Standard,
    Preview
};

using EditorActionExecutor = std::function<void(std::shared_ptr<EditorActionContext> ctx, EditorActionType type)>;

struct EditorAction
{
    std::shared_ptr<EditorActionContext> context;
    EditorActionExecutor forwardAction;
    EditorActionExecutor backAction;

    inline EditorAction(std::shared_ptr<EditorActionContext> context,
                        EditorActionExecutor forwardAction,
                        EditorActionExecutor backAction)
        : context(context), forwardAction(forwardAction), backAction(backAction) {};

    void forward(EditorActionType type = EditorActionType::Standard);
    void back(EditorActionType type = EditorActionType::Standard);
};

class EditorActionManager
{
    std::deque<EditorAction> _actions;
    std::deque<EditorAction>::iterator _currentAction;
    Option<EditorAction> _previewChange;
    size_t _maxActions = 200;

  public:
    EditorActionManager();

    void stepForward();
    void stepBack();

    void executeAction(EditorAction action, bool preview = false);
};
