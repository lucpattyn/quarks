//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, synchronous
// File: https://www.boost.org/doc/libs/1_66_0/libs/beast/example/websocket/server/sync/websocket_server_sync.cpp
//
//------------------------------------------------------------------------------

#include <quarkstaskqueue.hpp>

#include <random>


void Task::Execute() {
    std::this_thread::sleep_for(m_Duration);
    {
        std::scoped_lock<std::mutex> guard(m_CoutMutex);
        std::cout << "Task {" << m_Id << "} finished in " << m_Duration.count()
                  << "ms.\n";
    }
 }
 

 
void TaskQueue::PushTask(Task* t) {
 	{
    	std::scoped_lock l{m_Mutex};
     	m_Queue.push(t);
 	}
 
 	m_ConditionVariable.notify_one();
}
 
void TaskQueue::PushTasks(std::vector<Task*>& tasks) {
	{
	    std::scoped_lock l{m_Mutex};
	    for (Task* t : tasks) {
	        m_Queue.push(t);
	    }
	}
	m_ConditionVariable.notify_all();
}
 
void WorkerThread(const int workerId, TaskQueue& taskQueue,
                  std::mutex& coutMutex) {
    auto [m, cv] = taskQueue.Subscribe();
    
     while (true) {
        auto data = [&]() -> std::optional<Task*> {
            std::unique_lock l{m};
            cv.wait(l, [&] {
                return taskQueue.IsQueueStopped() || taskQueue.HasPendingTask();
            });
            if (taskQueue.IsQueueStopped()) {
                return {};
            } else {
                Task* taskToProcess = taskQueue.GetNextTask();
                assert(taskToProcess != nullptr);
                return taskToProcess;
            }
        }();
        
		if (!data) {

            break;
        }

        Task* taskPtr = *data;
        {
            std::scoped_lock<std::mutex> guard(coutMutex);
            std::cout << "Worker {" << workerId << "} is executing task {"
                      << taskPtr->GetId() << "}.\n";
        }

        // process the data
        taskPtr->Execute();
        delete taskPtr;
    }
}

int startTaskQueueService() {
	
    const unsigned int thread_pool_size =
        std::thread::hardware_concurrency() - 1;
    assert(thread_pool_size > 0);

    std::mutex coutMutex;

    std::vector<std::thread> thread_pool(thread_pool_size);
    TaskQueue taskQueue;

    for (size_t i = 0; i < thread_pool_size; ++i) {
        thread_pool[i] = std::thread(WorkerThread, i, std::ref(taskQueue),
                                     std::ref(coutMutex));
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> taskDuration(0, 1000);

    for (int i = 0; i < 5; ++i) {
        taskQueue.PushTask(
            new Task(i, std::chrono::milliseconds(taskDuration(gen)),
                     std::ref(coutMutex)));
    }

    std::vector<Task*> taskBatch;
    taskBatch.resize(5);
    for (int i = 0; i < 5; ++i) {
        taskBatch[i] =
            new Task(i + 5, std::chrono::milliseconds(taskDuration(gen)),
                     std::ref(coutMutex));
    }
    taskQueue.PushTasks(taskBatch);
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    taskQueue.StopQueue();

    for (size_t i = 0; i < thread_pool_size; ++i) {
        thread_pool[i].join();
    }
    return 0;


}
