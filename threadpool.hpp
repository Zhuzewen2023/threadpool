#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

// #include "blocking_queue.hpp"
#include <functional>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

/*前置声明，仅仅只能用作指针或引用*/
template <typename T>
class SwapBlockingQueue;

class ThreadPool
{
public:
    explicit ThreadPool(int threads_num);

    ~ThreadPool();
    
    /*生产者线程接口，即生产任务*/
    void post(std::function<void()> task);

private:
    void worker();
    std::unique_ptr<SwapBlockingQueue<std::function<void()>>> task_queue_; /*阻塞队列，任务队列为空，阻塞线程*/
    std::vector<std::thread> workers_;
};

#endif

