 #include <iostream>
#include <queue>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using namespace std;

mutex m1;
cv cv1;
cv cv2;
int cola = 0;
bool died[2] = {false,false};  

void producer(void *arg){
	for(int i = 0; i < 10; i++){  
		m1.lock();  
		while(cola >= 10){
			cv1.wait(m1);
		}
		cola++;
		cout<<"producer "<<*(int*)arg<<" produces 1, now cola has "<<cola<<endl;
		if(i == 9) died[*(int*)arg]=true;
		cv2.signal();
		m1.unlock();
	}
}

void consumer(void *arg){
	while(!died[0] || !died[1] || cola > 0){
		m1.lock();
		while(cola <= 0){
			cv2.wait(m1);
		}
		cola--;
		cout<<"consumer "<<*(int*)arg<<" consumes 1, now cola has "<<cola<<endl;
		cv1.broadcast();
		m1.unlock();
	}
}

void parent(void* arg){
    thread* t[7];
	t[0] = new thread(producer, new int(0));
	t[1] = new thread(producer, new int(1));
	
	t[2] = new thread(consumer, new int(0));
	t[3] = new thread(consumer, new int(1));
	t[4] = new thread(consumer, new int(2));
	t[5] = new thread(producer, new int(1));
	t[6] = new thread(producer, new int(1));
    
    for(int i = 6; i >= 3; i--)
		t[i]->join();
 for(int i = 0; i < 3; i++)
		t[i]->join();
    for(int i = 0; i < 5; i++)
		delete t[i];
}  

int main() {
    cpu::boot(4, &parent,new int(0),false, false, 12);
    
    return 0;
}

    
