//
// Created by zzliu on 2/10/19.
//
#include "cpu.h"
#include "thread.h"
#include <ucontext.h>
#include <iostream>
#include <queue>
#include <vector>
#include <assert.h>
#include <stdexcept>

enum Status{FREE,BUSY};

std::queue<thread::impl*> readyQueue; 
std::queue<ucontext_t*> rubbish;
std::queue<cpu*> suspendCpus; 
int totalthreads = 0;

class cpu::impl{
public: 
    thread::impl* currentThread;
    ucontext_t* suspendContext;

    void suspend(){
        while(!rubbish.empty()) {
            ucontext_t* nextRubbish = rubbish.front();
            rubbish.pop();
            assert(nextRubbish);
            delete [] (char*) nextRubbish->uc_stack.ss_sp;
            delete nextRubbish; 
        }    

        cpu::self()->impl_ptr->currentThread = nullptr;
        suspendCpus.push(cpu::self());
        assert(guard);
        guard = false;
        cpu::self()->interrupt_enable_suspend(); 
    }
}; 

void wakeUpSuspendCpu(){
    if(!suspendCpus.empty() && !readyQueue.empty()){
        cpu* nextCpu = nullptr;
        size_t wakeNum = readyQueue.size();
        for(size_t i = 0; i < wakeNum; i++){
            nextCpu = suspendCpus.front();
            suspendCpus.pop();
            nextCpu->interrupt_send();
            if(suspendCpus.empty()) break;
        }
    }
}

class thread::impl{
public: 
    bool objectFinished;
    bool contextFinished;
    ucontext_t* context;
    int id;
    std::queue<thread::impl*> joinThread;
    thread_startfunc_t func;
    void* args;
    void clean(){ 
        rubbish.push(cpu::self()->impl_ptr->currentThread->context); 

        while (!cpu::self()->impl_ptr->currentThread->joinThread.empty()){ 
            readyQueue.push(cpu::self()->impl_ptr->currentThread->joinThread.front());
            cpu::self()->impl_ptr->currentThread->joinThread.pop(); 
        } 

        cpu::self()->impl_ptr->currentThread->contextFinished = true;
        if(cpu::self()->impl_ptr->currentThread->objectFinished){
            delete cpu::self()->impl_ptr->currentThread;
        }


        wakeUpSuspendCpu();

        while(rubbish.size()>1) {
            ucontext_t* nextRubbish = rubbish.front();
            rubbish.pop();
            assert(nextRubbish);
            delete [] (char*) nextRubbish->uc_stack.ss_sp;
            delete nextRubbish; 
        }    

        if(!readyQueue.empty()){
            cpu::self()->impl_ptr->currentThread = readyQueue.front();
            readyQueue.pop();
            assert(cpu::self()->impl_ptr->currentThread);
            setcontext(cpu::self()->impl_ptr->currentThread->context);
        }
        else{
            setcontext(cpu::self()->impl_ptr->suspendContext);
        }
    };
    void execute(){  
        thread_startfunc_t func = cpu::self()->impl_ptr->currentThread->func;
        void* args = cpu::self()->impl_ptr->currentThread->args; 

        assert(guard);
        guard = false;
        cpu::interrupt_enable();

        (*func)(args);

        cpu::interrupt_disable();
        while(guard.exchange(true)){}

        clean();
    };
};

void TIMER_handler(){ 
    cpu::interrupt_disable();
    while(guard.exchange(true)){}
    thread::impl* current = cpu::self()->impl_ptr->currentThread;
    if (current && !readyQueue.empty()){
        readyQueue.push(current);
        thread::impl* next = readyQueue.front();
        cpu::self()->impl_ptr->currentThread = next;
        readyQueue.pop(); 
        swapcontext(current->context,next->context);
    } 
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
}

void IPI_handler(){
    cpu::interrupt_disable(); 
    while(guard.exchange(true)){}
    if(!readyQueue.empty()){
        cpu::self()->impl_ptr->currentThread = readyQueue.front();
        readyQueue.pop();
        assert(cpu::self()->impl_ptr->currentThread);
        setcontext(cpu::self()->impl_ptr->currentThread->context);
    }
    else{
        setcontext(cpu::self()->impl_ptr->suspendContext);
    }
}


void cpu::init(thread_startfunc_t func, void * args) { 
    try{
        while(guard.exchange(true)){}
        impl_ptr = new impl();
        auto suspend_ptr = new ucontext(); 
        suspend_ptr->uc_stack.ss_sp = new char [STACK_SIZE];
        suspend_ptr->uc_stack.ss_size = STACK_SIZE;
        suspend_ptr->uc_stack.ss_flags = 0;
        suspend_ptr->uc_link = nullptr; 
        makecontext(suspend_ptr, (void(*)())&cpu::impl::suspend,0);
        cpu::self()->impl_ptr->suspendContext = suspend_ptr;
        interrupt_vector_table[TIMER] = (interrupt_handler_t)TIMER_handler;
        interrupt_vector_table[IPI] = (interrupt_handler_t)IPI_handler; 
        if (func!=nullptr) {
            cpu::self()->impl_ptr->currentThread = new thread::impl(); 
            auto ucontext_ptr = new ucontext();  
            ucontext_ptr->uc_stack.ss_sp = new char [STACK_SIZE];
            ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
            ucontext_ptr->uc_stack.ss_flags = 0;
            ucontext_ptr->uc_link = nullptr; 
            makecontext(ucontext_ptr, (void(*)())&thread::impl::execute,0);
            thread::impl* newThread = new thread::impl;
            newThread->context = ucontext_ptr;     
            newThread->func = func;  
            newThread->args = args;
            newThread->contextFinished = false;
            newThread->objectFinished = false;
            newThread->id = totalthreads;
            totalthreads++;
            readyQueue.push(newThread);
        } 
        if(!readyQueue.empty()){
            cpu::self()->impl_ptr->currentThread = readyQueue.front();
            readyQueue.pop();
            assert(cpu::self()->impl_ptr->currentThread);
            setcontext(cpu::self()->impl_ptr->currentThread->context);
        }
        else{
            setcontext(cpu::self()->impl_ptr->suspendContext); 
       }
    } 
    catch(std::bad_alloc& e){
        assert(guard);
        guard = false;
        cpu::interrupt_enable();
        throw e; 
    }
}
void thread::yield() {   
    cpu::interrupt_disable();
    while(guard.exchange(true)){}
    thread::impl* current = cpu::self()->impl_ptr->currentThread;
    if (current && !readyQueue.empty()){
        readyQueue.push(current);
        thread::impl* next = readyQueue.front();
        cpu::self()->impl_ptr->currentThread = next;
        readyQueue.pop(); 
        swapcontext(current->context,next->context);
    } 
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
}

void thread::join(){  
    cpu::interrupt_disable();
    while(guard.exchange(true)){}
    if (!impl_ptr->contextFinished){
        thread::impl* current = cpu::self()->impl_ptr->currentThread;
        impl_ptr->joinThread.push(current);
        if(!readyQueue.empty()){
            cpu::self()->impl_ptr->currentThread = readyQueue.front();
            readyQueue.pop();
            assert(cpu::self()->impl_ptr->currentThread);
            swapcontext(current->context, cpu::self()->impl_ptr->currentThread->context);
        }
        else{
            swapcontext(current->context,cpu::self()->impl_ptr->suspendContext);
        }
    } 
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
} 

thread::thread(thread_startfunc_t func, void * args){    
    try{
        cpu::interrupt_disable();
        while(guard.exchange(true)){}
        impl_ptr = new impl(); 
        impl_ptr->contextFinished = false;
        impl_ptr->objectFinished = false;
        auto ucontext_ptr = new ucontext();  
        ucontext_ptr->uc_stack.ss_sp = new char [STACK_SIZE];
        ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
        ucontext_ptr->uc_stack.ss_flags = 0;
        ucontext_ptr->uc_link = nullptr;
        makecontext(ucontext_ptr, (void(*)())&thread::impl::execute,0);
        impl_ptr->context = ucontext_ptr;
        impl_ptr->func = func;
        impl_ptr->args = args;
        impl_ptr->id = totalthreads;
        totalthreads++;
        readyQueue.push(impl_ptr);  
        wakeUpSuspendCpu();
        assert(guard);
        guard = false;
        cpu::interrupt_enable();
    }
    catch (std::bad_alloc& e){
        assert(guard);
        guard = false;
        cpu::interrupt_enable();
        throw e;
    }   
};

thread::~thread(){  
    cpu::interrupt_disable();
    while(guard.exchange(true)){}
    impl_ptr->objectFinished = true;
    if (impl_ptr->contextFinished){
        delete impl_ptr;
    }
    guard = false;
    cpu::interrupt_enable();
};

class mutex::impl{
    public:
        std::queue<thread::impl*> waitingQueue;
        Status status; 
        int threadId;
};

mutex::mutex(){ 
    try{
    impl_ptr = new mutex::impl();
    impl_ptr -> status = FREE; 
    impl_ptr->threadId = -1;
    }
    catch (std::bad_alloc& e){ 
        throw e; 
    }
}

void mutex::lock(){
    cpu::interrupt_disable();
    while(guard.exchange(true)){}
    if(impl_ptr -> status == FREE){
        impl_ptr -> status = BUSY;
        impl_ptr ->threadId = cpu::self()->impl_ptr->currentThread->id;
    }
    else{
        thread::impl* current = cpu::self()->impl_ptr->currentThread;
        impl_ptr->waitingQueue.push(current);
        if(!readyQueue.empty()){
            cpu::self()->impl_ptr->currentThread = readyQueue.front();
            readyQueue.pop();
            assert(cpu::self()->impl_ptr->currentThread);
            swapcontext(current->context,cpu::self()->impl_ptr->currentThread->context);
        }
        else{
            swapcontext(current->context,cpu::self()->impl_ptr->suspendContext);
        }
    }
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
}

void mutex::unlock(){
    cpu::interrupt_disable();
    while(guard.exchange(true)){} 

    if (cpu::self()->impl_ptr->currentThread->id != impl_ptr->threadId){
        std::runtime_error e("try to release a lock that the current thread is not holding!");
	    assert(guard);
        guard = false;
        cpu::interrupt_enable();
        throw e; 
    }
    impl_ptr->status = FREE;
    impl_ptr->threadId = -1;
    if(!impl_ptr -> waitingQueue.empty()){
        impl_ptr->threadId = impl_ptr -> waitingQueue.front()->id;
        readyQueue.push( impl_ptr -> waitingQueue.front());
        impl_ptr->waitingQueue.pop();
        impl_ptr->status = BUSY;
        wakeUpSuspendCpu();
    }
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
}
 
mutex::~mutex()
{
    delete impl_ptr;
}

class cv::impl{
    public:
        std::queue<thread::impl*> waitingQueue; 
};

cv::cv(){
    try{
      impl_ptr = new cv::impl();
    }
    catch (std::bad_alloc& e){ 
        throw e; 
    }
}

void cv::wait(mutex& m){
    cpu::interrupt_disable();
    while(guard.exchange(true)){}
    if (cpu::self()->impl_ptr->currentThread->id != m.impl_ptr->threadId){
        std::runtime_error e("try to release a lock that the current thread is not holding!");
	    assert(guard);
        guard = false;
        cpu::interrupt_enable();
        throw e; 
    }

    m.impl_ptr->status = FREE;
    m.impl_ptr -> threadId = -1;
    if(!m.impl_ptr -> waitingQueue.empty()){
        readyQueue.push(m.impl_ptr->waitingQueue.front());
        m.impl_ptr -> threadId = m.impl_ptr->waitingQueue.front()->id;
        m.impl_ptr -> waitingQueue.pop();
        m.impl_ptr->status = BUSY;

        wakeUpSuspendCpu();
    }

    impl_ptr->waitingQueue.push(cpu::self()->impl_ptr->currentThread);

    thread::impl* current = cpu::self()->impl_ptr->currentThread; 
    if(!readyQueue.empty()){
        cpu::self()->impl_ptr->currentThread = readyQueue.front();
        readyQueue.pop();
        assert(cpu::self()->impl_ptr->currentThread);
        swapcontext(current->context,cpu::self()->impl_ptr->currentThread->context);
    }
    else{
        swapcontext(current->context,cpu::self()->impl_ptr->suspendContext);
    }

    
    if(m.impl_ptr -> status == FREE){
        m.impl_ptr -> status = BUSY;
        m.impl_ptr -> threadId = cpu::self()->impl_ptr->currentThread->id;
    }
    else{
        current = cpu::self()->impl_ptr->currentThread;
        m.impl_ptr->waitingQueue.push(current);
        if(!readyQueue.empty()){
            cpu::self()->impl_ptr->currentThread = readyQueue.front();
            readyQueue.pop();
            assert(cpu::self()->impl_ptr->currentThread);
            swapcontext(current->context,cpu::self()->impl_ptr->currentThread->context);
        }
        else{
            swapcontext(current->context,cpu::self()->impl_ptr->suspendContext);
        }
    }
    assert(guard);   
    guard = false;
    cpu::interrupt_enable();
}

void cv::signal(){
    cpu::interrupt_disable();
    while(guard.exchange(true)){}

    if(!impl_ptr->waitingQueue.empty()){
        readyQueue.push(impl_ptr->waitingQueue.front()); 
        impl_ptr->waitingQueue.pop();
        wakeUpSuspendCpu();
    }
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
}

void cv::broadcast(){
    cpu::interrupt_disable();
    while(guard.exchange(true)){}

    while(!impl_ptr->waitingQueue.empty()){
        readyQueue.push(impl_ptr->waitingQueue.front());    
        impl_ptr->waitingQueue.pop();
        wakeUpSuspendCpu();
    }
    assert(guard);
    guard = false;
    cpu::interrupt_enable();
}

cv::~cv(){
    delete impl_ptr;
}
