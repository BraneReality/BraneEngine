
#include "versioning.h"

void EditorAction::forward(EditorActionType type)
{
    forwardAction(context, type);
}

void EditorAction::back(EditorActionType type)
{
    backAction(context, type);
}

EditorActionManager::EditorActionManager()
{
    _currentAction = _actions.end();
}

void EditorActionManager::stepBack()
{
    if(_currentAction == _actions.begin())
        return;
    if(_previewChange)
    {
        _previewChange.value().back(EditorActionType::Preview);
        _previewChange = None();
    }
    --_currentAction;
    _currentAction->back(EditorActionType::Standard);
}

void EditorActionManager::stepForward()
{
    if(_currentAction == _actions.end())
        return;
    if(_previewChange)
    {
        _previewChange.value().back(EditorActionType::Preview);
        _previewChange = None();
    }
    _currentAction->forward(EditorActionType::Standard);
    ++_currentAction;
}

void EditorActionManager::executeAction(EditorAction action, bool preivew)
{
    if(_previewChange)
        _previewChange.value().back(EditorActionType::Preview);

    action.forward(EditorActionType::Standard);
    if(preivew)
    {
        _previewChange = std::move(action);
        return;
    }
    if(_currentAction != _actions.end())
        _actions.erase(_currentAction, _actions.end());
    _actions.push_back(std::move(action));
    _currentAction = _actions.end();

    while(_actions.size() > _maxActions)
        _actions.pop_front();
}
