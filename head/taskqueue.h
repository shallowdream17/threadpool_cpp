#pragma once
#include <queue>
//#include <pthread.h>
#include <mutex>

struct task{
    void (*function)(void *arg);
    void *arg;
};

class taskQueue{
public:
    taskQueue();
    ~taskQueue();

    //添加任务
    void addTask(const task& newTask);
    void addTask(void (*function)(void* arg),void *art);

    //从任务队列中取出任务
    task getTask();

    //获取任务队列中的任务个数
    inline int getTaskNum(){
        return this->que.size();//是否需要锁？
    }
private:
    std::queue<task> que;
    //pthread_mutex_t mutex;
    std::mutex task_mutex;
};
