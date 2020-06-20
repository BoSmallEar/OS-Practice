 #include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

mutex m1, m2;

void child(void *arg){
    cout<<"child start\n";
    m1.lock();
	m2.lock();
	cout<<"child end\n";
}  

void parent(void* arg){
cout<<"parent start\n";
    thread* t[2];
t[0] = new thread(child, arg);
t[1] = new thread(child, arg);
	t[0]->join();t[1]->join();
}  

int main() {
    cpu::boot(1, &parent,new int(0),false, false, 12);
    
    return 0;
}

    
