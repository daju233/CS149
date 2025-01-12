#include "tasksys.h"
#include <thread>
#include <atomic>
#include <functional>
#include <iostream>
IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char *TaskSystemSerial::name()
{
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads) : ITaskSystem(num_threads)
{
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable *runnable, int num_total_tasks)
{
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                          const std::vector<TaskID> &deps)
{
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync()
{
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelSpawn::name()
{
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads) : ITaskSystem(num_threads)
{
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->thread_max = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn()
{
}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    std::atomic<int> task_id(0);

    for (int i = 0; i < thread_max; i++)
    {
        std::thread t = std::thread([runnable, &task_id, num_total_tasks]()
                                    {for(;;){
                                        int my_task_id = task_id.fetch_add(1);
                                        if(my_task_id>=num_total_tasks)return;
                                        runnable->runTask(my_task_id,num_total_tasks);
                                    } });
        threads.push_back(std::move(t));
    }
    for (auto &thread : this->threads)
    {
        thread.join();
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                 const std::vector<TaskID> &deps)
{
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync()
{
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSpinning::name()
{
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads) : ITaskSystem(num_threads)
{
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    for (int i = 0; i < num_threads; i++)
    {
        workers.emplace_back(std::thread([this]()
                                         {
            while(!isstop){
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                if(tasks.empty()){
                    lock.unlock();
                    std::this_thread::yield();
                    continue;
                }
                task = std::move(tasks.front());
                tasks.pop();
                this->count_total_tasks.fetch_sub(1);
                if(count_total_tasks<=0)isstop=true;
                lock.unlock();
            }
            task();

            } }));
    }
}
// std::cout<<tasks.size()<<"\n";
// std::cout<<std::this_thread::get_id()<<"\n";
// std::cout<<count_total_tasks<<"\t";//为什么打印了4次？？？ 破案了 run了三次

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        isstop = true;
    }

    for (auto &thread : workers)
    {
        thread.join();
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    {
        std::lock_guard<std::mutex> lock(this->queue_mutex);
        this->count_total_tasks = num_total_tasks;
        for (int i = 0; i < num_total_tasks; i++)
        {
            tasks.push([i, num_total_tasks, runnable]()
                       { runnable->runTask(i, num_total_tasks); });
        }
    }
    while (this->count_total_tasks > 0)
    {
        std::this_thread::yield(); // 让出 CPU 时间片，减少自旋等待
    }

    // while(count_total_tasks>0)std::this_thread::yield();//为什么这样不行？？
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps)
{
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync()
{
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSleeping::name()
{
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads) : ITaskSystem(num_threads)
{
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    for (int i = 0; i < num_threads; i++)
    {
        workers.emplace_back(std::thread([this]()
                                         {
            while(1){
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(lock,[this]{
                    return !this->tasks.empty()||this->isstop;
                });
                if(this->isstop && this->tasks.empty())//线程池停止且为空则线程返回
                    return;
                task = std::move(tasks.front());
                tasks.pop();
                count_total_tasks--;
                lock.unlock();
            }
            task();
            {
                std::unique_lock<std::mutex> finish_lock(this->finish_mutex);
                if(count_total_tasks==0){
                    finish_condition.notify_one();
                }
            }
            } }));
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping()
{
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        isstop = true;
    }
    condition.notify_all();
    for (auto &thread : workers)
    {
        thread.join();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    //
    // std::cout<<"我喜欢你"<<"\t";
    {
        this->count_total_tasks = num_total_tasks;
        for (int i = 0; i < num_total_tasks; i++)
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            tasks.push([i, num_total_tasks, runnable]()
                       { runnable->runTask(i, num_total_tasks); });
            this->condition.notify_one();
        }
        {
            std::unique_lock<std::mutex> finish_lock(this->finish_mutex);

            this->finish_condition.wait(finish_lock, [this]()
                                        { return count_total_tasks == 0; });
        }
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps)
{

    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync()
{

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
