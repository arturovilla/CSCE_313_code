#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"

#define load 5 //this is defined as the maximum wieght the rope can carry as each monkey will have a random wieght


using namespace std;


int monkey_count[2] = {0,0}; //counter for direction
Semaphore* mtx[2]; // mtx for counter protection
Semaphore dirmtx(1); // direction of first monkey
int capcount[2] = {0,0};
Semaphore* cc[2];
Semaphore capacity(load);
Semaphore wlock(1);


void WaitUntilSafe(int dir, int weight)
{
    mtx[dir]->P();
    monkey_count[dir]++;

    if (monkey_count[dir]==1)
    {
        dirmtx.P();
    }

    mtx[dir]->V();
    
    wlock.P();
    int w = weight;
    //wlock.V();
    
    ////outer lock for direction
    //cc[dir]->P();
    if(w == 3)
    {
        cc[dir]->P();
        capcount[dir]+=3;
        cc[dir]->V();
        capacity.P();
        capacity.P();
        capacity.P();
    }
    if(w == 2)
    {
        
        cc[dir]->P();
        capcount[dir]+=2;
        cc[dir]->V();
        capacity.P();
        capacity.P();
    }
    if(w == 1)
    {
        cc[dir]->P();
        capcount[dir]+=1;
        cc[dir]->V();
        capacity.P();
    }
    
    
    

   
}

void DoneWithCrossing(int dir,int weight)
{
    mtx[dir]->P();
    monkey_count[dir]--;
    
    if(monkey_count[dir] == 0)
    {
        dirmtx.V();
    }

    mtx[dir]->V();
    //outer lock for direction
    wlock.V();
    int w = weight;
    //wlock.V();
    
    //outer lock for direction 
    //cc[dir]->P();
    if(w == 3)
    {
        
        cc[dir]->P();
        capcount[dir]-=3;
        cc[dir]->V(); 
        
        capacity.V();
        capacity.V();
        capacity.V();
    }
    if(w == 2)
    {
        cc[dir]->P();
        capcount[dir]-=2;
        cc[dir]->V(); 
        
        capacity.V();
        capacity.V();
    }
    if(w == 1)
    {
        cc[dir]->P();
        capcount[dir]-=1;
        cc[dir]->V();   
        capacity.V();
    }
   


}

void CrossRavine(int monkeyid, int dir, int weight)
{
    //prt.P();
    cout << "Monkey [id=" << monkeyid << ", wt=" << weight <<", dir=" << dir <<"] is ON the rope" << endl;
    cout << "cuurent weight on rope: " << capcount[dir] << endl;
    sleep (rand()%5);
    cout << "Monkey [id=" << monkeyid << ", wt=" << weight <<", dir=" << dir <<"] is OFF the rope" << endl;
    //prt.V();
}

void monkey(int monkeyid, int dir, int weight) 
{
    WaitUntilSafe(dir, weight);         // define
    CrossRavine(monkeyid, dir, weight); // given, no work needed
    DoneWithCrossing(dir, weight);      // define
}



int main()
{

	vector<thread> Monkeys_going_west;
	vector<thread> Monkeys_going_east;


    Semaphore cnt1(1);
    Semaphore cnt2(1);
    mtx[0] = &cnt1;
    mtx[1] = &cnt2;

    Semaphore cnt1c(1);
    Semaphore cnt2c(1);
    cc[0] = &cnt1c;
    cc[1] = &cnt2c;

    for(int i=0; i < 10; i++)
    {
        int w = rand() % 3 + 1;
		Monkeys_going_east.push_back(thread(monkey,i+1, 0, w));//the last parameter here can be a randomly generated num
	}
    for(int i=0; i < 10; i++)
    {
        int w = rand() % 3 + 1;
		Monkeys_going_west.push_back(thread(monkey,i+1, 1, w));//the last parameter here can be a randomly generated num
	}


    
    for(int i=0; i < 10; i++)
    {
		Monkeys_going_east[i].join();
    }
    for(int i=0; i < 10; i++)
    {
		Monkeys_going_west[i].join();
    }

	return 0;
}