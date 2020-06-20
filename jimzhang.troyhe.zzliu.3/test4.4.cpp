#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void parent(){
	void* p = vm_map(nullptr, 111);
	*((int*)p) = 1;
	cout << (uint64_t)p << " " << *((int*)p) << endl;
	pid_t id = fork();
	if(id == 0){
		*((int*)p) = 2;
		cout << (uint64_t)p << " " << *((int*)p) << endl;
	}
	else{
		*((int*)p) = 3;
		cout << (uint64_t)p << " " << *((int*)p) << endl;
	}
	*((int*)p) = 4;
	cout << (uint64_t)p << " " << *((int*)p) << endl;
}

int main(){
	parent();
	return 0;
}      