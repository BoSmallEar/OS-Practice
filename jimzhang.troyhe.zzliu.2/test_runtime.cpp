#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

cv cv1;
mutex m1, m2;

void child(void* arg){
	/*while(true){
		cv1.wait(m1);
	}*/
	
	try{
		cv1.wait(m1);

	}
	catch(std::runtime_error){cout<<"FUCK\n";}
}

void parent(void* arg){
    thread* t = new thread(child, arg);
	  
    t->join();
}  

int main() {
    cpu::boot(1, &parent,new int(0),false, false, 0);
    
    return 0;
}

    
