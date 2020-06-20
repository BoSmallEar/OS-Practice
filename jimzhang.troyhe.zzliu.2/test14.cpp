#include <iostream>
#include <queue>
#include <stdexcept>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

mutex m;

std::queue<int> test;

void enqueue(int elt)
{
    try{
        m.unlock();
        test.push(elt);
        thread::yield();
    }
    catch(std::runtime_error){std::cout<<"FUCK\n";}
}

void popqueue()
{
    m.lock();
    test.pop();  
    m.unlock();
}

void child(void *arg){
    std::cout<<"child thread start running "<<*(int*)arg<<"\n"; 
    enqueue(1*(*(int*)arg));
    enqueue(10*(*(int*)arg));
    thread::yield();
    enqueue(100*(*(int*)arg));
    std::cout<<"child thread finishing "<<*(int*)arg<<"\n";
}

void parent(void* arg){
    std::cout<<"parent thread start running\n"; 
    thread t1 (child,new int(1));  
    thread t2 (child,new int(2));  
    t1.join();t2.join();
    std::cout<<"parent thread finishing\n"; 
    while(!test.empty())
    {
        std::cout<<test.front()<<"\n";
        test.pop();
    }
}

int main() {
    cpu::boot(1,&parent,new int(2),false,false,1);
    
    return 0;
}
