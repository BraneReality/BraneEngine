#include "utility/threadPool.h"
#include "testing.h"

TEST(Threading, ThreadPoolTest)
{
    ThreadPool::init(4);
    std::atomic_bool testBool = false;
    std::shared_ptr<JobHandle> jh = ThreadPool::enqueue([&]() { testBool = true; });
    jh->finish();

    ThreadPool::cleanup();

    EXPECT_TRUE(testBool);
}
