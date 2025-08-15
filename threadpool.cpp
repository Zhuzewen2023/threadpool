#include "blocking_queue.hpp"
#include "threadpool.hpp"

ThreadPool::ThreadPool(int threads_num) {
    task_queue_ = std::make_unique<SwapBlockingQueue<std::function<void()>>>();
    for (size_t i = 0; i < threads_num; ++i) {
        workers_.emplace_back([this]() {
            this->worker();
        });
    }
}

ThreadPool::~ThreadPool() {
    task_queue_->cancel(); /*取消阻塞，唤醒所有线程*/
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join(); /*等待线程结束*/
        }
    }
}

/*生产者线程接口，即生产任务*/
void ThreadPool::post(std::function<void()> task) {
    task_queue_->push(task);
} /*post任务，异步执行，将任务放到任务队列*/

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        if (!task_queue_->pop(task)) {
            break; /*如果任务队列为空，退出线程*/
        }
        task(); /*执行任务*/
    }
}