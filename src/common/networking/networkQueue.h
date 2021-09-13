#pragma once
#include <mutex>
#include <queue>
#include <iterator>

namespace net
{
	template <class T>
	class NetQueue
	{
	private:
		std::mutex _m;
		std::deque<T> _queue;
	public:
		NetQueue() = default;
		NetQueue(const NetQueue&) = delete;
		~NetQueue()
		{
			_queue.clear();
		}
		const T& front()
		{
			std::scoped_lock lock(_m);
			return _queue.front();
		}
	
		const T& back()
		{
			std::scoped_lock lock(_m);
			return _queue.back();
		}

		void push_back(const T& value)
		{
			std::scoped_lock lock(_m);
			_queue.emplace_back(value);
		}

		void push_front(const T& value)
		{
			std::scoped_lock lock(_m);
			_queue.emplace_back(value);
		}

		size_t count()
		{
			std::scoped_lock lock(_m);
			return _queue.size();
		}

		bool empty()
		{
			return count() == 0;
		}

		void clean()
		{
			std::scoped_lock lock(_m);
			_queue.erase(std::remove(_queue.begin(), _queue.end(), nullptr));
		}

		void erase(T& value)
		{
			std::scoped_lock lock(_m);
			_queue.erase(std::remove(_queue.begin(), _queue.end(), value));
		}

		void clear()
		{
			std::scoped_lock lock(_m);
			_queue.clear();
		}

		T pop_front()
		{
			std::scoped_lock lock(_m);
			T value = std::move(_queue.front());
			_queue.pop_front();
			return value;
		}

		T pop_back()
		{
			std::scoped_lock lock(_m);
			T value = std::move(_queue.back());
			_queue.pop_back();
			return value;
		}

		void lock()
		{
			_m.lock();
		}

		void unlock()
		{
			_m.unlock();
		}
		 typename std::deque<T>::iterator begin() { return _queue.begin(); }
		 typename std::deque<T>::iterator end() { return _queue.end(); }
	};
}