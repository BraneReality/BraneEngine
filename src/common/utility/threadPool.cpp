#include "threadPool.h"

std::thread::id ThreadPool::main_thread_id;
size_t ThreadPool::_instances;
std::vector<std::thread> ThreadPool::_threads;
size_t ThreadPool::_staticThreads;
size_t ThreadPool::_minThreads;

std::atomic<bool> ThreadPool::_running = true;
std::queue<ThreadPool::Job> ThreadPool::_jobs;
std::mutex ThreadPool::_queueMutex;

int ThreadPool::threadRuntime()
{
	while (_running)
	{
		
		Job job;

		// Lock queue and retrieve job
		_queueMutex.lock();
		bool jobAvailable = !_jobs.empty();
		if (jobAvailable)
		{
			job = _jobs.front();
			_jobs.pop();
		}
		_queueMutex.unlock();

		// if we got a job, run it outside of the lock
		if (jobAvailable)
		{
			try
			{
				job.f();

			}
			catch (const std::exception& e)
			{
				std::cerr << "Thread Error: " << e.what() << std::endl;
			}
			job.handle->_instances -= 1;
		}
		
		std::this_thread::yield();
	}
	return 0;
}

void ThreadPool::init(size_t minThreads)
{
	_minThreads = minThreads;
	main_thread_id = std::this_thread::get_id();
	_instances++;
	if (_instances == 1)
	{
		_running = true;
		size_t threadCount = std::max<size_t>(std::thread::hardware_concurrency(), minThreads);
		_threads.reserve(threadCount);
		for (size_t i = 0; i < threadCount; i++)
		{
			_threads.emplace_back(threadRuntime);
		}
	}
	
}

void ThreadPool::cleanup()
{
	_instances--;
	if (_instances == 0)
	{
		_running = false;
		for (size_t i = 0; i < _threads.size(); i++)
		{
			try
			{
				if(_threads[i].joinable())
					_threads[i].join();
			}
			catch (const std::exception& e)
			{
				std::cout << "Error when exiting thread: " << e.what() << std::endl;
			}
		}
		_threads.clear();
	}
	
}

std::shared_ptr<JobHandle> ThreadPool::enqueue(std::function<void()> function)
{
	std::shared_ptr<JobHandle> handle = std::make_shared<JobHandle>();
	handle->_instances = 1;
	_queueMutex.lock();
	_jobs.push(Job( function, handle));
	_queueMutex.unlock();
	return handle;
}

void ThreadPool::enqueue(std::function<void()> function, std::shared_ptr<JobHandle> handle)
{
	handle->_instances += 1;
	_queueMutex.lock();
	_jobs.push(Job(function, handle));
	_queueMutex.unlock();
}

void ThreadPool::addStaticThread(std::function<void()> function)
{
	_staticThreads++;
	if(_threads.size() - _staticThreads < _minThreads)
		_threads.emplace_back(function);
	else
		enqueue(function);

}

bool JobHandle::finished()
{
	return _instances == 0;
}
void JobHandle::finish()
{
	while (!finished())
	{
		std::this_thread::yield();
	}
}

JobHandle::JobHandle()
{
	_instances = 0;
}

void ThreadPool::Job::operator=(const Job& job)
{
	f = job.f;
	handle = job.handle;
}

ThreadPool::Job::Job(std::function<void()> f, std::shared_ptr<JobHandle> handle)
{
	this->f = f;
	this->handle = handle;
}
