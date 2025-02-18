#pragma once

#include <boost/asio.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <chrono>
#include <queue>
#include <tuple>

#include <optional>

class Task {
	public:
	 Task(unsigned int id, std::chrono::milliseconds duration,
	      std::mutex& coutMutex):m_CoutMutex(coutMutex), m_Id(id), m_Duration(duration){
	}		
	
	~Task(){
	}
	
	virtual void Execute();
	
	unsigned int GetId() const { return m_Id; }
	
	private:
	unsigned int m_Id = 0;
	std::chrono::milliseconds m_Duration;
	std::mutex& m_CoutMutex;
};

class TaskQueue {
    public:
    ~TaskQueue(){
	}

    // Thread safe functions for producers
    void PushTask(Task* t);
    void PushTasks(std::vector<Task*>& tasks);
    void StopQueue(){
		m_QueueIsStopped = true;
	}
    
    // Way for consumers to get the sync variables
    std::tuple<std::mutex&, std::condition_variable&> Subscribe(){
    	return std::tie(m_Mutex, m_ConditionVariable);
	}
    
    // Non-thread safe function. Consumers must ensure
    // lock acquisition
    bool HasPendingTask() const { return !m_Queue.empty(); }
    bool IsQueueStopped() const { return m_QueueIsStopped; }
    Task* GetNextTask(){
    	Task* t = m_Queue.front();
    	m_Queue.pop();
    	
    	return t;
	}
   
   private:
    bool m_QueueIsStopped = false;
    std::queue<Task*> m_Queue;
    std::mutex m_Mutex;
    std::condition_variable m_ConditionVariable;
 };

int startTaskQueueService();

