#include "chunk.h"
#include "component.h"

void operator>>(ChunkPool& pool, std::unique_ptr<Chunk>& dest)
{
    std::scoped_lock lock(pool._m);
    if(!pool._unused.empty())
    {
        dest = std::move(pool._unused[pool._unused.size() - 1]);
        pool._unused.erase(pool._unused.end() - 1);
    }
    else
        dest = std::make_unique<Chunk>();
}

void operator<<(ChunkPool& pool, std::unique_ptr<Chunk>& src)
{
    std::scoped_lock lock(pool._m);
    src->clear();
    pool._unused.emplace_back(std::move(src));
}

ChunkComponentView::ChunkComponentView(byte* data, size_t maxSize, const ComponentDescription* def)
    : _data(data), _maxSize(maxSize), _description(def)
{
    _size = 0;
    version = 0;
}

ChunkComponentView::ChunkComponentView(const ChunkComponentView& o)
{
    _description = o._description;
    _data = o._data;
    _size = o._size;
    _maxSize = o._maxSize;
    version = o.version;
}

ChunkComponentView& ChunkComponentView::operator=(const ChunkComponentView& o)
{
    _description = o._description;
    _data = o._data;
    _size = o._size;
    _maxSize = o._maxSize;
    version = o.version;
    return *this;
}

ChunkComponentView::ChunkComponentView(ChunkComponentView&& o)
{
    _description = o._description;
    _data = o._data;
    _size = o._size;
    _maxSize = o._maxSize;
    version = o.version;
    o._data = nullptr;
    o._size = 0;
    o.version = 0;
}

ChunkComponentView::~ChunkComponentView()
{
    for(size_t i = 0; i < _size; ++i)
        _description->deconstruct(dataIndex(i));
}

VirtualComponentView ChunkComponentView::operator[](size_t index) const
{
    assert(index < _size);
    return VirtualComponentView(_description, dataIndex(index));
}

byte* ChunkComponentView::dataIndex(size_t index) const { return _data + _description->size() * index; }

void ChunkComponentView::createComponent()
{
    assert(_size < _maxSize);
    _description->construct(dataIndex(_size));
    ++_size;
}

void ChunkComponentView::erase(size_t index)
{
    assert(index < _size);
    --_size;
    if(_size > 0)
        _description->move(dataIndex(_size), dataIndex(index));
    _description->deconstruct(dataIndex(_size));
}

size_t ChunkComponentView::size() const { return _size; }

uint32_t ChunkComponentView::compID() const { return _description->id; }

const ComponentDescription* ChunkComponentView::def() { return _description; }

void ChunkComponentView::setComponent(size_t index, const VirtualComponentView component)
{
    assert(index < _size);
    _description->copy(component.data(), dataIndex(index));
}

void ChunkComponentView::setComponent(size_t index, VirtualComponent&& component)
{
    assert(index < _size);
    _description->move(component.data(), dataIndex(index));
}

byte* ChunkComponentView::getComponentData(size_t index)
{
    assert(index < _size);
    return dataIndex(index);
}

void ChunkComponentView::lockShared() { _mutex.lock_shared(); }

void ChunkComponentView::unlockShared() { _mutex.unlock_shared(); }

void ChunkComponentView::lock() { _mutex.lock(); }

void ChunkComponentView::unlock() { _mutex.unlock(); }
