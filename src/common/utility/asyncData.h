//
// Created by eli on 7/9/2022.
//

#ifndef BRANEENGINE_ASYNCDATA_H
#define BRANEENGINE_ASYNCDATA_H

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <runtime/runtime.h>
#include <utility/threadPool.h>

template<typename T>
class AsyncData
{
    struct InternalData
    {
        std::unique_ptr<T> _data;
        std::function<void(T)> _callback;
        bool _thenMain = false;
        std::string _errorMessage;
        std::function<void(const std::string&)> _error;
        std::mutex _lock;
    };

    std::shared_ptr<InternalData> _instance;

  public:
    AsyncData()
    {
        _instance = std::make_shared<InternalData>();
    }

    AsyncData& then(const std::function<void(T)>& callback)
    {
        std::scoped_lock lock(_instance->_lock);
        if(_instance->_data)
            callback(std::move(*_instance->_data));
        else
            _instance->_callback = callback;
        return *this;
    }

    AsyncData& thenMain(const std::function<void(T)>& callback)
    {
        std::scoped_lock lock(_instance->_lock);
        _instance->_thenMain = true;
        if(_instance->_data)
        {
            if(IS_MAIN_THREAD())
                callback(std::move(*_instance->_data));
            else
            {
                const auto& data = std::move(*_instance->_data);
                ThreadPool::enqueueMain([callback, data = std::move(data)] { callback(std::move(data)); });
            }
        }
        else
            _instance->_callback = callback;
        return *this;
    }

    AsyncData& onError(const std::function<void(const std::string& err)>& callback)
    {
        std::scoped_lock lock(_instance->_lock);
        if(_instance->_errorMessage.empty())
            _instance->_error = callback;
        else
            callback(_instance->_errorMessage);
        return *this;
    }

    void setData(const T& data) const
    {
        std::scoped_lock lock(_instance->_lock);
        if(_instance->_callback)
        {
            if(_instance->_thenMain)
            {
                if(IS_MAIN_THREAD())
                    _instance->_callback(std::move(*_instance->_data));
                else
                {
                    const auto& callback = _instance->_callback;
                    ThreadPool::enqueueMain([callback, data] { callback(data); });
                }
            }
            else
                _instance->_callback(data);
        }
        else
            _instance->_data = std::make_unique<T>(data);
    }

    void setError(const std::string& error) const
    {
        std::scoped_lock lock(_instance->_lock);
        if(!_instance->_errorMessage.empty())
            return;
        if(_instance->_error)
        {
            _instance->_errorMessage = error;
            _instance->_error(error);
        }
        else
            _instance->_errorMessage = error;
    }

    [[nodiscard]] bool isCallbackSet() const
    {
        std::scoped_lock lock(_instance->_lock);
        return (bool)(_instance->_callback);
    }
};

#endif // BRANEENGINE_ASYNCDATA_H
