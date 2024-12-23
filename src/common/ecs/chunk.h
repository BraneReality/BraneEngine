#pragma once

#include "virtualType.h"
#include <array>
#include <vector>

#include "component.h"
#include "utility/sharedRecursiveMutex.h"

class ChunkComponentView {
    const ComponentDescription* _description = nullptr;
    byte* _data = nullptr;
    size_t _maxSize = 0;
    size_t _size = 0;

    byte* dataIndex(size_t index) const;

    SharedRecursiveMutex _mutex;

  public:
    uint32_t version = 0;

    ChunkComponentView() = default;

    ChunkComponentView(byte* data, size_t maxSize, const ComponentDescription* def);

    ChunkComponentView(const ChunkComponentView&);

    ChunkComponentView& operator=(const ChunkComponentView&);

    ChunkComponentView(ChunkComponentView&&);

    ~ChunkComponentView();

    VirtualComponentView operator[](size_t index) const;

    void createComponent();

    void erase(size_t index);

    void setComponent(size_t index, VirtualComponentView component);

    void setComponent(size_t index, VirtualComponent&& component);

    byte* getComponentData(size_t index);

    void lockShared();

    void unlockShared();

    void lock();

    void unlock();

    size_t size() const;

    uint32_t compID() const;

    const ComponentDescription* def();
};

template <size_t N> class ChunkBase {
#ifdef TEST_BUILD
  public:
#endif
    size_t _size;
    size_t _maxCapacity;
    std::array<byte, N> _data;
    std::unordered_map<ComponentID, ChunkComponentView> _components;

  public:
    ChunkBase() { _size = 0; }

    ChunkBase(const ChunkBase&) = delete;

    ~ChunkBase() { clear(); }

    void setComponents(std::vector<const ComponentDescription*> components)
    {
        clear();
        _components.reserve(components.size());
        byte* componentDataStart = _data.data();
        size_t entitySize = 0;
        for(auto& c : components)
            entitySize += c->size();
        _maxCapacity = N / entitySize;

        for(auto& c : components) {
            _components.insert({c->id, ChunkComponentView{componentDataStart, _maxCapacity, c}});
            componentDataStart += c->size() * _maxCapacity;
        }
    }

    const std::unordered_map<ComponentID, ChunkComponentView>& components() { return _components; }

    bool hasComponent(uint32_t id) { return _components.contains(id); }

    ChunkComponentView& getComponent(uint32_t id)
    {
        auto c = _components.find(id);
        if(c != _components.end())
            return c->second;
        throw std::runtime_error("Tried to access component not contained by chunk");
    }

    bool tryGetComponent(uint32_t id, ChunkComponentView*& view)
    {
        auto c = _components.find(id);
        bool found = c != _components.end();
        if(found)
            view = &c->second;
        return found;
    }

    size_t createEntity()
    {
        assert(_size < _maxCapacity);
        for(auto& c : _components) {
            c.second.createComponent();
            c.second.version++;
        }
        return _size++;
    }

    void moveEntity(ChunkBase* dest, size_t sIndex, size_t dIndex)
    {
        assert(sIndex < _size);
        assert(dIndex < dest->_size);
        for(auto& c : _components) {
            ChunkComponentView* oc = nullptr;
            if(dest->tryGetComponent(c.second.compID(), oc)) {
                c.second.def()->move(c.second[sIndex].data(), (*oc)[dIndex].data());
                oc->version = std::max(oc->version, c.second.version);
            }
        }
    }

    void removeEntity(size_t index)
    {
        assert(index < _size);
        --_size;
        for(auto& c : _components) {
            c.second.erase(index);
            assert(_size == c.second.size());
        }
    }

    void clear()
    {
        _components.clear();
        _size = 0;
        _maxCapacity = 0;
    }

    size_t size() { return _size; }

    size_t maxCapacity() { return _maxCapacity; }

    byte* data() { return _data.data(); }

    static size_t allocationSize() { return N; }
};

template <size_t> class ChunkBase;

using Chunk = ChunkBase<16384>;

class ChunkPool {
#ifdef TEST_BUILD
  public:
#endif
    std::mutex _m;
    std::vector<std::unique_ptr<Chunk>> _unused;

  public:
    friend void operator>>(ChunkPool& pool, std::unique_ptr<Chunk>& dest);

    friend void operator<<(ChunkPool& pool, std::unique_ptr<Chunk>& src);
};
