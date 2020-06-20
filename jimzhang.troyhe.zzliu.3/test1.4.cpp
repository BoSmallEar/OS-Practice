#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

int main(){
	void* p[8];
	for(size_t i = 0; i < 3; i++){
		p[i] = vm_map(nullptr, 0);
	}

	*((char*)p[0]) = 'a';
	*((char*)p[1]) = 'b';
	*((char*)p[2]) = 'c';
	
	// p 0 1 2 | 0 1 2
	p[3] = vm_map(nullptr, 111);
	p[4] = vm_map(nullptr, 111);

	*((char*)p[3]) = 'd';
	// p 3 1 2 | 0 1 2 3  
	char buf1 = *((char*)p[1]);

	*((char*)p[4]) = 'e';
	// p 3 1 4 | 0 1 2 3 4
	for(size_t i = 5; i < 8; i++){
		p[i] = vm_map(nullptr, 0);
	}
	// p 3 1 4 | 0 1 2 3 4 5 6 7
	*((char*)p[1]) = 'b';

	buf1 = *((char*)p[5]) + *((char*)p[7]) ;
	// p 3 1 4 | 0 1 2 3 4 5 6 7

	*((char*)p[6]) = 'g';
	// p 6 1 4 | 0 1 2 3 4 5 6 7

	buf1 = *((char*)p[5]) + *((char*)p[0]);
	// p 6 1 0 | 0 1 2 3 4 5 6 7

	*((char*)p[3]) = 'd';
	// p 6 3 0 | 0 1 2 3 4 5 6 7

	*((char*)p[7]) = 'h';
	// p 7 3 0 | 0 1 2 3 4 5 6 7
	
	return 0; 
}     
