#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <cmath>
#include "thread.h"

using namespace std;

int max_request;
int num_requesters;
int active_requesters;
int active_services;
int current_track = 0;
vector< vector<int> > requesters;
vector<size_t> req_pointer;
vector< pair<int, int> > disk_queue;
mutex queue_mutex;
cv queue_cv;

bool cmp(pair<int, int>& a, pair<int, int>& b)
{
	return abs(a.second - current_track) > abs(b.second - current_track);
}  

void request_func(void *req_pos)
{
	int *pos = (int*)req_pos;
	for(size_t i = 0; i < requesters[*pos].size(); i++)
	{
		queue_mutex.lock();
		while(i > req_pointer[*pos]) //wait until the last request is serviced
		{
			//cout << "requester " << *pos << " waiting" << endl;
			queue_cv.wait(queue_mutex);
		}
		while((int)disk_queue.size() >= max_request) //wait until disk is not full
		{
			//cout << "requester " << *pos << " waiting" << endl;
			queue_cv.wait(queue_mutex);
		}
		disk_queue.emplace_back(*pos, requesters[*pos][i]);
		cout<<"request"<<*pos<<" "<<requesters[*pos][i]<<endl;
		
		//printf("queue: ");for(auto iii:disk_queue)printf("%d ",iii.first);printf("\n");
		queue_cv.broadcast();
		queue_mutex.unlock();
	}
}

void service_func(void *a)
{
	while(active_requesters > 0)
	{
		queue_mutex.lock();
		//printf("%d active\n",active_requesters);
		while((int)disk_queue.size() < active_services)
		{
			//cout << "service waiting" << endl;
			queue_cv.wait(queue_mutex);
		}
		
		sort(disk_queue.begin(), disk_queue.end(), cmp);
		current_track = disk_queue.back().second;
		req_pointer[disk_queue.back().first]++;
		
		if(req_pointer[disk_queue.back().first] == requesters[disk_queue.back().first].size())
		{
			active_requesters--;
		}
		
		active_services = (max_request > active_requesters) ? active_requesters : max_request;
		
		cout<<"service "<<disk_queue.back().first<<" "<<current_track<<endl;
		disk_queue.pop_back();
		
		//printf("queue: ");for(auto iii:disk_queue)printf("%d ",iii.first);printf("\n");
		//printf("pointer: ");for(auto iii:req_pointer)printf("%d ",iii);printf("\n");
		queue_cv.broadcast();
		queue_mutex.unlock();
	}
}

void program(void *a)
{
	thread* t[num_requesters + 1];
	
	int *order = new int[num_requesters];
	
	for(int i = 0; i < num_requesters; i++)
	{
		order[i] = i;
		t[i] = new thread((thread_startfunc_t) request_func, (void *) (order + i));
	}
	
	t[num_requesters] = new thread((thread_startfunc_t) service_func, (void *) 0);
	
	for(int i = 0; i < num_requesters + 1; i++)
	{
		t[i]->join();
	}
	
	for(int i = 0; i < num_requesters + 1; i++)
	{
		delete t[i];
	}
	
	delete [] order;
}

int main()
{
	max_request = 5;
	num_requesters = 10;
	active_services = (max_request > num_requesters) ? num_requesters : max_request;
	active_requesters = num_requesters;
	requesters.resize(num_requesters);
	disk_queue.reserve(max_request);
	req_pointer.resize(num_requesters);

	string tmpnum;
	for(int i = 0; i < num_requesters; i++)
	{
		for(int j = 0; j < 10; j++)
		{
			requesters[i].push_back(i*i*j + i*j + j);
		}
	}	
	
	cpu::boot(1, &program, new int(0), false, false, 20);

	return 0;
}
