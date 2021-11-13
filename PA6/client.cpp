/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/19

    modified : Arturo Villalobos 
 */
#include "common.h"
#include <sys/wait.h>
#include "TCPreqchannel.h"
#include "BoundedBuffer.h"
#include "HistogramCollection.h"
#include <thread>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unordered_map>


using namespace std;
struct Response{
    int person;
    double ecgval;

    Response (int _p, double _e): person (_p), ecgval (_e){;}

};



void timediff (struct timeval& start, struct timeval& end)
{
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    std::cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
}

// TCPrequestChannel* create_new_channel (TCPrequestChannel* mainchan)
// {
//     char name [1024];
//     MESSAGE_TYPE m = NEWCHANNEL_MSG;
//     mainchan->cwrite (&m, sizeof (m));
//     mainchan->cread (name, 1024);
//     TCPrequestChannel* newchan = new TCPrequestChannel (name, TCPrequestChannel::CLIENT_SIDE);
//     return newchan;
// }

void patient_thread_function (int n, int p, BoundedBuffer* reqbuffer)
{
    double t = 0;
    datamsg d (p, t, 1);
    for (int i=0; i<n; i++){
        reqbuffer->push ((char*) &d, sizeof (d));
        d.seconds += 0.004;
    }
    
}


void worker_thread_function (int mb, BoundedBuffer* requestBuffer, TCPrequestChannel* workChan, BoundedBuffer* responseBuffer){
    char buf [1024];
    double resp = 0;

    char recvbuf [mb];
    while (true){
        requestBuffer->pop (buf, 1024);
        MESSAGE_TYPE* m = (MESSAGE_TYPE *) buf;
        
        if (*m == DATA_MSG)
        {
            datamsg* d = (datamsg*) buf;
            workChan->cwrite (d, sizeof (datamsg));
            workChan->cread (&resp, sizeof (double));
            Response r (d->person, resp);
            responseBuffer->push ((char*)&r, sizeof (Response));
        }
        else if (*m == QUIT_MSG)
        {
            workChan->cwrite (m, sizeof (MESSAGE_TYPE));
            delete workChan;
            break;
        }
        else if (*m == FILE_MSG)
        {
            filemsg* fm = (filemsg*) buf;
            string fname = (char *)(fm + 1);
            int sz = sizeof (filemsg) + fname.size() + 1;
            workChan->cwrite (buf, sz);
            workChan->cread (recvbuf, mb);

            string recvfname = "recv/" + fname;
            FILE* fp = fopen (recvfname.c_str(), "r+");
            fseek (fp, fm->offset, SEEK_SET);
            fwrite (recvbuf, 1, fm->length, fp);
            fclose (fp);
        }
    }
}

void histogram_thread_function (BoundedBuffer* responseBuffer, HistogramCollection* hc){
    char buf [1024];
    Response* r = (Response *) buf;
    while (true){
        //cout<<"in histo function"<<endl;
        responseBuffer->pop (buf, 1024);
        if (r->person < 1){ // it means quit
            //cout<<"made it into break"<<endl;
            break;
        }
        hc->update (r->person, r->ecgval);
    }
}



void file_thread_function(string fname, BoundedBuffer* request_buffer, TCPrequestChannel* chan, int mb)
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



void event_polling_function (int w, TCPrequestChannel** wchans, BoundedBuffer* request_buffer, int mb,BoundedBuffer* responseBuffer){
    char buf [1024];
    double resp = 0;
    char recvbuf [mb];


    struct epoll_event ev;
    struct epoll_event events[w];

    int epollfd = epoll_create1(0);//exmpty list
    if(epollfd == -1)
    {
        EXITONERROR("epoll create 1");
    }
    
    unordered_map <int,int> fd_to_index;
    vector<vector<char>> state (w);
    int nsent = 0;
    int nrecv = 0;


    for(int i = 0; i < w; i++)
    {
        int sz = request_buffer->pop(buf,1024);
        wchans[i]->cwrite(buf,sz);
        nsent ++;
        state[i] = vector<char>(buf,buf+sz);

        int rfd  = wchans[i]->getfd(); 
        fd_to_index[rfd] = i;
        fcntl(rfd, F_SETFL,O_NONBLOCK);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = rfd;
        if(epoll_ctl(epollfd , EPOLL_CTL_ADD, rfd, &ev) == -1)
        {
            EXITONERROR("epoll ctl");

        }
    }//end of for loop


    bool quit_recved = false;
    while(true) 
    {
        if(quit_recved == true && nsent==nrecv)
        {
            break;
        }
        int nfds = epoll_wait(epollfd,events,w,-1); // if any events from epollfd are triggered , then put them into events , return events triggered
        if(nfds ==-1)
        {
            EXITONERROR("epoll wait ");

        }
        
        for(int i = 0; i < nfds; i++)
        {
            int rfd = events[i].data.fd;
            int index = fd_to_index[rfd];
            int response = wchans[index]->cread(recvbuf,mb);
            nrecv++;
            vector<char> req = state[index];
            char* request = req.data();
            MESSAGE_TYPE* m = (MESSAGE_TYPE*)request;
            if (*m == DATA_MSG)
            {
               
                //datamsg* d = (datamsg*) request;
                //Response r (((datamsg*)request)->person, resp);

                Response r{((datamsg*)request)->person,*(double*)recvbuf};
                //cout<<"recvbuf datamsg: "<< recvbuf <<endl;
                responseBuffer->push ((char*)&r, sizeof (r));
                //cout<<"pushed data to response buf"<<endl;
            }
            else if (*m == FILE_MSG)
            {
                filemsg* fm = (filemsg*) request;
                string fname = (char *)(fm + 1);
                int sz = sizeof (filemsg) + fname.size() + 1;
               

                string recvfname = "recv/" + fname;
                FILE* fp = fopen (recvfname.c_str(), "r+");
                fseek (fp, fm->offset, SEEK_SET);
                fwrite (recvbuf, 1, fm->length, fp);
                fclose (fp);
            }

            if(!quit_recved)
            {

                int req_sz = request_buffer->pop(buf,sizeof(buf));
                if(*(MESSAGE_TYPE*)buf == QUIT_MSG)
                {
                    //cout<<"quit recieved "<<endl;
                    quit_recved = true;
                }
                wchans[index]->cwrite(buf, req_sz);
                nsent++;
                state[index] = vector<char> (buf, buf+req_sz);
            }
            
            
            
        }//end of for loop


    }//end of while loop    
}//end of function




int main(int argc, char *argv[])
{
    
    int c = -1;
    int buffercap = MAX_MESSAGE;
    int p = 10, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    int b = 1024;
    int w = 100;
    int n = 10000;
    int m = MAX_MESSAGE;
    int h = 3;
    string host_name;
    string port_no;

    while ((c = getopt (argc, argv, "p:t:e:m:f:b:cw:n:h:z:r:")) != -1){//host nam = z, portnum =r
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                m = buffercap;
                break;
            case 'c':
                isnewchan = true;
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'b':
                b = atoi (optarg);
                break;
            case 'w':
                w = atoi (optarg);//number of request channels 
                break;
            case 'n':
                n = atoi (optarg);
                break;
            case 'h':
                h = atoi (optarg);
                break;
            case 'z':
                host_name = optarg;
                break;
            case 'r':
                port_no = optarg;
                break;
        }
    }
    
    // fork part
    

    TCPrequestChannel* chan = new TCPrequestChannel(host_name,port_no);
    //BoundedBuffer req_Buffer (b);
    BoundedBuffer requestBuffer (b);
	BoundedBuffer responseBuffer (b);
    HistogramCollection hc;

    // making histograms and adding to the histogram collection hc
    for (int i=0; i<p; i++){
        Histogram* h = new Histogram (10, -2.0, 2.0);
        hc.add (h);
    }

    // make w worker channels (make sure to do it sequentially in the main)
    TCPrequestChannel** wchans  = new TCPrequestChannel*[w];
    for (int i=0; i<w; i++){
        wchans [i] = new TCPrequestChannel(host_name,port_no);     
    }
	
	
    struct timeval start, end;
    gettimeofday (&start, 0);




    /* Start all threads here */
    
    thread patient [p];
    for (int i=0; i<p; i++){
        patient [i] = thread (patient_thread_function, n, i+1, &requestBuffer);
    }
    //file thread
    
    //thread filethread(file_thread_function, filename, &requestBuffer,chan,m);
    
    thread filethread; //(file_thread_function, filename, &requestBuffer,chan,m);
    if(isfiletransfer == true)
    {
        filethread = thread(file_thread_function, filename, &requestBuffer,chan,m);
    }

    thread evp (event_polling_function, w,wchans, &requestBuffer,m,&responseBuffer);
    
    
    thread hists [h];
    for (int i=0; i<h; i++){
        hists [i] = thread (histogram_thread_function, &responseBuffer, &hc);
    }

    




  	/* Join all threads here */
    for (int i=0; i<p; i++){
        patient [i].join ();
    }
    std::cout << "Patient threads finished" << endl;

    
    MESSAGE_TYPE wq = QUIT_MSG;
    requestBuffer.push ((char*) &wq, sizeof (wq));

    
    evp.join();
    std::cout << "event polling thread finished" << endl;
    

    for (int i=0; i<h; i++){
        datamsg d(-1, 0, -1);
        responseBuffer.push ((char*)&d, sizeof (d));
    }

    for (int i=0; i<h; i++){
        hists [i].join ();
    }
    std::cout << "Histogram threads are done. All client threads are now done" << endl;

    if(isfiletransfer == true)
    {
        filethread.join();
        std::cout<< "file thread has finished" << endl;
    }
    //filethread.join();
    //std::cout<< "file thread has finished" << endl;

    gettimeofday (&end, 0);
    // print time difference
    timediff (start, end);

	
    // print the results
    if(isfiletransfer == false)
    {
        hc.print ();
    }
	//hc.print ();


    for(int i = 0 ; i < w; i++)
    {
        delete wchans[i];
    }


    // cleaning the main channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    wait (0);
    std::cout << "All Done!!!" << endl;

    delete chan;
   
}


