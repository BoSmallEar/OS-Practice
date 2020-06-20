#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

cv cv1;
mutex m1;

void child(void* arg){
	m1.lock();
	while(true){
		cv1.wait(m1);
	}
	m1.unlock();
}

void parent(void* arg){
    thread* t = new thread(child, arg);
	  
    t->join();  
}  

int main() {
    cpu::boot(1, &parent,new int(0),false, false, 0);
    
    return 0;
}

    
