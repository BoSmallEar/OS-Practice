#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

void child(void* arg){
	return;
}

void parent(void* arg){
	for(int i = 0; i < 1000000; i++){
		try{thread t ((thread_startfunc_t) child, (void *) &i);}
		catch(std::bad_alloc){cout<<"FUCK"<<endl;exit(0);}
	}
}

int main() {
    cpu::boot(1, &parent,new int(0),false, false, 0);
    
    return 0;
}

    
