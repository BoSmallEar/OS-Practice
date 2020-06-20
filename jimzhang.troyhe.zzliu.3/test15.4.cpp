#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

int main()
{
    char* p2 = (char*)vm_map(nullptr, 0);
    fork();
	strcpy(p2, "data1.bin");
	char* content2[2];
	for(int i = 0; i < 2; i++){
		content2[i] = (char*)vm_map(p2, i);
	}

	fork();
	
	char* p1 = (char*)vm_map(p2, 1);
	strcpy(p1, "data2.bin");
	char* content1[2];  
	for(int i = 0; i < 2; i++){
		content1[i] = (char*)vm_map(p1, i);
	}

	for(int i = 0; i < 2; i++){
		*((char*)content1[i]) = 'A' + i;
		*((char*)content2[i]) = 'a' + i;
	}

	char* p3[2];
	for(size_t i = 0; i < 2; i++){
	 	p3[i] = (char*)vm_map(nullptr, 11111);
	}

	fork();

	*((char*)p3[0]) = *((char*)content1[2]);
	*((char*)content2[0]) = *((char*)p3[0]);
	*((char*)p3[1]) = *((char*)content1[3]);   
	*((char*)content2[3]) = *((char*)p3[0]);
    fork();
	*((char*)content1[1]) = *((char*)p3[1]);
	*((char*)p3[1]) = *((char*)content2[3]);
     
}


