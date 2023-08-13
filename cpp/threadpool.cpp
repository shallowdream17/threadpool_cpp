#include <iostream>
//#include <pthread.h>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <thread>
#include <sstream>
#include <map>
#include <stack>
#include "../head/threadpool.h"


std::string threadPool::threadIdToString(const std::thread::id& id){
    std::ostringstream oss;
    oss<<id;
    return oss.str();
}

threadPool::threadPool(int minNum,int maxNum){
    //申请内存
    this->taskQue= new taskQueue;
    if(this->taskQue==nullptr){
        std::cout<<"Failed to apply for thread pool task queue memory...\n";
        exit(1);
    }
    //this->workerIds= new pthread_t[maxNum];
    this->workerIds= new std::thread[maxNum];
    if(this->workerIds==nullptr){
        std::cout<<"Worker thread memory application failed...\n";
        delete this->taskQue;
        this->taskQue=nullptr;
        exit(1);
    }
    //memset(this->workerIds,0,sizeof(std::mutex::id)*maxNum);

    //初始化锁和信号量
    /*
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
    */
    if(sem_init(&this->full,0,0)!=0){
        std::cout<<"sem init fail...\n";
        delete this->taskQue;
        for(int i=0;i<maxNum;i++){
            if(this->workerIds[i].joinable()){
                this->workerIds->join();
            }
        }
        delete[] this->workerIds;
        this->taskQue=nullptr;
        this->workerIds=nullptr;
        exit(1);
    }

    //初始化其他属性
    while(!this->stopStack.empty()){
        this->stopStack.pop();
    }
    Map.clear();
    this->minNum=minNum;
    this->maxNum=maxNum;
    this->busyNum=0;
    this->liveNum=minNum;
    this->exitNum=0;
    this->shutdown=false;

    //创建管理者线程和工作线程
    //pthread_create(&this->managerId,nullptr,this->managerFunc,this);
    this->managerId = std::thread(&threadPool::managerFunc,this);
    for(int i=0;i<minNum;i++){
        this->workerIds[i]=std::thread(&threadPool::workerFunc,this);
        Map[this->workerIds[i].get_id()]=i;
    }

}

threadPool::~threadPool(){
    std::cout<<"Thread pool destruction start...\n";
    this->shutdown=true;

    //pthread_join(this->managerId,nullptr);
    this->managerId.join();
    //pthread_mutex_lock(&this->mutexPool);
    std::unique_lock<std::mutex> lock(this->mutexPool);
    int livenum=this->liveNum;
    //pthread_mutex_unlock(&this->mutexPool);
    lock.unlock();
    for(int i=0;i<livenum;i++){
        sem_post(&this->full);
    }
    //std::cout<<"--------------------------------------\n";
    /*
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
    */

    for(int i=0;i<this->maxNum;i++){
        if(this->workerIds[i].joinable()){
            std::thread::id tid = this->workerIds[i].get_id();
            std::string out_str = "thread " + this->threadIdToString(tid)+" exiting...\n";
            std::cout<<out_str;
            this->workerIds[i].join();
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

    //pthread_mutex_destroy(&this->mutexPool);
    //pthread_mutex_destroy(&this->mutexBusy);
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
    //pthread_mutex_lock(&this->mutexPool);
    std::unique_lock<std::mutex> lock(this->mutexPool);
    int ans=this->liveNum;
    //pthread_mutex_unlock(&this->mutexPool);
    lock.unlock();
    return ans;
}

int threadPool::threadpoolbusynum(){
    //pthread_mutex_lock(&this->mutexBusy);
    std::unique_lock<std::mutex> lock(this->mutexPool);
    int ans=this->busyNum;
    //pthread_mutex_unlock(&this->mutexBusy);
    lock.unlock();
    return ans;
}

void threadPool::workerFunc(){
    //threadPool *pool = static_cast<threadPool*>(arg);
    while(true){
        sem_wait(&this->full);
        //pthread_mutex_lock(&this->mutexPool);
        std::unique_lock<std::mutex> lock(this->mutexPool,std::defer_lock);
        lock.lock();
        if(this->exitNum>0){
            //cout<<"first"<<endl;
            this->exitNum--;
            if(this->liveNum>this->minNum){
                this->liveNum--;
                //pthread_mutex_unlock(&pool->mutexPool);
                this->stopStack.push(Map[std::this_thread::get_id()]);
                lock.unlock();
                //std::cout<<<<std::endl;
                return;
                //pool->threadExit();
            }
            else{
                lock.unlock();
                return;
            }
        }
        if(this->shutdown){
            this->liveNum--;
            //pthread_mutex_unlock(&pool->mutexPool);
            lock.unlock();
            return;
            //pool->threadExit();
        }

        task nowTask=this->taskQue->getTask();

        //解锁
        //pthread_mutex_unlock(&pool->mutexPool);
        lock.unlock();

        std::thread::id tid=std::this_thread::get_id();
        std::string out="thread "+this->threadIdToString(tid)+" start working...\n";
        std::cout<<out;
        //pthread_mutex_lock(&pool->mutexBusy);
        //std::unique_lock<std::mutex> lock(this->mutexBusy);
        lock.lock();
        this->busyNum++;
        //pthread_mutex_unlock(&pool->mutexBusy);
        lock.unlock();
        nowTask.function(nowTask.arg);
        

        out="thread "+this->threadIdToString(tid)+" end working...\n";
        std::cout<<out;
        //pthread_mutex_lock(&pool->mutexBusy);
    
        //std::unique_lock<std::mutex> lock(this->mutexBusy);
        lock.lock();
        this->busyNum--;
        //pthread_mutex_unlock(&pool->mutexBusy);
        lock.unlock();
    }
    //return nullptr;
}

void threadPool::managerFunc(){
    //threadPool *pool= static_cast<threadPool*>(arg);
    while(!this->shutdown){
        sleep(3);//3s运行一次

        //pthread_mutex_lock(&pool->mutexPool);
        std::unique_lock<std::mutex> lock(this->mutexPool,std::defer_lock);
        lock.lock();
        int queueSize=this->taskQue->getTaskNum();
        int livenum=this->liveNum;//不要重名
        //pthread_mutex_unlock(&pool->mutexPool);
        lock.unlock();

        //pthread_mutex_lock(&pool->mutexBusy);
        //std::unique_lock<std::mutex> lock(this->mutexPool);
        lock.lock();
        int busynum=this->busyNum;//不要重名
        //pthread_mutex_unlock(&pool->mutexBusy);
        lock.unlock();

        //添加线程
        //任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if(queueSize>livenum && livenum< this->maxNum){
            //pthread_mutex_lock(&pool->mutexPool);
            //std::unique_lock<std::mutex> lock(this->mutexPool);
            lock.lock();
            int counter=0;
            for(int i=0;i<this->maxNum && counter<2 && this->liveNum<this->maxNum;i++){
                if(this->workerIds[i].joinable()==false){
                    //pthread_create(&this->workerIds[i],nullptr,this->workerFunc,this);
                    this->workerIds[i]=std::thread(&threadPool::workerFunc,this);
                    Map[this->workerIds[i].get_id()]=i;
                    std::cout<<"created thread...\n";
                    counter++;
                    this->liveNum++;
                }
            }
            //pthread_mutex_unlock(&this->mutexPool);
            lock.unlock();
        }

        //销毁线程
        //忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if(busynum * 2 <livenum && livenum > this->minNum){
            //std::cout<<"---------------------------------------"<<std::endl;
            //pthread_mutex_lock(&pool->mutexPool);
            //std::unique_lock<std::mutex> lock(this->mutexPool);
            lock.lock();
            int killNum=0;//记录要杀死的线程个数
            if(livenum-this->minNum>=2){
                this->exitNum=2;
                killNum=2;
            }
            else{
                killNum=livenum-this->minNum;
                this->exitNum=killNum;
            }
            //pthread_mutex_unlock(&pool->mutexPool);
            lock.unlock();
            for(int i=0;i<killNum;i++){
                sem_post(&this->full);//杀死几个就唤醒几个
            }
            //std::cout<<"-------------------------------"<<std::endl;
            while(killNum){//以后可以给stack上个单独的锁
                lock.lock();
                if(this->stopStack.size()!=0){
                    //std::cout<<this->stopStack.top()<<std::endl;
                    //std::cout<<"join is ok?"<<std::endl;
                    std::thread::id tid = this->workerIds[this->stopStack.top()].get_id();
                    std::string out_str="thread "+this->threadIdToString(tid)+" exiting...\n";
                    std::cout<<out_str;
                    this->workerIds[this->stopStack.top()].join();
                    this->stopStack.pop();
                    killNum--;
                }
                lock.unlock();
            }

        }
    }
    //return nullptr;
}

/*
void threadPool::threadExit(){
    //pthread_t tid= pthread_self();
    std::thread::id tid = std::this_thread::get_id();
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
*/
