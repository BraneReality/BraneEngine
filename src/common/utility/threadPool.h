#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <condition_variable>
#include <system_error>

#include <iostream>

// Eventually I might want to move this into the runtime class

class JobHandle
{
    std::atomic<size_t> _instances;
    std::function<void()> _next;
    std::shared_ptr<JobHandle> _nextHandle;

    friend class ThreadPool;

    void enqueueNext();

  public:
    bool finished();

    void finish();

    std::shared_ptr<JobHandle> then(std::function<void()> f);

    JobHandle();
};

struct BraneJob
{
    std::function<void()> f;
    std::shared_ptr<JobHandle> handle;

    BraneJob(const BraneJob&) = delete;

    BraneJob(BraneJob&&) = default;

    BraneJob& operator=(BraneJob&&) = default;

    BraneJob() = default;

    BraneJob(std::function<void()> f, std::shared_ptr<JobHandle> handle);
};

struct ConditionJob
{
    std::function<void()> f;
    size_t conditionCount;

    void signal();
};

class ThreadPool
{
    static size_t _instances;
    static std::vector<std::thread> _threads;
    static size_t _staticThreads;
    static size_t _minThreads;

    static std::mutex _queueMutex;
    static std::condition_variable _workAvailable;
    static std::queue<BraneJob> _jobs;
    static std::mutex _mainQueueMutex;
    static std::queue<BraneJob> _mainThreadJobs;
    static std::atomic_bool _running;

    static int threadRuntime();

  public:
    static std::thread::id main_thread_id;

    static void init(size_t minThreads);

    static void runMainJobs();

    static void cleanup();

    static std::shared_ptr<JobHandle> addStaticThread(std::function<void()> function);

    static void addStaticTimedThread(std::function<void()> function, std::chrono::seconds interval);

    static std::shared_ptr<JobHandle> enqueue(std::function<void()> function);

    static void enqueueMain(std::function<void()> function);

    static void enqueue(std::function<void()> function, std::shared_ptr<JobHandle>& sharedHandle);

    static std::shared_ptr<JobHandle> enqueueBatch(std::vector<std::function<void()>> functions);

    static std::shared_ptr<ConditionJob> conditionalEnqueue(std::function<void()> function, size_t conditionCount);
};

#define IS_MAIN_THREAD() std::this_thread::get_id() == ThreadPool::main_thread_id
#define ASSERT_MAIN_THREAD() assert(IS_MAIN_THREAD())
