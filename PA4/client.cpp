#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <time.h>
#include <thread>

using namespace std;


FIFORequestChannel* create_new_channel(FIFORequestChannel* mainchan)
{
    char name [1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    mainchan->cwrite(&m, sizeof (m));
    mainchan->cread(name,1024);
    FIFORequestChannel* newchan = new FIFORequestChannel (name, FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}

void patient_thread_function(int n,int pno, BoundedBuffer* request_buffer)
{ 
    datamsg d(pno,0.0,1);
    double resp = 0;

    for(int i=0; i<n; i++)
    {
        request_buffer->push((char*) &d, sizeof(datamsg));
        d.seconds += .004;
       
    }
}

void file_thread_function(string fname, BoundedBuffer* request_buffer, FIFORequestChannel* chan, int mb)
{
    //1. create the file / open file
    string recvfname = "recv/" + fname; 
    char buf[1024];
    filemsg f (0,0);
    memcpy (buf, &f, sizeof(f));
    strcpy(buf+sizeof(f), fname.c_str());
    chan->cwrite(buf, sizeof(f)+ fname.size() + 1);
    __int64_t filelength;
    chan->cread(&filelength, sizeof(filelength));

    FILE* fp = fopen(recvfname.c_str(), "w");
    fseek(fp,filelength, SEEK_SET);
    fclose(fp);

    //2. generate the file messages

    filemsg* fm = (filemsg*) buf;
    __int64_t rem = filelength;

    while (rem>0)
    {
        fm->length = min(rem,(__int64_t) mb);
        request_buffer->push(buf,sizeof(filemsg) + fname.size()+1);
        fm->offset += fm->length;
        rem -= fm->length;
    }
    
}

void worker_thread_function( FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int mb)
{
    char buf [1024];
    double resp = 0;

    char recvbuf[mb];
    while(true)
    {
       request_buffer->pop(buf,1024);
       MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf;
       
       if(*m == DATA_MSG)
       {
           chan->cwrite(buf, sizeof(datamsg));
           chan->cread(&resp, sizeof (double));
           hc->update(((datamsg *)buf)->person,resp);
       }
       else if(*m == QUIT_MSG)
       {
           chan->cwrite(m,sizeof (MESSAGE_TYPE));
           delete chan;
           break;
       }
       else if (*m == FILE_MSG)
       {
           filemsg* fm = (filemsg*) buf;
           string fname = (char*)(fm + 1);
           int size = sizeof(filemsg)+fname.size()+1;
           chan->cwrite(buf, size);
           chan->cread(recvbuf, mb);

           string recvfname = "recv/" + fname;
           FILE* fp = fopen(recvfname.c_str(), "r+");
           fseek(fp, fm->offset , SEEK_SET);
           fwrite(recvbuf, 1, fm->length, fp);
           fclose(fp);
       }       
    }  
}



int main(int argc, char *argv[])
{
    int n = 15000;    //default number of requests per "patient"
    int p = 0;     // number of patients [1,15]
    int w = 50;    //default number of worker threads
    int b = 100; 	// default capacity of the request buffer, you should change this default
	int m = 256; 	// default capacity of the message buffer
    srand(time_t(NULL));
    string fname = "10.csv";
    
    int opt = -1;
    while((opt = getopt(argc, argv, "m:n:b:w:p:f:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            m = atoi(optarg);
            break;
        case 'n':
            n = atoi(optarg);
            break;    
        case 'p':
            p = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 'w':
            w = atoi(optarg);
            break;     
        case 'f':
            fname = optarg;
            break;                            
        }
    }
    int pid = fork();
    if (pid == 0){
        //modify this to pass along m
        execl ("server", "server", (char *)NULL);
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
    
	for(int i = 0; i<p; i++)
    {
        Histogram* h = new Histogram (10, -2.0, 2.0);
        hc.add(h);
    }

    // make w worker channels 
    FIFORequestChannel* wchans[w];
    for(int i=0; i<w; i++)
    {   
        wchans[i] = create_new_channel(chan);
    }

	
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
	thread patient [p];
    if(p <= 15 && p > 0)
    {
        for(int i=0; i<p; i++)
        {   
            patient[i] = thread(patient_thread_function, n, i+1, &request_buffer);
        }
    }
    

    thread filethread(file_thread_function, fname, &request_buffer, chan, m);
    thread workers[w];
    for(int i=0; i<w; i++)
    {
        workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc, m);
    }

	/* Join all threads here */

    for(int i=0; i<p; i++)
    {   
        patient[i].join();
    }
    
    filethread.join();

    for(int i=0; i<w; i++)
    {   
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*) &q, sizeof(q));
    }

    for(int i=0; i<w; i++)
    {   
        workers[i].join();
    }
    gettimeofday (&end, 0);
    //print time difference
   // timediff(start,end);

    // print the results
	hc.print ();

    
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;

    
}
