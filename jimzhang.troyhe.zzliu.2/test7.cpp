#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

void child(void* arg){
	int id = *(int*)arg;
	if(id <= 100000){
		thread* t = new thread(child, new int(id * 2));
		cout<<id<<endl;
		if(1000 < id && id < 10000) thread::yield();
	}
}

void parent(void* arg){
    thread* t1 = new thread(child, new int(1));
    thread* t2 = new thread(child, new int(2));
    t1->join();t2->join();
}  
 
int main() {
    cpu::boot(1, &parent,new int(0),false, false, 100);
    
    return 0;  
}

    
