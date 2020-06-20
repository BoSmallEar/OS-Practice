#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

void parent(){
	char* p2 = (char*)vm_map(nullptr, 0);
	strcpy(p2, "data1.bin");
	char* content2[4];
	for(int i = 0; i < 4; i++){
		content2[i] = (char*)vm_map(p2, i);
	}
	
	char* p1 = (char*)vm_map(p2, 1);
	strcpy(p1, "data2.bin");
	char* content1[4];
	for(int i = 0; i < 4; i++){
		content1[i] = (char*)vm_map(p1, i);
	}

	for(int i = 0; i < 4; i++){
		*((char*)content1[i]) = 'A' + i;
		*((char*)content2[i]) = 'a' + i;
	}

	char* p3[2];
	for(size_t i = 0; i < 2; i++){
	 	p3[i] = (char*)vm_map(nullptr, 11111);
	}

	*((char*)p3[0]) = *((char*)content1[2]);
	*((char*)content2[0]) = *((char*)p3[0]);
	*((char*)p3[1]) = *((char*)content1[3]);
	*((char*)content2[3]) = *((char*)p3[0]);
	*((char*)content1[1]) = *((char*)p3[1]);
	*((char*)p3[1]) = *((char*)content2[3]);

}

int main(){
    parent();
    return 0;
}  
       
  
   