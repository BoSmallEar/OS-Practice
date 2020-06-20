#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

mutex m1;
cv cv1;

int a[5] = {1,2,3,4,5};
int b[5] = {10,20,30,40,50};
int c[5];
int dot;

void child(void *arg){
    int index = *(int*)arg;
    c[index] = a[index] + b[index];
	m1.lock();
	dot += a[index] * b[index];
	m1.unlock();
}
 
void parent(void* arg){
    thread* t[5];
    for(int i = 0; i < 5; i++)
	t[i] = new thread(child, new int(i));
    
    for(int i = 0; i < 5; i++)
	t[i]->join();
    for(int i = 0; i < 5; i++)
	delete t[i];
    
    for(int i = 0; i < 5; i++)
	cout<<c[i]<<" ";
    cout<<endl;
    cout<<dot<<endl;
}

int main() {
    cpu::boot(1,&parent,new int(0),false,false,13);
    
    return 0;
}

    
