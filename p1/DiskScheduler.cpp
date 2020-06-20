#include <iostream>
#include "thread.h" 
#include "disk.h"
#include <fstream>
#include <queue>
#include <map>
#include <vector>
using std::cout;
using std::endl;
 
std::vector<std::queue<int>> data;
 
int maxDiskQueue;
int aliveRequests;
int currentPos = 0;
int threadcount = 0;
int pendingcount = 0; 
std::map<int, int> scheduler;
mutex threadcountlock;
mutex requestlock;
mutex requestCounterLock;
cv requestcv;
cv servicecv;
void request(void* arg){
    
    int requestId = *(int *) arg; 
    requestlock.lock();
    while (!data[requestId].empty()){
        while (pendingcount >= std::min(threadcount,maxDiskQueue) || scheduler[requestId]!=-1 ){
            requestcv.wait(requestlock);
        }
        pendingcount++;
        int pos = data[requestId].front();
        data[requestId].pop();
        print_request(requestId,pos);
        scheduler[requestId] = pos;
        servicecv.signal(); 
    } 
    while (scheduler[requestId] != -1){
            requestcv.wait(requestlock);
    }
    threadcount--;
    servicecv.signal();
    requestlock.unlock();
};

void service(void* arg){
    requestlock.lock();
    while (threadcount>0){
        while (pendingcount<std::min(threadcount,maxDiskQueue)){
            servicecv.wait(requestlock);
        }
        int minD = 2000;
        int minI = 0;
        for (auto w:scheduler){
            if (w.second!=-1 && std::abs(currentPos-w.second)<minD){
                minD = abs(currentPos-w.second);
                minI = w.first;
            }
        }
        if (scheduler[minI]!=-1){
            print_service(minI,scheduler[minI]);
        }
        currentPos = scheduler[minI];
        scheduler[minI] = -1;
        pendingcount--;
        requestcv.broadcast();
    }
    requestlock.unlock();
};

void mainthread(void* arg)
{
    int argc = *(int*)arg;
    thread se((thread_startfunc_t) service, (void *) 100);
    for (int i=0; i<argc; i++){
        thread re((thread_startfunc_t) request, new int(i)); 
    }
}  

int main(int argc, char* argv[]){ 
    maxDiskQueue = atoi(argv[1]);
    data.resize(argc-2);   
    for (int i=2; i<argc; i++){
        std::ifstream ff;
        ff.open(argv[i]); 
        int pos;  
        while (ff>>pos){
            data[i-2].push(pos);
        }
    }
    threadcount = argc-2;
    for (int i=0;i<threadcount;i++){
        scheduler[i] = -1;
    }
    cpu::boot((thread_startfunc_t) mainthread, new int (argc-2), 1);
}