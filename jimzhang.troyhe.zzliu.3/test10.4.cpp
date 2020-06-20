#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void parent(){
	char* p2 = (char*)vm_map(nullptr, 0);
	strcpy(p2, "data2.bin");
	char* content2[4];
	for(int i = 0; i < 4; i++){
		content2[i] = (char*)vm_map(p2, 3);
	}
	char* r2[2];
	for(int i = 0; i < 2; i++){
		r2[i] = (char*)vm_map(nullptr, 3);
	}
	for(int i = 0; i < 2; i++){
		*content2[i] = *r2[i];
	}
	for(int i = 0; i < 2; i++){
		*r2[i] = i;
	}
	for(int i = 2; i < 4; i++){
		*content2[i] = *r2[i - 2];  
	}
}

int main(){
    parent();
    return 0;
}  
     
      
	