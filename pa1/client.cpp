/*

name: Arturo Villalobos
UIN: 827008236
class: CSCE 313 - 503
date: 2-11-21
soures:
------https://www.techiedelight.com/find-execution-time-c-program/

Tanzir Ahmed
Department of Computer Science & Engineering
Texas A&M University
Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/time.h>
#include <fstream>
using namespace std;

int main(int argc, char *argv[])
{

    struct timeval start, end;
    gettimeofday(&start,NULL);

    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

	
    __int64_t filesz;
    
    int buflength = 256;
    int personnum = 0;
    int ecgnum = -1;
    double secondsnum = -1.0;
    char* filenamefromarg = NULL;
    int command; // commands will be person , seconds , and ecg machine num

    while((command = getopt(argc,argv,"p:t:e:f:m:")) != -1)// size, arguemnt, definition return ascii value of argument
    {
        switch(command)
        {
            case 'p':
                personnum = stoi(optarg);
                break;
            case 't':  // optarg is a string ::: to use in program string must be converted
                secondsnum = stod(optarg);
                break;
            case 'e':
                ecgnum = stoi(optarg);
                break;
            case 'f':
                filenamefromarg = optarg;
                break;
            case 'm':
                buflength = stoi(optarg);
            case '?':
                break;

        }
    }


 
    
    

        
        if(personnum!=0)//requesting data point from command line block
        {
            if(secondsnum==-1 && ecgnum ==-1) // if only the p flag is specfied 
            {
                secondsnum = 0.000;
                ofstream file ("x1.csv");
                for(int i  = 0; i<1000;i++)// for loop starting at ecg 1 and time 0
                {
                    datamsg e1(personnum,secondsnum,1);
                    chan.cwrite(&e1,sizeof(datamsg));//f is on the fifo
                    double r1;//result
                    chan.cread(&r1,sizeof(double));//store in local variable

                    datamsg e2(personnum,secondsnum,2);
                    chan.cwrite(&e2,sizeof(datamsg));//f is on the fifo
                    double r2;//result
                    chan.cread(&r2,sizeof(double));//store in local variable

                    file<<secondsnum<< "," <<r1 << "," << r2 <<"\n";

                    secondsnum+=0.004;
                }
                file.close();
            }
            if(ecgnum!=-1)
            {
                datamsg datapointreq(personnum,secondsnum,ecgnum);
                chan.cwrite(&datapointreq,sizeof(datamsg));//f is on the fifo
                double r;//result
                chan.cread(&r,sizeof(double));//store in local variable 
                cout<<"Date point: "<<r<<endl;
            }
        }

        //file transfer block 
        //cout<<"here"<<endl;
        

        if(filenamefromarg)
        {
            filemsg filesizereq(0,0);//size request is when both params are 0
            char* filename = filenamefromarg;
            int size_total = sizeof(filemsg) + strlen(filename) + 1;
            char* buf = new char[size_total];//char pointer need +1 \n
            memcpy(buf,&filesizereq,sizeof(filemsg));
            strcpy(buf+sizeof(filemsg),filename);
            chan.cwrite(buf,size_total);//buff is on the fifo
            __int64_t filesize;
            chan.cread(&filesize,sizeof(__int64_t));// now we have the file size
            cout<<"filesize: "<<filesize<<endl;
            filesz = filesize;


            if(filesize <= buflength)
            {
                buflength = filesize;
                filemsg *fm = (filemsg*) buf;
                fm->length = buflength;
                chan.cwrite(buf,size_total);// buf used to be 0,0 but now its 0,256
                char* rec_buf = new char[buflength];
                chan.cread(rec_buf,buflength);
                string outputpath = string("received/") + string(filename);
                FILE *outputfile = fopen(outputpath.c_str(),"wb");
                fwrite(rec_buf,1,buflength,outputfile);
                delete [] rec_buf;
            }
            else
            {
                //to actually get stuff from the file 
                int offset = buflength; //migos!!!
                filemsg *fm = (filemsg*) buf;
                fm->length = buflength;
                chan.cwrite(buf,size_total);// buf used to be 0,0 but now its 0,256
                char* rec_buf = new char[buflength];
                chan.cread(rec_buf,buflength);
                string outputpath = string("received/") + string(filename);
                FILE *outputfile = fopen(outputpath.c_str(),"wb");
                fwrite(rec_buf,1,buflength,outputfile);
                delete [] rec_buf;

                while(offset!=filesize)
                {
                    if((filesize - offset) < buflength)
                    {
                        int newbuf  = filesize - offset;
                        fm->length = newbuf;
                        fm->offset = offset;
                        chan.cwrite(buf,size_total);// buf used to be 0,0 but now its 0,256
                        char* rec_buf = new char[newbuf];
                        chan.cread(rec_buf,newbuf);
                        fwrite(rec_buf,1,newbuf,outputfile);
                        offset  = offset + newbuf;
                        delete [] rec_buf;
                    }
                    else
                    {
                        fm->offset = offset;
                        chan.cwrite(buf,size_total);// buf used to be 0,0 but now its 0,256
                        char* rec_buf = new char[buflength];
                        chan.cread(rec_buf,buflength);
                        fwrite(rec_buf,1,buflength,outputfile);
                        offset  = offset + buflength;
                        delete [] rec_buf;
                    }
                }
                fclose(outputfile);
            }
        }
        



















        gettimeofday(&end,NULL);
        long seconds = (end.tv_sec - start.tv_sec);
        long microsecs = ((seconds*1000000) + end.tv_usec) - (start.tv_usec);
        cout<<"Time elpased is: " << seconds<< " seconds and " << microsecs<< " microseconds"<<endl;
        //long timepast = (end.tv_sec-start.tv_sec)*1000000 + end.tv_usec-start.tv_usec;
        //cout<<"Time elpased is: " << timepast<<endl;
        //printf("%ld\n", ((end.tv_sec * 1000000 + end.tv_usec)- (start.tv_sec * 1000000 + start.tv_usec)));
        

        //ofstream file2 ("timeing.csv");
        //file2<<buflength<< "," <<filesz << "," << timepast <<"\n";
        //file2.close();

        char* buffernewchan = new char[30];
        MESSAGE_TYPE newchan = NEWCHANNEL_MSG;
        chan.cwrite(&newchan,sizeof(MESSAGE_TYPE));
        chan.cread(buffernewchan,30);
        FIFORequestChannel newwchan (buffernewchan, FIFORequestChannel::CLIENT_SIDE);
        datamsg datareq(12,0.004,1);
        newwchan.cwrite(&datareq,sizeof(datamsg));//f is on the fifo
        double r;//result
        newwchan.cread(&r,sizeof(double));//store in local variable 
        cout<<"Date point new channel "<<r<<endl;
        
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite (&m, sizeof (MESSAGE_TYPE));

        MESSAGE_TYPE m1 = QUIT_MSG;
        newwchan.cwrite (&m1, sizeof (MESSAGE_TYPE));
        
        

   
    
    
}//main
