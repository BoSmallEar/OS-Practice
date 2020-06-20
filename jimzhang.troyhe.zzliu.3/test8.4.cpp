#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void* p;
void* q;

void parent(){   
	p = vm_map(nullptr, 0);
	q = vm_map(nullptr, 0);
	*((int*)p) = 1111;
	*((int*)q) = 1111111;
	pid_t id;
	id = fork();
	if(id != 0){
		vm_yield();
	}
	else{
		p = vm_map(nullptr, 111);
		*((int*)p) = 2222;
		*((int*)q) = 2222222;
	}  
}

int main(){
    parent();     
    return 0;
}  
    
  