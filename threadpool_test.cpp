#include "threadpool.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

// 测试1：基本功能测试，执行简单任务
void test_basic_functionality() {
    std::cout << "=== 测试基本功能 ===" << std::endl;
    ThreadPool pool(2); // 创建2个线程的线程池
    
    pool.post([]() {
        std::cout << "任务1执行中，线程ID: " << std::this_thread::get_id() << std::endl;
    });
    
    pool.post([]() {
        std::cout << "任务2执行中，线程ID: " << std::this_thread::get_id() << std::endl;
    });
    
    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << std::endl;
}

// 测试2：并发任务执行，验证线程池负载均衡
void test_concurrent_tasks() {
    std::cout << "=== 测试并发任务 ===" << std::endl;
    const int TASK_COUNT = 5;
    ThreadPool pool(2);
    std::atomic<int> completed_tasks(0);
    
    for (int i = 0; i < TASK_COUNT; ++i) {
        pool.post([i, &completed_tasks]() {
            std::cout << "任务" << i << "开始执行，线程ID: " << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 模拟耗时操作
            std::cout << "任务" << i << "执行完成" << std::endl;
            completed_tasks++;
        });
    }
    
    // 等待所有任务完成
    while (completed_tasks < TASK_COUNT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
}

// 测试3：验证任务参数传递和返回值（通过捕获外部变量）
void test_task_parameters() {
    std::cout << "=== 测试任务参数传递 ===" << std::endl;
    ThreadPool pool(1);
    std::atomic<int> sum(0);
    const int COUNT = 1000;
    
    for (int i = 1; i <= COUNT; ++i) {
        pool.post([i, &sum]() {
            sum += i; // 累加1到1000
        });
    }
    
    // 等待计算完成
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "1到" << COUNT << "的和为: " << sum << "（预期值: " << COUNT*(COUNT+1)/2 << "）" << std::endl;
    std::cout << std::endl;
}

// 测试4：验证线程池析构时的资源释放
void test_threadpool_destruction() {
    std::cout << "=== 测试线程池析构 ===" << std::endl;
    std::atomic<bool> task_executed(false);
    
    {
        ThreadPool pool(1);
        pool.post([&task_executed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            task_executed = true;
            std::cout << "析构前的任务执行完成" << std::endl;
        });
        // 离开作用域时线程池会被析构
    }
    
    std::cout << "线程池已析构，任务是否执行: " << (task_executed ? "是" : "否") << std::endl;
    std::cout << std::endl;
}

// 测试5：测试大量小任务的处理能力
void test_massive_tasks() {
    std::cout << "=== 测试大量小任务 ===" << std::endl;
    const int TASK_COUNT = 10000;
    ThreadPool pool(4);
    std::atomic<int> counter(0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < TASK_COUNT; ++i) {
        pool.post([&counter]() {
            counter++; // 简单的计数操作
        });
    }
    
    // 等待所有任务完成
    while (counter < TASK_COUNT) {
        std::this_thread::yield();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "完成" << TASK_COUNT << "个任务，耗时: " << duration.count() << "ms" << std::endl;
    std::cout << std::endl;
}

int main() {
    test_basic_functionality();
    test_concurrent_tasks();
    test_task_parameters();
    test_threadpool_destruction();
    test_massive_tasks();
    
    std::cout << "所有测试完成!" << std::endl;
    return 0;
}
