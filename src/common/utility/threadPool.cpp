#include "threadPool.h"
#include <runtime/runtime.h>

#include <memory>

BraneJob::BraneJob(std::function<void()> f, std::shared_ptr<JobHandle> handle)
    : f(std::move(f)), handle(std::move(handle))
{
}

std::thread::id ThreadPool::main_thread_id;
size_t ThreadPool::_instances;
std::vector<std::thread> ThreadPool::_threads;
size_t ThreadPool::_staticThreads;
size_t ThreadPool::_minThreads;

std::atomic<bool> ThreadPool::_running = true;
std::queue<BraneJob> ThreadPool::_jobs;
std::mutex ThreadPool::_queueMutex;
std::condition_variable ThreadPool::_workAvailable;

std::mutex ThreadPool::_mainQueueMutex;
std::queue<BraneJob> ThreadPool::_mainThreadJobs;

int ThreadPool::threadRuntime()
{
    while(_running) {

        BraneJob job;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            if(_workAvailable.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(200), [] {
                   return !_jobs.empty();
               })) {
                job = std::move(_jobs.front());
                _jobs.pop();
            }
            else
                continue;
        }
#if NDEBUG
        try {
#endif
            job.f();
#if NDEBUG
        }
        catch(const std::exception& e) {
            std::cerr << "Thread Error: " << e.what() << std::endl;
        }
#endif
        job.handle->_instances -= 1;
        if(job.handle->_instances == 0)
            job.handle->enqueueNext();
    }
    return 0;
}

void ThreadPool::init(size_t minThreads)
{
    _minThreads = minThreads;
    main_thread_id = std::this_thread::get_id();
    _instances++;
    if(_instances == 1) {
        _running = true;
        size_t threadCount = std::max<size_t>(std::thread::hardware_concurrency(), minThreads);
        _threads.reserve(threadCount);
        for(size_t i = 0; i < threadCount; i++) {
            _threads.emplace_back(threadRuntime);
        }
        Runtime::log("Started up thread pool with " + std::to_string(threadCount) + " threads");
    }
}

void ThreadPool::cleanup()
{
    _instances--;
    if(_instances == 0) {
        _running = false;
        for(auto& _thread : _threads) {
            try {
                if(_thread.joinable())
                    _thread.join();
            }
            catch(const std::exception& e) {
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
    _jobs.emplace(std::move(function), handle);
    _queueMutex.unlock();
    _workAvailable.notify_one();
    return handle;
}

void ThreadPool::enqueueMain(std::function<void()> function)
{
    _mainQueueMutex.lock();
    _mainThreadJobs.emplace(std::move(function), nullptr);
    _mainQueueMutex.unlock();
}

void ThreadPool::enqueue(std::function<void()> function, std::shared_ptr<JobHandle>& sharedHandle)
{
    sharedHandle->_instances += 1;
    _queueMutex.lock();
    _jobs.emplace(std::move(function), sharedHandle);
    _queueMutex.unlock();
    _workAvailable.notify_one();
}

std::shared_ptr<JobHandle> ThreadPool::enqueueBatch(std::vector<std::function<void()>> functions)
{
    std::shared_ptr<JobHandle> handle = std::make_shared<JobHandle>();

    _queueMutex.lock();
    for(auto& function : functions) {
        handle->_instances += 1;
        _jobs.emplace(std::move(function), handle);
    }
    _queueMutex.unlock();

    _workAvailable.notify_one();
    return handle;
}

std::shared_ptr<JobHandle> ThreadPool::addStaticThread(std::function<void()> function)
{
    std::shared_ptr<JobHandle> handle = std::make_shared<JobHandle>();

    _staticThreads++;
    if(_threads.size() - _staticThreads < _minThreads) {
        _threads.emplace_back(function);
        _workAvailable.notify_one();
    }
    else
        enqueue(std::move(function), handle);
    return handle;
}

void ThreadPool::addStaticTimedThread(std::function<void()> function, std::chrono::seconds interval)
{
    auto* lastExecution = new std::chrono::time_point<std::chrono::system_clock>; // Memory leak alert!
    *lastExecution = std::chrono::system_clock::now();
    addStaticThread([function, interval, lastExecution]() {
        function();

        auto nextExecution = *lastExecution + interval;
        *lastExecution = std::chrono::system_clock::now();
        std::this_thread::sleep_until(nextExecution);
    });
}

std::shared_ptr<ConditionJob> ThreadPool::conditionalEnqueue(std::function<void()> function, size_t conditionCount)
{
    assert(conditionCount != 0);
    return std::make_shared<ConditionJob>(ConditionJob{std::move(function), conditionCount});
}

void ThreadPool::runMainJobs()
{
    _mainQueueMutex.lock();
    while(!_mainThreadJobs.empty()) {
        auto job = std::move(_mainThreadJobs.front());
        _mainThreadJobs.pop();
        _mainQueueMutex.unlock();

        job.f();

        _mainQueueMutex.lock();
    }
    _mainQueueMutex.unlock();
}

bool JobHandle::finished() { return _instances == 0; }

void JobHandle::finish()
{
    while(!finished()) {
        std::this_thread::yield();
    }
}

JobHandle::JobHandle() { _instances = 0; }

std::shared_ptr<JobHandle> JobHandle::then(std::function<void()> f)
{
    _next = std::move(f);
    if(!_nextHandle)
        _nextHandle = std::make_shared<JobHandle>();
    return _nextHandle;
}

void JobHandle::enqueueNext()
{
    if(_next)
        ThreadPool::enqueue(_next, _nextHandle);
}

void ConditionJob::signal()
{
    if(--conditionCount == 0)
        ThreadPool::enqueue(std::move(f));
}
