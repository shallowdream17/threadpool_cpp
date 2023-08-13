#include <iostream>
#include "../head/taskqueue.h"

taskQueue::taskQueue(){
    //pthread_mutex_init(&this->mutex,nullptr);
}

taskQueue::~taskQueue(){
    //pthread_mutex_destroy(&this->mutex);
}

void taskQueue::addTask(const task& newTask){
    //pthread_mutex_lock(&this->mutex);
    std::unique_lock<std::mutex> lock(task_mutex);
    this->que.push(newTask);
    //pthread_mutex_unlock(&this->mutex);
    lock.unlock();
}

void taskQueue::addTask(void (*function)(void* arg),void *arg){
    task newTask;
    newTask.function=function;
    newTask.arg=arg;
    //pthread_mutex_lock(&this->mutex);
    std::unique_lock<std::mutex> lock(task_mutex);
    this->que.push(newTask);
    //pthread_mutex_unlock(&this->mutex);
    lock.unlock();
}

task taskQueue::getTask(){//经过P(full)后才会调用该函数
    //pthread_mutex_lock(&this->mutex);
    std::unique_lock<std::mutex> lock(task_mutex);
    task newTask = this->que.front();
    this->que.pop();
    //pthread_mutex_unlock(&this->mutex);
    lock.unlock();
    return newTask;
}


