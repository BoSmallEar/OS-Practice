 #include <iostream>
#include <queue>
#include <stdexcept>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

mutex m1;
mutex m2;
cv cv1;

void child(void *arg){
    try{
        cout<<"child start\n";
    m1.lock();
    m2.lock();
    cv1.wait(m1);
    cv1.wait(m2);
    m1.unlock();
    m2.unlock();
    cout<<"child end\n";

    }
    catch(std::runtime_error){cout<<"FUCK\n";}
}

void parent(void* arg){
    cout<<"parent start\n";
    thread* t = new thread(child, arg);
    t->join();
    cout<<"parent end\n";
}  

int main() {
    cpu::boot(1, &parent,new int(0),false, false, 12);
    
    return 0;
}

    
