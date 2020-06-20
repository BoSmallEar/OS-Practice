#include "vm_pager.h"
#include <unordered_map>
#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <memory>
#include <unordered_set>
#include <cstring>

size_t memoryPages;    
size_t swapBlocks; 
std::vector<bool> swapSpaceAvailable;
std::vector<bool> memorySpaceAvailable;
size_t swapSpaceLeft;
size_t memoryLeft;
uint8_t copyOnWriteBuffer[4096]; 

class PageInfo{
    public:
    bool referenced;
    bool dirty;
    bool residence;
    bool swapback; 
    std::string fileName;
    size_t block;
    size_t diskLoc;
    size_t physmemLoc;
    std::unordered_map<pid_t,std::vector<size_t>> processes; 
    PageInfo():referenced(true),dirty(false),residence(true){
    }
};

class ProcessInfo{
    public:
    pid_t pid;
    page_table_t* page_table;
    size_t totalValidPages;
    std::vector<std::shared_ptr<PageInfo>> pageinfos;
    ProcessInfo(pid_t pid){
        this->pid = pid;
        this->page_table = new page_table_t();
        this->totalValidPages = 0;
    }
};

std::vector<std::string> usedfiles;
std::vector<std::unordered_map<size_t, std::shared_ptr<PageInfo> > > usedBlocks;
std::unordered_map<pid_t,std::shared_ptr<ProcessInfo>> processInfos;
std::shared_ptr<ProcessInfo> currentProcess;
std::list<std::shared_ptr<PageInfo>> clock_queue;
std::shared_ptr<PageInfo> pinnedPage;

size_t evictAndSwap(){
    while (clock_queue.front()->referenced)
    {
        clock_queue.push_back(clock_queue.front());
        clock_queue.front()->referenced = false;
        for (auto it = clock_queue.front()->processes.begin();it!=clock_queue.front()->processes.end(); it++){
            for (auto vp_id:it->second){
                processInfos[it->first]->page_table->ptes[vp_id].read_enable = 0;
                processInfos[it->first]->page_table->ptes[vp_id].write_enable = 0;  
            } 
        }
        clock_queue.pop_front();
    }

    auto ptrEvictPage = clock_queue.front();
    clock_queue.pop_front(); 
    ptrEvictPage->residence = false;
    if (ptrEvictPage->dirty) {
        ptrEvictPage->dirty = false;
        if (ptrEvictPage->swapback){
            file_write(nullptr, ptrEvictPage->diskLoc, (void*)((size_t)vm_physmem+(ptrEvictPage->physmemLoc<<12)));
        }
        else{
            file_write(ptrEvictPage->fileName.c_str(), ptrEvictPage->block, (void*)((size_t)vm_physmem+(ptrEvictPage->physmemLoc<<12)));
        }
    }
    return ptrEvictPage->physmemLoc;
} 

extern void vm_init(size_t memory_pages, size_t swap_blocks){
    memoryPages = memory_pages;
    swapBlocks = swap_blocks; 
    memorySpaceAvailable = std::vector<bool>(memory_pages,true);
    swapSpaceAvailable = std::vector<bool>(swap_blocks,true);
    swapSpaceLeft = swapBlocks;
    memoryLeft = memory_pages - 1;

    //set up pinned memory
    memorySpaceAvailable[0] = false;
    for(size_t i = 0; i < 4096; i++){
        ((char*)vm_physmem)[i] = 0;
    }

    pinnedPage = std::make_shared<PageInfo>();
    pinnedPage->dirty = false;
    pinnedPage->referenced = true; // irrelevant
    pinnedPage->physmemLoc = 0;
    pinnedPage->diskLoc = 0; // irrelevant
    pinnedPage->swapback = true; // ?
    pinnedPage->residence = true;
    pinnedPage->block = 0; // irrelevant
}

extern int vm_create(pid_t parent_pid, pid_t child_pid){
    auto parent_it = processInfos.find(parent_pid); 
    if (parent_it != processInfos.end() ){
        size_t swapSpaceRequired = 0;
        for(auto it : parent_it->second->pageinfos)
        {
            swapSpaceRequired += it->swapback;
        }
        if (swapSpaceRequired>swapSpaceLeft){
            return -1;
        }
        swapSpaceLeft -= swapSpaceRequired;
    }
    auto childProcess = std::make_shared<ProcessInfo>(child_pid);
    if(parent_it != processInfos.end()){ 
        auto parentProcess = parent_it->second;
        childProcess->totalValidPages = parentProcess->totalValidPages;
        childProcess->page_table = new page_table_t();
        childProcess->pageinfos = parentProcess->pageinfos;
        std::copy(std::begin(parentProcess->page_table->ptes),std::end(parentProcess->page_table->ptes),childProcess->page_table->ptes); 
        processInfos[child_pid] = childProcess;
        for (auto pageinfo : childProcess->pageinfos){ 
            pageinfo->processes[child_pid] = pageinfo->processes[parent_pid];
            if (pageinfo->swapback){
                for (auto it = pageinfo->processes.begin();it!=pageinfo->processes.end();it++){ 
                        for (auto vp_id:it->second){ 
                            processInfos[it->first]->page_table->ptes[vp_id].write_enable = 0;  
                        } 
                }
            }
        }
    }
    else{
        processInfos[child_pid] = childProcess;
    }
    return 0;
}

extern void vm_switch(pid_t pid){
    currentProcess = processInfos[pid]; 
    page_table_base_register = currentProcess->page_table;
}

extern int vm_fault(const void* addr, bool write_flag){
    if ((uint64_t)addr<(uint64_t)VM_ARENA_BASEADDR+(uint64_t)VM_ARENA_SIZE&&(uint64_t)addr>=(uint64_t)VM_ARENA_BASEADDR){
        size_t faultAddr = ((uint64_t)addr - (uint64_t)VM_ARENA_BASEADDR) >> 12;
        if (faultAddr<currentProcess->totalValidPages){
            auto currentPage = currentProcess->pageinfos[faultAddr];
            if (write_flag&&(currentPage->processes.size()>1 || currentPage->physmemLoc == 0)&&currentPage->swapback){
                if (!currentPage->residence){
                    currentPage->residence = true; 
                    size_t memLoc;
                    if (memoryLeft == 0){
                        memLoc = evictAndSwap();
                    }
                    else{
                        for (memLoc = 0; memLoc<memoryPages;memLoc++){
                            if (memorySpaceAvailable[memLoc]) break;
                        }
                        memoryLeft--; 
                        memorySpaceAvailable[memLoc] = false;
                    }
                    clock_queue.push_back(currentPage);
                    currentPage->physmemLoc = memLoc;
                    file_read(nullptr,currentPage->diskLoc,(void*)((size_t)vm_physmem+(memLoc<<12)));
                    for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){
                        for (auto vp_id:it->second){
                            processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;  
                            processInfos[it->first]->page_table->ptes[vp_id].write_enable = 0; 
                            processInfos[it->first]->page_table->ptes[vp_id].ppage = memLoc;
                        } 
                    }
                }
                else{ 
                    if (currentPage->processes.size()==2){
                        for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){
                            for (auto vp_id:it->second){
                                processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;
                                processInfos[it->first]->page_table->ptes[vp_id].write_enable = currentPage->dirty;  
                            } 
                        } 
                    }
                    else{
                         for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){
                            for (auto vp_id:it->second){
                                processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;
                                processInfos[it->first]->page_table->ptes[vp_id].write_enable = 0;  
                            } 
                        } 
                    }
                }
  
                currentPage->referenced = true;
                size_t virtual_loc = faultAddr;
                
                for(auto it = currentPage->processes[currentProcess->pid].begin();it != currentPage->processes[currentProcess->pid].end();it++){
                    if (*it == faultAddr){
                         currentPage->processes[currentProcess->pid].erase(it);
                         if ( currentPage->processes[currentProcess->pid].empty()) currentPage->processes.erase(currentProcess->pid);
                         break;
                    }
                }

                std::copy((char*)((size_t)vm_physmem+(currentPage->physmemLoc<<12)),(char*)((size_t)vm_physmem+((currentPage->physmemLoc+1)<<12)),copyOnWriteBuffer);
                std::shared_ptr<PageInfo> newPage = std::make_shared<PageInfo>();
                currentProcess->pageinfos[virtual_loc] = newPage;
                newPage->referenced = true;
                newPage->swapback = true;
                newPage->dirty = true;
                newPage->processes[currentProcess->pid].push_back(virtual_loc);
                size_t memLoc;
             
                if (memoryLeft == 0){
                    memLoc = evictAndSwap();
                }
                else{
                    for (memLoc = 1; memLoc<memoryPages;memLoc++){
                        if (memorySpaceAvailable[memLoc]) break;
                    }
                    memoryLeft--; 
                    memorySpaceAvailable[memLoc] = false;
                } 
                 
                clock_queue.push_back(newPage); 
                newPage->physmemLoc = memLoc; 
                std::copy(copyOnWriteBuffer,copyOnWriteBuffer+4096,(char*)((size_t)vm_physmem+(memLoc<<12))); 
                currentProcess->page_table->ptes[virtual_loc].read_enable = 1;
                currentProcess->page_table->ptes[virtual_loc].write_enable = 1;
                currentProcess->page_table->ptes[virtual_loc].ppage = memLoc;
 
                size_t diskLoc;
                for (diskLoc = 0; diskLoc<swapBlocks; diskLoc++){
                    if (swapSpaceAvailable[diskLoc]) break;
                }
                swapSpaceAvailable[diskLoc] = false; 
            
                newPage->diskLoc = diskLoc;
            
            }
            else{
                if (!currentPage->residence){
                    currentPage->residence = true;
                    size_t memLoc;
                    if (memoryLeft == 0){
                        memLoc = evictAndSwap();
                    }
                    else{
                        for (memLoc = 0; memLoc<memoryPages;memLoc++){
                            if (memorySpaceAvailable[memLoc]) break;
                        }
                        memoryLeft--; 
                        memorySpaceAvailable[memLoc] = false;
                    }
                    clock_queue.push_back(currentPage); 
                    currentPage->physmemLoc = memLoc;

 
                    if (currentPage->swapback){
                        file_read(nullptr,currentPage->diskLoc,(void*)((size_t)vm_physmem+(memLoc<<12)));
                    }
                    else{ 
                        int readRes = file_read(currentPage->fileName.c_str(),currentPage->block,(void*)((size_t)vm_physmem+(memLoc<<12)));
                         if(readRes == -1){
                            memoryLeft++;
                            memorySpaceAvailable[memLoc] = true;
                            return -1;
                        }
                    }

                   
                    for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){ 
                        for (auto vp_id:it->second){
                            processInfos[it->first]->page_table->ptes[vp_id].ppage = memLoc;  
                            processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;  
                            processInfos[it->first]->page_table->ptes[vp_id].write_enable = write_flag;
                        } 
                    }
                    
                }
                else{ 
		    
                    for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){ 
                        for (auto vp_id:it->second){
                            processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;  
			    bool writeConstraint =  (write_flag || currentPage->dirty);
			    if (currentPage->processes.size()>1 && currentPage->swapback){
                        processInfos[it->first]->page_table->ptes[vp_id].write_enable = 0;
			    }
			    else{
			    }

                        } 
                    }
                }
               
                if(write_flag){
                    currentPage->dirty = true; 
                }
                currentPage->referenced = true;
            } 
            return 0;
        }
        return -1;
    }
    return -1;
}

extern void vm_destroy(){
    for (auto it = clock_queue.begin(); it != clock_queue.end(); it++){ 
        if ((*it)->processes.find(currentProcess->pid) != (*it)->processes.end()){
            (*it)->processes.erase(currentProcess->pid);
            if ((*it)->swapback) swapSpaceLeft ++;
            if ((*it)->processes.empty() && ((*it)->swapback)){
                memorySpaceAvailable[(*it)->physmemLoc] = true; 
                swapSpaceAvailable[(*it)->diskLoc] = true; 
                memoryLeft++;
                clock_queue.erase(it);
                it--;
            }
            else if ((*it)->processes.size() == 1){
                auto lastProcess = (*it)->processes.begin();
                for (auto vp_id: lastProcess->second){  
                        processInfos[lastProcess->first]->page_table->ptes[vp_id].write_enable = (*it)->dirty && (*it)->referenced;  
                } 
            }
        }
    }

    if(pinnedPage->processes.find(currentProcess->pid) != pinnedPage->processes.end())
    {
        swapSpaceLeft+=pinnedPage->processes[currentProcess->pid].size();
        pinnedPage->processes.erase(currentProcess->pid);
    }

    for (auto pageinfo : currentProcess->pageinfos){
        pageinfo->processes.erase(currentProcess->pid);
        if (!pageinfo->residence){             
            if (pageinfo->swapback){
                if (pageinfo->processes.empty()){
                    swapSpaceAvailable[pageinfo->diskLoc] = true; 
                }
                swapSpaceLeft++;
            } 
        }
    }
    
    processInfos.erase(processInfos.find(currentProcess->pid));
    
    delete currentProcess->page_table;
    currentProcess = nullptr; 
}

extern void *vm_map(const char *filename, size_t block){
    if (swapSpaceLeft>0 || filename!=nullptr)
    {
        if(filename != nullptr){
            size_t fileNameAddr = ((uint64_t)filename - (uint64_t)VM_ARENA_BASEADDR) >> 12;
            auto currentPage = currentProcess->pageinfos[fileNameAddr];
            
            if (!currentPage->residence){
                size_t memLoc;
                if (memoryLeft == 0){
                    memLoc = evictAndSwap();
                }
                else{
                    for (memLoc = 0; memLoc<memoryPages;memLoc++){
                        if (memorySpaceAvailable[memLoc]) break;
                    }
                    memoryLeft--; 
                    memorySpaceAvailable[memLoc] = false;
                }

                void* physMemLoc = (void*)((size_t)vm_physmem+(memLoc<<12));
                if(currentPage->swapback){
                    file_read(nullptr, currentPage->diskLoc, physMemLoc);
                }
                else{
                    file_read(currentPage->fileName.c_str(), currentPage->block, physMemLoc);
                }

                currentPage->residence = true;
                clock_queue.push_back(currentPage); 
                currentPage->physmemLoc = memLoc;
                
                for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){ 
                    for (auto vp_id:it->second){
                        processInfos[it->first]->page_table->ptes[vp_id].ppage = memLoc;  
                        processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;  
                        processInfos[it->first]->page_table->ptes[vp_id].write_enable = 0;
                    } 
                }
            }
            else{ 
                for (auto it = currentPage->processes.begin();it!=currentPage->processes.end();it++){ 
                    for (auto vp_id:it->second){
                        processInfos[it->first]->page_table->ptes[vp_id].read_enable = 1;  
                        processInfos[it->first]->page_table->ptes[vp_id].write_enable = currentPage->dirty;
                    } 
                }
            }

            std::string physFileName((const char*)((size_t)vm_physmem + (currentPage->physmemLoc << 12)));
            currentPage->referenced = true;
            bool shared = false;
            bool nameshared = false;
            size_t i; 
            for (i=0; i<usedfiles.size();i++){
                if (usedfiles[i] == physFileName){
                    nameshared = true;
                    shared = usedBlocks[i].find(block) != usedBlocks[i].end();
                    break;
                }
            } 
            if (!shared){
                std::shared_ptr<PageInfo> newPage = std::make_shared<PageInfo>();

                newPage->fileName = physFileName;
                newPage->block = block;
                newPage->swapback = false;
                if (!nameshared){
                    usedfiles.push_back(physFileName); 
                    usedBlocks.emplace_back();
                }
                usedBlocks[i][block] = newPage;

                newPage->processes[currentProcess->pid].push_back(currentProcess->totalValidPages);
    
                currentProcess->pageinfos.push_back(newPage);
                newPage->residence = false;  
                currentProcess->page_table->ptes[currentProcess->totalValidPages].read_enable = 0;
                currentProcess->page_table->ptes[currentProcess->totalValidPages].write_enable = 0;
                currentProcess->totalValidPages++;         
            }
            else{ 
                std::shared_ptr<PageInfo>  oldPage = usedBlocks[i][block];
                size_t old_read_enable;
                size_t old_write_enable;
                for (auto it = oldPage->processes.begin();it!=oldPage->processes.end();it++){ 
                    for (auto vp_id:it->second){
                        old_read_enable = processInfos[it->first]->page_table->ptes[vp_id].read_enable;  
                        old_write_enable = processInfos[it->first]->page_table->ptes[vp_id].write_enable; 
                        break;
                    } 
                    break;
                }

                currentProcess->pageinfos.push_back(oldPage);
                oldPage->processes[currentProcess->pid].push_back(currentProcess->totalValidPages); 
                currentProcess->page_table->ptes[currentProcess->totalValidPages].ppage = oldPage->physmemLoc;
                currentProcess->page_table->ptes[currentProcess->totalValidPages].read_enable = old_read_enable;
                currentProcess->page_table->ptes[currentProcess->totalValidPages].write_enable = old_write_enable;
                currentProcess->totalValidPages++;
            }
        }
        else {
            pinnedPage->processes[currentProcess->pid].push_back(currentProcess->totalValidPages);

            currentProcess->pageinfos.push_back(pinnedPage);
            currentProcess->page_table->ptes[currentProcess->totalValidPages].ppage = 0;
            currentProcess->page_table->ptes[currentProcess->totalValidPages].read_enable = 1;
            currentProcess->page_table->ptes[currentProcess->totalValidPages].write_enable = 0;
            currentProcess->totalValidPages++;

            swapSpaceLeft--;
        }            
        return (void*)((size_t)VM_ARENA_BASEADDR+((currentProcess->totalValidPages-1)<<12));
    }
    else return nullptr; 
}


