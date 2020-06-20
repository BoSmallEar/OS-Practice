#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void parent(){
	int* p  = (int*)vm_map(nullptr, 0);
	*p = (int)getpid();
	fork(); 
	vm_yield();
	fork();  
	vm_yield();
	*p = (int)getpid();  
	vm_yield();
	cout<< *p << endl;

}

int main(){
    parent();
    return 0;
}  
       
  
   