#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include "./head/threadpool.h"

void testFunc(void* arg)
{
    int num = *(int*)arg;
    std::string out="thread "+std::to_string(pthread_self())+" is working, number = "+std::to_string(num)+"\n";
    std::cout<<out;
    sleep(1);
}

int main(){
    threadPool pool(3,10);
    for (int i = 0; i < 121; i++)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i+100;

        task ttt;
        ttt.function=testFunc;
        ttt.arg=num;

        pool.addTask(ttt);
    }

    sleep(60);//假装在做别的事

    //printf("--------------------------------\n");
    //printf("%d\n",pool->livenum);
    //printf("%ld\n",pool->managerid);
    return 0;
}
