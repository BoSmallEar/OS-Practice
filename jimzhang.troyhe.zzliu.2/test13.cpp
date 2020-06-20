#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;
//multi-join
mutex m1;
cv cv1;

int a[10] = {1,2,3,4,5,6,7,8,9,10};
int b[10] = {10,20,30,40,50,60,70,80,90,100};
int c[10];
int dot;
thread* t[10];

void child(void *arg){
    int index = *(int*)arg;
    c[index] = a[index] + b[index];
	m1.lock();
	dot += a[index] * b[index];
	m1.unlock();
    if (index <= 4){
        for(int i = 7; i < 10; i++) t[i]->join();
    }
    
}

void selfjoin(void *arg){
    t[0]->join();
}
 
void parent(void* arg){
    t[0] = new thread(selfjoin, new int(0));
    for(int i = 1; i < 2; i++) t[i] = new thread(child, new int(i));
    
    for(int i = 1; i < 10; i++) t[i]->join();
    for(int i = 0; i < 10; i++) delete t[i];
    
    
}

int main() {
    cpu::boot(1,&parent,new int(0),false,false,13);
    
    return 0;
}