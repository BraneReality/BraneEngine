//
// Created by eli on 3/7/2022.
//

#ifndef BRANEENGINE_STATICINDEXVECTOR_H
#define BRANEENGINE_STATICINDEXVECTOR_H

#include <cassert>
#include <functional>
#include <stack>
#include <vector>

template<typename T>
class staticIndexVector
{
    struct Element
    {
        T object;
        bool isUsed = false;
    };

    std::vector<Element> _data;
    std::stack<size_t> _unused;
    size_t _size = 0;

  public:
    size_t push(const T& element)
    {
        size_t index;
        if(!_unused.empty())
        {
            index = _unused.top();
            _data[index].object = element;
            _data[index].isUsed = true;
            _unused.pop();
        }
        else
        {
            index = _data.size();
            _data.push_back({element, true});
        }
        _size++;
        return index;
    }

    size_t push(T&& element)
    {
        size_t index;
        if(!_unused.empty())
        {
            index = _unused.top();
            _data[index].object = std::move(element);
            _data[index].isUsed = true;
            _unused.pop();
        }
        else
        {
            index = _data.size();
            _data.push_back({std::move(element), true});
        }
        _size++;
        return index;
    }

    void remove(size_t index)
    {
        _unused.push(index);
        _data[index].isUsed = false;
        _size--;
    }

    bool hasIndex(size_t index) const
    {
        if(index >= _data.size())
            return false;
        return _data[index].isUsed;
    }

    size_t size() const { return _size; }

    void clear()
    {
        _size = 0;
        while(!_unused.empty())
            _unused.pop();
        _data.clear();
    }

    const T& operator[](size_t index) const
    {
        assert(index < _data.size() && _data[index].isUsed);
        return _data[index].object;
    }

    T& operator[](size_t index)
    {
        assert(index < _data.size() && _data[index].isUsed);
        return _data[index].object;
    }

    class iterator
    {
        staticIndexVector<T>& _ref;
        size_t _index;

      public:
        iterator(staticIndexVector<T>& ref, size_t index) : _ref(ref), _index(index)
        {
            while(_index != _ref._data.size() && !_ref._data[_index].isUsed)
                ++_index;
        };

        void operator++()
        {
            ++_index;
            while(_index != _ref._data.size() && !_ref._data[_index].isUsed)
                ++_index;
        }

        void operator+(size_t index) { _index += index; }

        bool operator!=(const iterator& o) const { return _index != o._index; }

        bool operator==(const iterator& o) const { return _index == o._index; }

        T& operator*() const { return _ref[_index]; };

        size_t index() const { return _index; }

        using iterator_category = std::forward_iterator_tag;
        using reference = T&;
        using pointer = T*;
    };

    iterator begin() { return {*this, 0}; }

    iterator end() { return {*this, _data.size()}; }

    class const_iterator
    {
        const staticIndexVector<T>& _ref;
        size_t _index;

      public:
        const_iterator(const staticIndexVector<T>& ref, size_t index) : _ref(ref), _index(index)
        {
            while(_index != _ref._data.size() && !_ref._data[_index].isUsed)
                ++_index;
        };

        void operator++()
        {
            ++_index;
            while(_index != _ref._data.size() && !_ref._data[_index].isUsed)
                ++_index;
        }

        void operator+(size_t index) { _index += index; }

        bool operator!=(const const_iterator& o) const { return _index != o._index; }

        bool operator==(const const_iterator& o) const { return _index == o._index; }

        const T& operator*() const { return _ref[_index]; };

        size_t index() const { return _index; }

        using iterator_category = std::forward_iterator_tag;
        using reference = T&;
        using pointer = T*;
    };

    const_iterator begin() const { return {*this, 0}; }

    const_iterator end() const { return {*this, _data.size()}; }
};

#endif // BRANEENGINE_STATICINDEXVECTOR_H