#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using namespace std;

int main()
{   
    fork();
    vm_yield();
    fork();
    fork();
    vm_yield();
    fork();

    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename,  "shakespeare.txt");
    

    if (!fork()){
        char *p1 = (char*) vm_map(filename, 0);
        cout << (void *) p1 << endl;

        for (unsigned int i=0; i<100; i++) {
           cout << p1[i];
        }
        cout<<endl;

    } 
    return 0;
}   