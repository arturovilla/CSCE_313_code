#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <assert.h>
#include <condition_variable>

using namespace std;

class BoundedBuffer
{
private:
	int cap; // max number of items in the buffer
	queue<vector<char>> q;	
	

	// thread safety
	mutex m;
	condition_variable data_available; // for wait by the pop, signaled push function
	condition_variable slot_available; // waited on by the push, sssignaled by the poping an item from qeue.
public:
	BoundedBuffer(int _cap){
		cap = _cap;
	}
	~BoundedBuffer(){

	}

	void push(char* data, int len){
		//1. Wait until there is room in the queue (i.e., queue lengh is less than cap)
		unique_lock<mutex> l (m);
		slot_available.wait(l, [this]{return q.size() < cap;});
		//2. Convert the incoming byte sequence given by data and len into a vector<char>
		vector<char> d(data, data+len);
		//3. Then push the vector at the end of the queue, wtach out for race considtion
		q.push(d);
		
		data_available.notify_one();
	}

	int pop(char* buf, int bufcap){
		//1. Wait until the queue has at least 1 item
		unique_lock<mutex> l (m);
		data_available.wait(l, [this]{return q.size()>0;});
		//2. pop the front item of the queue. The popped item is a vector<char>, watch for race condition
		vector<char> d = q.front();
		q.pop();
		
		memcpy(buf, d.data(), d.size());
		//5. wake up any potentially sleeping push() functions
		slot_available.notify_one();
		//4. Return the vector's length to the caller so that he knows many bytes were popped
		return d.size();
	}
};

#endif /* BoundedBuffer_ */
