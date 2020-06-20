#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void func1(){
	void* p[16];
	for(size_t i = 0; i < 8; i++){
		p[i] = vm_map(nullptr, 0);
	}
	for(int i = 7; i >= 0; i--){
		*((int*)p[i]) = i * i + 1;
	}
	for(size_t i = 8; i < 16; i++){
		p[i] = vm_map(nullptr, 0);
	}
	for(int i = 8; i < 16; i++){
		*((int*)p[i]) = i * i - 1;
	}
	for(int i = 0; i < 8; i++){
		cout << *((int*)p[i]) << endl;
	}
	  
}

int main(){
	func1();  
	return 0;
}
