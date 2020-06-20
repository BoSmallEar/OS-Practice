#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void* p;

void parent(){
	*((int*)p) = getpid();
	cout << (uint64_t)p << " " << *((int*)p) << endl;
	fork(); 
	vm_yield();
	fork();
	fork();
	vm_yield();
	fork();
	*((int*)p) = getpid();     
	cout << (uint64_t)p << " " << *((int*)p) << endl;
	
}

int main(){
	p = vm_map(nullptr, 111);
	parent();
	return 0;
}    
   
  