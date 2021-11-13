#include <iostream>
//#include <vector>
 //Blank A
#include <vector>//needed to include the right ,libraries
using namespace std;// standard 



class node {
 //Blank B
 public:	// allows function to acces the data attributes
 	int val;
 	node* next;
};



void create_LL(vector<node*>& mylist, int node_num){
    mylist.assign(node_num, NULL); /// all elements in the lsist are assigned to null

    //create a set of nodes
    for (int i = 0; i < node_num; i++) {
        //Blank C
        mylist[i] = new node(); // first make a node since its null at first 
        mylist[i]->val = i; // for some reason the . operator didnt work so i need to use ->
        mylist[i]->next = NULL;
    }

    //create a linked list
    for (int i = 0; i < node_num; i++) {
        if(i == node_num-1)
        {
            mylist[i]->next = NULL;
        }
        else
        {
            mylist[i]->next = mylist[i+1];//member attribute not acces with the right operator
        }
        
        
    }
}

int sum_LL(node* ptr) {
    int ret = 0;
    while(ptr) {
        ret += ptr->val;//operators again
        ptr = ptr->next;
    }
    return ret;
}

int main(int argc, char ** argv){
    const int NODE_NUM = 3;
    vector<node*> mylist;

    create_LL(mylist, NODE_NUM);
    int ret = sum_LL(mylist[0]); 
    cout << "The sum of nodes in LL is " << ret << endl;

    //Step4: delete nodes
    //Blank D
    for(int i =0; i <NODE_NUM;i++)
    {
        delete  mylist[i];
    }
    
}


//TERMINAL COMMANDS
//COMPILE: g++ -o [executable name] file.cpp
//RUNNING: ./[executable name]  
//GDB: g++ -g -o [executable name] file.cpp ::::: then:  gdb [executable name].out
//ADDSANT:  g++ -o [executable name] file.cpp -fsanitize=address ::::: then execute
