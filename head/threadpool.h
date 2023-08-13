#pragma once
#include "taskqueue.h"
//#include <pthread.h>
#include <thread>
#include <mutex>
#include <stack>
#include <string>
#include <map>
#include <sstream>
#include <semaphore.h>

class threadPool{
public:
    threadPool(){};//待修改
    threadPool(int minNum,int maxNum);
    ~threadPool();
    void addTask(const task& newTask);//添加任务
    int threadpoolbusynum();//获取线程池中工作的线程个数
    int threadpoollivenum();//获取线程池中存活的线程个数
    std::string threadIdToString(const std::thread::id& id);//将thread::id转换为字符串


private:
    //pthread_t managerId;//管理者线程(只有一个)
    //pthread_t *workerIds;//工作线程
    std::thread managerId;//管理者线程
    std::thread *workerIds;//工作线程
    int minNum;//最小线程数量
    int maxNum;//最大线程数量
    int busyNum;//忙碌线程数量
    int liveNum;//存活的线程数量
    int exitNum;//退出线程数量
    //pthread_mutex_t mutexPool;//锁下整个线程池
    //pthread_mutex_t mutexBusy;//锁下忙碌的线程数量
    std::mutex mutexPool;//锁下整个线程池
    std::mutex mutexBusy;//锁下忙碌的线程数量
    sem_t full;//任务队列中的任务个数

    std::stack<int> stopStack;//记录运行过程中要回收的子线程
    std::map<std::thread::id,int> Map;//记录线程id
    

    bool shutdown;//是否要销毁线程池

    taskQueue *taskQue;//任务队列

private:
    void workerFunc();//工作者线程回调函数
    void managerFunc();//管理者线程回调函数
    void threadExit();//单个线程退出
};
