#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;




Semaphore capmtx(1);// general mutex
Semaphore thinkers_at_the_table(4); // 4 people at the table max
Semaphore* chopsticks[5];
int capacity = 0;

//these are the flags where every thinker is inti to 1(thinking), the others 2(hungry), 3(eat)

void put_down_chopsticks(int thinkerID)
{
    sleep (rand()%5);
    chopsticks[thinkerID]->V();
    cout<<"Philosopher: "<<thinkerID<<" put down his left chopstick."<<endl;
    chopsticks[((thinkerID+1)%5)]->V();
    cout<<"Philosopher: "<<thinkerID<<" put down his right chopstick."<<endl;
}

void eatfunc(int thinkerID)
{
    cout<<"Philosopher: "<<thinkerID<<" is eating..."<<endl;
    put_down_chopsticks(thinkerID);
    cout<<"Philosopher: "<<thinkerID<<" is done eating."<<endl;
    
}

void thinkfunc(int thinkerID)
{
    cout<<"Philosopher: "<<thinkerID<<" is thinking..."<<endl;
    sleep (rand()%5);
}
void pick_up_chopsticks(int thinkerID)
{
    chopsticks[thinkerID]->P();
    cout<<"Philosopher: "<<thinkerID<<" picked up left chopstick."<<endl;
    chopsticks[((thinkerID+1)%5)]->P();
    cout<<"Philosopher: "<<thinkerID<<" picked up right chopstick."<<endl;
    eatfunc(thinkerID);
}


void sit_at_table(int thinkerID)
{
    thinkers_at_the_table.P();
    thinkfunc(thinkerID);
    pick_up_chopsticks(thinkerID);
    thinkers_at_the_table.V();
}


void thinker_function(int thinkerID)
{
    for(int i = 0; i < 8 ; i++)
    {
        sit_at_table(thinkerID);
    }
}


int main()
{
    Semaphore c1(1);
    Semaphore c2(1);
    Semaphore c3(1);
    Semaphore c4(1);
    Semaphore c5(1);
    chopsticks[0] = &c1;
    chopsticks[1] = &c2;
    chopsticks[2] = &c3;
    chopsticks[3] = &c4;
    chopsticks[4] = &c5;

    vector<thread> thinkers;///number of philosopher threads
    for (int i=0; i< 5; i++)//5 philosopher in total 
    {
        //thinkerstate.push_back(1);//these are the flags where every thinker is inti to 1(thinking), the others 2(hungry), 3(eat)
        thinkers.push_back(thread (thinker_function, i)); 
    }




    for(int i=0; i < 5; i++)
    {
		thinkers[i].join();
    }

}