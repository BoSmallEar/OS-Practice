#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
using namespace std;

int main(){
    char *filename1 = (char *) vm_map(nullptr, 0); 

    strcpy(filename1, "shakespeare.txt"); 
    char *p1 = (char *) vm_map (filename1, 0);
    for (unsigned int i=0; i<100; i++) {
	    cout << p1[i];
    }
    cout<<endl;
    char *filename2 = (char *) vm_map(nullptr, 123); 
    strcpy(filename2, "data1.bin"); 
    char *p2 = (char *) vm_map (filename2, 0);
    for (unsigned int i=0; i<100; i++) {
	    cout << p2[i];
    }
    cout<<endl;
	
	char *filename3 = (char *) vm_map(nullptr, 123); 
    strcpy(filename3, "fuck.bin"); 
    char *p3 = (char *) vm_map (filename3, 0);
    cout<<(p3==nullptr)<<endl;

    return 0;
}  
       
