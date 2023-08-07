#pragma once
#include "taskqueue.h"
#include <pthread.h>
#include <semaphore.h>

class threadPool{
public:
    threadPool(){};//待修改
    threadPool(int minNum,int maxNum);
    ~threadPool();
    void addTask(const task& newTask);//添加任务
    int threadpoolbusynum();//获取线程池中工作的线程个数
    int threadpoollivenum();//获取线程池中存活的线程个数


private:
    pthread_t managerId;//管理者线程(只有一个)
    pthread_t *workerIds;//工作线程
    int minNum;//最小线程数量
    int maxNum;//最大线程数量
    int busyNum;//忙碌线程数量
    int liveNum;//存活的线程数量
    int exitNum;//退出线程数量
    pthread_mutex_t mutexPool;//锁下整个线程池
    pthread_mutex_t mutexBusy;//锁下忙碌的线程数量
    sem_t full;//任务队列中的任务个数

    bool shutdown;//是否要销毁线程池

    taskQueue *taskQue;//任务队列

private:
    static void *workerFunc(void *arg);//工作者线程回调函数
    static void *managerFunc(void *arg);//管理者线程回调函数
    void threadExit();//单个线程退出
};
