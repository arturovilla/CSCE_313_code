#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;


#define Nb 2


Semaphore A(0);
Semaphore B(0);
Semaphore C(1);
Semaphore mtx(1);
int bcount = 0;

void ThreadA(int id)
{
	while(true)
    {
		C.P();//link the chain so to speak, cneeds to be declared with 1 for it to run the first time 
		cout << ":: A ::" << endl;
        A.V();
        A.V();// notify both b threads
	}
}

void ThreadB(int id)
{
	while(true)
    {

		A.P();//dec a sema
		mtx.P();//use a mutex and bcount bc there need to be 2 b threads
		bcount++;
		cout << ":: B ::" << endl;
		mtx.V();


		mtx.P();
		if(bcount == Nb)//last one notifys c
        {
			B.V();
			bcount = 0;
		}
		mtx.V();

	}
}

void ThreadC(int id)
{
	while(true)
    {

		B.P();
		cout << ":: C ::" << endl;
		C.V();

	}
}

int main()
{

	vector<thread> ClinkA;
	vector<thread> AlinkB;
	vector<thread> BlinkC;

	for(int i=0; i < 100; i++)
    {
		ClinkA.push_back(thread(ThreadA,i+1));
	}

	for(int i=0; i < 100; i++)
    {
		AlinkB.push_back(thread(ThreadB,i+1));
	}

	for(int i=0; i < 100; i++)
    {
		BlinkC.push_back(thread(ThreadC,i+1));
	}


	for(int i=0; i < 100; i++)
    {
		ClinkA[i].join();
    }

	for(int i=0; i < 100; i++)
    {
		AlinkB[i].join();
    }
	for(int i=0; i < 100; i++)
    {
		BlinkC[i].join();
    }

	return 0;
}
