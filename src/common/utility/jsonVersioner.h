//
// Created by eli on 8/18/2022.
//

#ifndef BRANEENGINE_JSONVERSIONER_H
#define BRANEENGINE_JSONVERSIONER_H

#include <deque>
#include <functional>
#include <json/json.h>

class VersionedJson;

namespace Json
{
    Value& resolvePath(const std::string& path, Json::Value& root);

    std::string getPathComponent(const std::string& path, uint32_t index);

    void insertArrayValue(const Value& value, ArrayIndex index, Value& array);

    void eraseArrayValue(ArrayIndex index, Value& array);
} // namespace Json

class JsonChangeBase
{
    friend class JsonVersionTracker;

  protected:
    VersionedJson* _json = nullptr;

  public:
    JsonChangeBase(VersionedJson* json);

    const VersionedJson* json();

    virtual void undo() = 0;

    virtual void redo() = 0;
};

class JsonChange : public JsonChangeBase
{
  protected:
    std::string _path;
    Json::Value _before;
    Json::Value _after;

  public:
    JsonChange(const std::string& path, Json::Value newValue, VersionedJson* json);

    JsonChange(const std::string& path, Json::Value oldValue, Json::Value newValue, VersionedJson* json);

    const Json::Value& before() const;

    const Json::Value& after() const;

    void undo() override;

    void redo() override;
};

class MultiJsonChange : public JsonChangeBase
{
  public:
    bool dontReverse = false;
    std::vector<std::unique_ptr<JsonChangeBase>> changes;

    MultiJsonChange(bool dontReverse, VersionedJson* json);

    void undo() override;

    void redo() override;
};

class JsonArrayChange : public JsonChangeBase
{
  protected:
    std::string _path;
    Json::ArrayIndex _index;
    Json::Value _value;
    bool _inserting = true;

    void insertValue();

    void removeValue();

  public:
    JsonArrayChange(std::string path, Json::ArrayIndex index, Json::Value value, bool inserting, VersionedJson* json);

    Json::ArrayIndex index() const;

    bool inserting() const;

    const Json::Value& value() const;

    void undo() override;

    void redo() override;
};

class JsonVersionTracker
{
    std::deque<std::unique_ptr<JsonChangeBase>> _changes;
    std::deque<std::unique_ptr<JsonChangeBase>>::iterator _currentChange;

    // TODO add this to config
    size_t maxChanges = 200;

  public:
    JsonVersionTracker();

    void recordChange(std::unique_ptr<JsonChangeBase> change);

    void clearChanges(const VersionedJson* json);

    void undo();

    void redo();
};

class VersionedJson
{
    JsonVersionTracker& _tkr;
    Json::Value _root;
    uint32_t _version = 0;
    uint32_t _lastCleanedVersion = 0;

    struct UncompletedChange
    {
        std::string path;
        Json::Value before;
    };

    std::unique_ptr<UncompletedChange> _uncompletedChange;
    std::stack<std::unique_ptr<MultiJsonChange>> _multiChanges;

    friend class JsonVersionTracker;

  public:
    VersionedJson(JsonVersionTracker& tkr);

    void initialize(const Json::Value& value);

    void changeValue(const std::string& path, const Json::Value& newValue, bool changeComplete = true);

    void insertIndex(const std::string& path, Json::ArrayIndex index, const Json::Value& newValue);

    void removeIndex(const std::string& path, Json::ArrayIndex index);

    void appendValue(const std::string& path, const Json::Value& newValue);

    void recordChange(std::unique_ptr<JsonChangeBase>&& change);

    void beginMultiChange(bool dontReverse = false);

    void endMultiChange();

    void markDirty();

    void markClean();

    bool dirty() const;

    Json::Value& data();

    const Json::Value& operator[](const std::string&) const;

    const Json::Value& operator[](const char*) const;

    JsonVersionTracker& tracker();
};

#endif // BRANEENGINE_JSONVERSIONER_H
