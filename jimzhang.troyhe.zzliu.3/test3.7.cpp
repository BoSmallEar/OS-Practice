#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void* global_p;

void parent(){
	global_p = vm_map(nullptr, 111);
	printf("I'm parent! 0. %ld\n", (uint64_t)global_p);
	void* p[8];
	for(int i = 0; i < 8; i++){
		p[i] = vm_map(nullptr, 0);
	}
	for(int i = 7; i >= 0; i--){
		*((int*)p[i]) = i * i;
	}
	pid_t id;
	id = fork();
	if(id == 0){
		void* more = vm_map(nullptr, 2221);    
	}
	else{
		printf("I'm parent! 1. %ld\n", (uint64_t)global_p);
		vm_yield();
	}
	if(id == 0){
		printf("I'm child! 2. %ld\n", (uint64_t)global_p);
	}
	else{
		printf("I'm parent! 2. %ld\n", (uint64_t)global_p);
	}
}

int main(){  
	parent();
	return 0;
}
