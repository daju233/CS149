#include "tasksys.h"
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
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
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync()
{
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks)
{
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                 const std::vector<TaskID> &deps)
{
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync()
{
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks)
{
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps)
{
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync()
{
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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
                    return !this->vaildTasks.empty()||this->isstop;
                });
                if(this->isstop && this->vaildTasks.empty())//线程池停止且为空则线程返回
                    break;//return;
                task = std::move(vaildTasks.front());
                vaildTasks.pop();
                count_total_tasks--;
                std::cout<<count_total_tasks<<"\t";
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
        if(thread.joinable())thread.join();
    }
    for(auto& pair : taskContexts){
        delete pair.second;
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    {
        this->count_total_tasks = num_total_tasks;
        for (int i = 0; i < num_total_tasks; i++)
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            vaildTasks.push([i, num_total_tasks, runnable]()
                            { runnable->runTask(i, num_total_tasks); });
            condition.notify_one();
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
        int cur_task_id = this->taskID.fetch_add(1);
        TaskContext *task_context = new TaskContext(cur_task_id);


    {
        // 在另一个线程中执行任务 TaskSystemParallelThreadPoolSleeping::run 函数
        std::thread([this, runnable, num_total_tasks, task_context, &deps]()
                    {
                    // 先等待依赖任务完成
                for (auto dep : deps) {
                    TaskContext* task_context = this->taskContexts[dep];
                    std::unique_lock<std::mutex> lock(task_context->mutex);
                    task_context->cv.wait(lock, [&task_context]() { return task_context->is_finished; });
                }

                this->run(runnable, num_total_tasks);
                {
                    std::lock_guard<std::mutex> lock(task_context->mutex);
                    task_context->is_finished = true;
                }
                task_context->cv.notify_all(); })
            .detach();

        this->taskContexts[cur_task_id] = task_context;

        // std::unique_lock<std::mutex> lock(this->queue_mutex);
        // taskID++;
        // finish_tasks.insert(std::pair<TaskID, bool>(taskID, false));
        // for (auto depTaskID : deps)
        // {
        //     std::unique_lock<std::mutex> deplock(this->dep_mutex);
        //     auto issolved = finish_tasks[depTaskID];
        //     std::cout<<issolved;
        //     dep_condition.wait(deplock, [issolved]
        //                        { return issolved; });
        // }
        // this->run(runnable, num_total_tasks);

        // 这些可以不用写，只是记录依赖和异步，其余逻辑用run
        // You can assume all programs will either call only run() or only runAsyncWithDeps();
        // that is, you do not need to handle the case where a run() call needs to wait for all proceeding calls to runAsyncWithDeps() to finish.
        //  Note that this assumption means you can implement run() using appropriate calls to runAsyncWithDeps() and sync().
        // 您可以假设所有程序将仅调用run()或仅调用runAsyncWithDeps() ；也就是说，您不需要处理run()调用需要等待所有正在进行的runAsyncWithDeps()调用完成的情况。
        // 请注意，此假设意味着您可以使用对runAsyncWithDeps()和sync()的适当调用来实现run() 。
        //  for (int i = 0; i < num_total_tasks; i++)
        //  {
        //      if (deps.empty())
        //      {
        //          vaildTasks.push([num_total_tasks, runnable, this]()
        //                          { runnable->runTask(taskID, num_total_tasks);
        //                           });
        //          this->condition.notify_one();
        //      }
        //  }
    }
    return cur_task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync()
{

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    for (const auto& taskContext : this->taskContexts) {
        TaskContext* task_context = taskContext.second;

        std::unique_lock<std::mutex> lock(task_context->mutex);
        task_context->cv.wait(lock, [task_context]() { return task_context->is_finished; });
    }

    return;
}

