#include <iostream>
#include <unistd.h>
//#include <pthread.h>
#include <thread>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "./head/threadpool.h"
std::string threadIdToString(const std::thread::id& id) {
    std::ostringstream oss;
    oss << id;
    return oss.str();
}



void testFunc(void* arg)
{
    int num = *(int*)arg;

    std::string out="thread "+threadIdToString(std::this_thread::get_id())+" is working, number = "+std::to_string(num)+"\n";
    std::cout<<out;
    sleep(1);
}

//测试
int main(){
    threadPool pool(3,10);
    for (int i = 0; i < 100; i++)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i+100;

        task ttt;
        ttt.function=testFunc;
        ttt.arg=num;

        pool.addTask(ttt);
    }

    sleep(30);//假装在做别的事

    return 0;
}
