#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

int main(){
	void* p[8];
	for(size_t i = 0; i < 8; i++){
		p[i] = vm_map(nullptr, 0);
	}
	for(int i = 7; i >= 0; i--){
		*((int*)p[i]) = i * i + 1;
	}
	pid_t id = fork();

	if(id == 0){
	for(int i = 0; i < 8; i++){
		*((int*)p[i]) = 2 * i;
	}}  
	else {
		for(int i = 0; i < 8; i++){
		*((int*)p[i]) = 2 * i + 1;
	}}

	for(int i = 0; i < 8; i++){
		cout << *((int*)p[i]) << endl;
	}
	return 0;
}    
  