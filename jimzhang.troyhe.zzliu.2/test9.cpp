#include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

thread* haha[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

void grandgrandchild(void* arg){
	cout<<"grandgrandchild "<<*(int*)arg<<endl;

}

void grandchild(void* arg){
	cout<<"grandchild "<<*(int*)arg<<endl;
	haha[*(int*)arg+4] = new thread(grandgrandchild, arg);
	if(haha[5]&&1!=*(int*)arg) haha[5]->join();
	if(haha[6]&&2!=*(int*)arg) haha[6]->join();
	haha[*(int*)arg+4]->join();
}
 
void child(void* arg){
	cout<<"child "<<*(int*)arg<<endl;
	haha[*(int*)arg] = new thread(grandchild, arg);
	for(int i =0;i<4;i++) if(haha[i]&&i!=*(int*)arg) haha[i]->join();
	if(haha[4]&&0!=*(int*)arg) haha[4]->join();
	if(haha[7]&&3!=*(int*)arg) haha[7]->join();
	haha[*(int*)arg]->join();
	
}
 
void parent(void* arg){
    thread* t1 = new thread(child, new int(0));
    thread* t2 = new thread(child, new int(1));
	thread* t3 = new thread(child, new int(2));
	thread* t4 = new thread(child, new int(3));
	t3->join(); t4->join(); t1->join();t2->join();
}  

int main() {
    cpu::boot(1, &parent,new int(0),false, false, 0);
    
    return 0;
}

    
