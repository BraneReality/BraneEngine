//
// Created by eli on 8/18/2022.
//

#include "jsonVersioner.h"
#include <algorithm>
#include <cassert>
#include <iostream>

Json::Value& Json::resolvePath(const std::string& path, Json::Value& root)
{
    Json::Value* value = &root;
    std::vector<std::string> components;
    uint32_t begin = 0;
    for(uint32_t end = 1; end < path.size(); ++end)
    {
        if(path[end] == '/' && begin < end)
        {
            components.emplace_back(path.begin() + begin, path.begin() + end);
            begin = end + 1;
        }
    }
    if(begin < path.size())
        components.emplace_back(path.begin() + begin, path.end());
    for(auto& comp : components)
    {
        bool isIndex = true;
        for(char c : comp)
        {
            if(!std::isdigit(c))
            {
                isIndex = false;
                break;
            }
        }
        if(!isIndex)
            value = &(*value)[comp];
        else
            value = &(*value)[std::stoi(comp)];
    }

    return *value;
}

std::string Json::getPathComponent(const std::string& path, uint32_t index)
{
    uint32_t begin = 0;
    uint32_t searchIndex = 0;
    for(uint32_t end = 1; end < path.size(); ++end)
    {
        if(path[end] == '/' && begin < end)
        {
            if(searchIndex++ == index)
                return {path.begin() + begin, path.begin() + end};
            begin = end + 1;
        }
    }
    return {path.begin() + begin, path.end()};
}

void Json::insertArrayValue(const Value& value, ArrayIndex index, Value& array)
{
    if(array.isNull() || array.size() == index)
        array.append(value);
    else
        assert(array.insert(index, value));
}

void Json::eraseArrayValue(ArrayIndex index, Value& array)
{
    assert(array.isArray());
    assert(index < array.size());
    Json::Value newArray;
    for(Json::ArrayIndex i = 0; i < array.size(); ++i)
    {
        if(i != index)
            newArray.append(array[i]);
    }
    array = std::move(newArray);
}

JsonChangeBase::JsonChangeBase(VersionedJson* json) { _json = json; }

const VersionedJson* JsonChangeBase::json() { return _json; }

JsonChange::JsonChange(const std::string& path, Json::Value newValue, VersionedJson* json)
    : JsonChange(path, Json::nullValue, std::move(newValue), json)
{}

JsonChange::JsonChange(const std::string& path, Json::Value oldValue, Json::Value newValue, VersionedJson* json)
    : _path(path), _before(std::move(oldValue)), _after(std::move(newValue)), JsonChangeBase(json)
{}

void JsonChange::undo() { Json::resolvePath(_path, _json->data()) = _before; }

void JsonChange::redo()
{
    auto& value = Json::resolvePath(_path, _json->data());
    if(_before.isNull())
        _before = value;
    value = _after;
}

const Json::Value& JsonChange::before() const { return _before; }

const Json::Value& JsonChange::after() const { return _after; }

void JsonVersionTracker::recordChange(std::unique_ptr<JsonChangeBase> change)
{
    change->redo();
    change->_json->_version++;
    if(_currentChange != _changes.end())
        _changes.erase(_currentChange, _changes.end());
    _changes.push_back(std::move(change));
    while(_changes.size() > maxChanges)
        _changes.pop_front();
    _currentChange = _changes.end();
}

void JsonVersionTracker::undo()
{
    if(_currentChange == _changes.begin())
        return;
    --_currentChange;
    (*_currentChange)->undo();
    (*_currentChange)->_json->_version--;
}

void JsonVersionTracker::redo()
{
    if(_currentChange == _changes.end())
        return;
    (*_currentChange)->redo();
    (*_currentChange)->_json->_version++;
    ++_currentChange;
}

void JsonVersionTracker::clearChanges(const VersionedJson* json)
{
    if(_currentChange != _changes.end())
        _changes.erase(_currentChange, _changes.end());
    _changes.erase(std::remove_if(_changes.begin(), _changes.end(), [json](auto& c) { return c->json() == json; }),
                   _changes.end());

    _currentChange = _changes.end();
}

JsonVersionTracker::JsonVersionTracker() { _currentChange = _changes.end(); }

VersionedJson::VersionedJson(JsonVersionTracker& tkr) : _tkr(tkr) {}

void VersionedJson::initialize(const Json::Value& value)
{
    _root = value;
    _version = 0;
    _lastCleanedVersion = 0;
    _tkr.clearChanges(this);
}

void VersionedJson::changeValue(const std::string& path, const Json::Value& newValue, bool changeComplete)
{
    if(!changeComplete)
    {
        if(!_uncompletedChange)
        {
            _uncompletedChange = std::make_unique<UncompletedChange>();
            _uncompletedChange->path = path;
            _uncompletedChange->before = Json::resolvePath(path, _root);
        }
        Json::resolvePath(path, _root) = newValue;
        return;
    }

    if(_uncompletedChange)
    {
        assert(_uncompletedChange->path == path);
        if(!_multiChanges.empty())
            _multiChanges.top()->changes.push_back(
                std::make_unique<JsonChange>(path, _uncompletedChange->before, newValue, this));
        else
            _tkr.recordChange(std::make_unique<JsonChange>(path, _uncompletedChange->before, newValue, this));
        _uncompletedChange = nullptr;
        return;
    }

    if(!_multiChanges.empty())
        _multiChanges.top()->changes.push_back(std::make_unique<JsonChange>(path, newValue, this));
    else
        _tkr.recordChange(std::make_unique<JsonChange>(path, newValue, this));
}

void VersionedJson::markClean() { _lastCleanedVersion = _version; }

bool VersionedJson::dirty() const { return _lastCleanedVersion != _version; }

Json::Value& VersionedJson::data() { return _root; }

JsonVersionTracker& VersionedJson::tracker() { return _tkr; }

const Json::Value& VersionedJson::operator[](const std::string& p) const { return _root[p]; }

const Json::Value& VersionedJson::operator[](const char* p) const { return _root[p]; }

void VersionedJson::beginMultiChange(bool dontReverse)
{
    _multiChanges.push(std::make_unique<MultiJsonChange>(dontReverse, this));
}

void VersionedJson::endMultiChange()
{
    assert(!_multiChanges.empty());
    auto change = std::move(_multiChanges.top());
    _multiChanges.pop();
    if(change->changes.empty())
        return;
    if(change->changes.size() == 1)
    {
        recordChange(std::move(change->changes[0]));
        return;
    }
    recordChange(std::move(change));
}

void VersionedJson::insertIndex(const std::string& path, Json::ArrayIndex index, const Json::Value& newValue)
{
    recordChange(std::make_unique<JsonArrayChange>(path, index, newValue, true, this));
}

void VersionedJson::removeIndex(const std::string& path, Json::ArrayIndex index)
{
    recordChange(std::make_unique<JsonArrayChange>(path, index, Json::nullValue, false, this));
}

void VersionedJson::appendValue(const std::string& path, const Json::Value& newValue)
{
    recordChange(std::make_unique<JsonArrayChange>(path, Json::resolvePath(path, _root).size(), newValue, true, this));
}

void VersionedJson::recordChange(std::unique_ptr<JsonChangeBase>&& change)
{
    if(!_multiChanges.empty())
        _multiChanges.top()->changes.push_back(std::move(change));
    else
        _tkr.recordChange(std::move(change));
}

void VersionedJson::markDirty() { ++_version; }

MultiJsonChange::MultiJsonChange(bool dontReverse, VersionedJson* json) : dontReverse(dontReverse), JsonChangeBase(json)
{}

void MultiJsonChange::undo()
{
    if(!dontReverse)
    {
        for(size_t i = 1; i <= changes.size(); ++i)
            changes[changes.size() - i]->undo();
    }
    else
    {
        for(auto& change : changes)
            change->undo();
    }
}

void MultiJsonChange::redo()
{
    for(auto& change : changes)
        change->redo();
}

JsonArrayChange::JsonArrayChange(
    std::string path, Json::ArrayIndex index, Json::Value value, bool inserting, VersionedJson* json)
    : JsonChangeBase(json)
{
    _path = std::move(path);
    _index = index;
    _value = std::move(value);
    _inserting = inserting;
}

void JsonArrayChange::undo()
{
    if(_inserting)
        removeValue();
    else
        insertValue();
}

void JsonArrayChange::redo()
{
    if(_inserting)
        insertValue();
    else
        removeValue();
}

void JsonArrayChange::insertValue()
{
    auto& array = Json::resolvePath(_path, _json->data());
    Json::insertArrayValue(_value, _index, array);
}

void JsonArrayChange::removeValue()
{
    auto& array = Json::resolvePath(_path, _json->data());
    assert(array.isArray());
    assert(_index < array.size());
    if(_value.isNull())
        _value = array[_index];

    Json::eraseArrayValue(_index, array);
}

Json::ArrayIndex JsonArrayChange::index() const { return _index; }

bool JsonArrayChange::inserting() const { return _inserting; }

const Json::Value& JsonArrayChange::value() const { return _value; }
