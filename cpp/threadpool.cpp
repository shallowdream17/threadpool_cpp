#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include "../head/threadpool.h"

threadPool::threadPool(int minNum,int maxNum){
    //申请内存
    this->taskQue= new taskQueue;
    if(this->taskQue==nullptr){
        std::cout<<"Failed to apply for thread pool task queue memory...\n";
        exit(1);
    }
    this->workerIds= new pthread_t[maxNum];
    if(this->workerIds==nullptr){
        std::cout<<"Worker thread id memory application failed...\n";
        delete this->taskQue;
        this->taskQue=nullptr;
        exit(1);
    }
    memset(this->workerIds,0,sizeof(pthread_t)*maxNum);

    //初始化锁和信号量
    if(pthread_mutex_init(&this->mutexPool,nullptr)!=0 ||
            pthread_mutex_init(&this->mutexBusy,nullptr)!=0 ||
            sem_init(&this->full,0,0)!=0){
        std::cout<<"mutex or sem init fail...\n";
        delete this->taskQue;
        delete[] this->workerIds;
        this->taskQue=nullptr;
        this->workerIds=nullptr;
        exit(1);
    }

    //初始化其他属性
    this->minNum=minNum;
    this->maxNum=maxNum;
    this->busyNum=0;
    this->liveNum=minNum;
    this->exitNum=0;
    this->shutdown=false;

    //创建管理者线程和工作线程
    pthread_create(&this->managerId,nullptr,this->managerFunc,this);
    for(int i=0;i<minNum;i++){
        pthread_create(&this->workerIds[i],nullptr,this->workerFunc,this);
    }

}

threadPool::~threadPool(){
    std::cout<<"Thread pool destruction start...\n";
    this->shutdown=true;

    pthread_join(this->managerId,nullptr);
    pthread_mutex_lock(&this->mutexPool);
    int livenum=this->liveNum;
    pthread_mutex_unlock(&this->mutexPool);
    for(int i=0;i<livenum;i++){
        sem_post(&this->full);
    }
    //std::cout<<"--------------------------------------\n";
    while(true){
        pthread_mutex_lock(&this->mutexPool);
        int res=this->liveNum;
        pthread_mutex_unlock(&this->mutexPool);
        if(res>0){
            sleep(1);
        }
        else{
            break;
        }
    }

    if(this->taskQue!=nullptr){
        delete this->taskQue;
        this->taskQue=nullptr;
    }
    if(this->workerIds!=nullptr){
        delete[] this->workerIds;
        this->workerIds=nullptr;
    }

    pthread_mutex_destroy(&this->mutexPool);
    pthread_mutex_destroy(&this->mutexBusy);
    sem_destroy(&this->full);


}

void threadPool::addTask(const task& newTask){
    if(this->shutdown){
        return;
    }
    this->taskQue->addTask(newTask);
    sem_post(&this->full);
}

int threadPool::threadpoollivenum(){
    pthread_mutex_lock(&this->mutexPool);
    int ans=this->liveNum;
    pthread_mutex_unlock(&this->mutexPool);
    return ans;
}

int threadPool::threadpoolbusynum(){
    pthread_mutex_lock(&this->mutexBusy);
    int ans=this->busyNum;
    pthread_mutex_unlock(&this->mutexBusy);
    return ans;
}

void* threadPool::workerFunc(void *arg){
    threadPool *pool = static_cast<threadPool*>(arg);
    while(true){
        sem_wait(&pool->full);
        pthread_mutex_lock(&pool->mutexPool);
        if(pool->exitNum>0){
            pool->exitNum--;
            if(pool->liveNum>pool->minNum){
                pool->liveNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                pool->threadExit();
            }
        }
        if(pool->shutdown){
            pool->liveNum--;
            pthread_mutex_unlock(&pool->mutexPool);
            pool->threadExit();
        }

        task nowTask=pool->taskQue->getTask();

        //解锁
        pthread_mutex_unlock(&pool->mutexPool);

        std::string out="thread "+std::to_string(pthread_self())+" start working...\n";
        std::cout<<out;
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        nowTask.function(nowTask.arg);

        out="thread "+std::to_string(pthread_self())+" end working...\n";
        std::cout<<out;
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return nullptr;
}

void *threadPool::managerFunc(void *arg){
    threadPool *pool= static_cast<threadPool*>(arg);
    while(!pool->shutdown){
        sleep(3);//3s运行一次

        pthread_mutex_lock(&pool->mutexPool);
        int queueSize=pool->taskQue->getTaskNum();
        int livenum=pool->liveNum;//不要重名
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        int busynum=pool->busyNum;//不要重名
        pthread_mutex_unlock(&pool->mutexBusy);

        //添加线程
        //任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if(queueSize>livenum && livenum< pool->maxNum){
            pthread_mutex_lock(&pool->mutexPool);
            int counter=0;
            for(int i=0;i<pool->maxNum && counter<2 && pool->liveNum<pool->maxNum;i++){
                if(pool->workerIds[i]==0){
                    pthread_create(&pool->workerIds[i],nullptr,pool->workerFunc,pool);
                    std::cout<<"created thread...\n";
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //销毁线程
        //忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if(busynum * 2 <livenum && livenum > pool->minNum){
            pthread_mutex_lock(&pool->mutexPool);
            int killNum=0;//记录要杀死的线程个数
            if(livenum-pool->minNum>=2){
                pool->exitNum=2;
                killNum=2;
            }
            else{
                killNum=livenum-pool->minNum;
                pool->exitNum=killNum;
            }
            pthread_mutex_unlock(&pool->mutexPool);
            for(int i=0;i<killNum;i++){
                sem_post(&pool->full);//杀死几个就唤醒几个
            }
        }
    }
    return nullptr;
}

void threadPool::threadExit(){
    pthread_t tid= pthread_self();
    for(int i=0;i<this->maxNum;i++){
        if(this->workerIds[i]==tid){
            this->workerIds[i]=0;
            std::string out="threadExit() called, "+std::to_string(tid)+" exiting...\n";
            std::cout<<out;
            break;
        }
    }
    pthread_exit(nullptr);
}
