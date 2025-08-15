#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include <functional>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue
{
public:
    BlockingQueue(bool nonblock = false) : nonblock_(nonblock) {}
    /*入队操作*/
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        not_empty_.notify_one(); /*唤醒一个等待线程*/
    } 

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_); /*unique_lock可以手动调用unlock*/
        // 1. mutex.unlock()
        // 2. queue_.empty() && !nonblock_ 线程在wait中阻塞 
        // 3. notify_one(), notify_all() 唤醒线程
        // 4. 满足条件,mutex.lock() 重新上锁
        // 5. 不满足条件，回到2继续休眠
        not_empty_.wait(lock, [this] { return !queue_.empty() || nonblock_;}); /*等待队列非空或非阻塞*/
        if (queue_.empty()) {
            return false;
        }
        value = queue_.front();
        queue_.pop();
        return true;
    }

    /*解除阻塞接口，用于唤醒休眠线程*/
    void cancel() {
        nonblock_ = true;
        not_empty_.notify_all(); /*唤醒所有等待线程*/
    } 

private:
    bool nonblock_;
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
};

template <typename T>
class SwapBlockingQueue
{
public:
    SwapBlockingQueue(bool nonblock = false) : nonblock_(nonblock) {}
        /*入队操作*/
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(prod_mutex_);
        prod_queue_.push(value);
        not_empty_.notify_one(); /*唤醒一个等待线程*/
    } 

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(cons_mutex_); /*unique_lock可以手动调用unlock*/
        if (cons_queue_.empty()) {
            if (0 == swap_queue()) {
                return false;
            }
        }
        value = cons_queue_.front();
        cons_queue_.pop();
        return true;

    }

    /*解除阻塞接口，用于唤醒休眠线程*/
    void cancel() {
        nonblock_ = true;
        not_empty_.notify_all(); /*唤醒所有等待线程*/
    } 

private:
    int swap_queue() {
        std::unique_lock<std::mutex> lock(prod_mutex_);
        not_empty_.wait(lock, [this] (){
            return !this->prod_queue_.empty() || this->nonblock_; /*等待生产队列非空或非阻塞*/
        });
        std::swap(prod_queue_, cons_queue_); /*交换生产和消费队列*/
        return cons_queue_.size(); /*返回消费队列的大小*/
    }
    bool nonblock_;
    std::queue<T> prod_queue_;
    std::queue<T> cons_queue_;
    std::mutex prod_mutex_;
    std::mutex cons_mutex_;
    std::condition_variable not_empty_;
};

class ThreadPool
{
public:
    explicit ThreadPool(int threads_num) {
        for (size_t i = 0; i < threads_num; ++i) {
            workers_.emplace_back([this]() {
                this->worker();
            });
        }
    }

    ~ThreadPool() {
        task_queue_.cancel(); /*取消阻塞，唤醒所有线程*/
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join(); /*等待线程结束*/
            }
        }
    }
    
    /*生产者线程接口，即生产任务*/
    void post(std::function<void()> task) {
        task_queue_.push(task);
    } /*post任务，异步执行，将任务放到任务队列*/

private:
    void worker() {
        while (true) {
            std::function<void()> task;
            if (!task_queue_.pop(task)) {
                break; /*如果任务队列为空，退出线程*/
            }
            task(); /*执行任务*/
        }
    }
    SwapBlockingQueue<std::function<void()>> task_queue_; /*阻塞队列，任务队列为空，阻塞线程*/
    std::vector<std::thread> workers_;
};

#endif

