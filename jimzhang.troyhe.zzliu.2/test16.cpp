#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

mutex m1, m2;
cv cv1;



void Tom(void *arg){
    m1.lock();
    m2.lock();

    m2.unlock();
    m1.unlock();
}
 
void Jerry(void* arg){
    m2.lock();
    m1.lock();

    m1.unlock();
    m2.unlock();
    
    
}

void parent(void* arg){
    thread* t[2];
    t[0] = new thread(Tom, arg);
    t[1] = new thread(Jerry, arg);
    for (int i = 0; i < 30 ; i++){
        t[0]->join();
        t[1]->join();
        delete t[0];
        delete t[1];
        t[0] = nullptr;
        t[0] = new thread(Tom, arg);
        t[1] = nullptr;
        t[1] = new thread(Jerry, arg);
    }

	t[0]->join();
    t[1]->join();
    delete t[0];        
    delete t[1];
} 

int main() {
    cpu::boot(1,&parent,new int(0),false,false,13);
    
    return 0;
}
