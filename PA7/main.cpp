#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <regex>
using namespace std;

const static std::regex url_regex{"http://([a-z-.]*)/(.*)"};



struct addrinfo hints, *res;
int sockfd;


int main(int argc, char *argv[])
{
    close(sockfd);
    struct addrinfo hints, *res;
    int sockfd;
    string port_no = "80";
    string url = argv[1];
    
    pair<string,string> host_path;//[host,path]

    if (smatch smatch;regex_search(url, smatch, url_regex))
    {
        host_path.first = smatch[1];
        host_path.second = smatch[2];
    }
    cout<<"host: "<<host_path.first<<endl;
    cout<<"path: "<<host_path.second<<endl<<endl;
    string filename = "1.html";//host_path.first+".html"; 

    stringstream request_string;
    
    request_string << "GET /"  << host_path.second   << " HTTP/1.1\r\n";
    request_string << "Host: " << host_path.first  << "\r\n";
    request_string << "Accept: text/html"<<"\r\n\r\n";
    //cout<< "Host: " << host_path.first  << "\r\n\r\n";
    //request_string << "Connection: keep-alive\r\n";
    //cout << "Connection: keep-alive\r\n\r\n";
    //request_string << "Keep-Alive: 300\r\n\r\n";
    //cout << "Keep-Alive: 300\r\n";
    
    auto request = request_string.str();
    cout<<request<<endl;
    const auto& request_c_string = request.c_str();
    //string request_c_string = "GET /"  + host_path.second   + " HTTP/1.1\r\n" + "Host: " + host_path.first  + "\r\n" + "Accept: text/html"+"\r\n\r\n";
    int reqlength = sizeof(request_c_string);
    ////////////////////////////////////////////////////////////////////////////////////////////////


    cout<<"request created..."<<endl;
    
    int bytes_sent;
    char responsebuf[500];
    int responselength = 500;
    int bytes_recv;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status;
    //getaddrinfo("www.example.com", "3490", &hints, &res);
    if ((status = getaddrinfo (host_path.first.c_str(), port_no.c_str(), &hints, &res)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(status) << endl;
        exit(1);
    }
    cout<<"passed getadderinfo"<<endl;


    // for(iter = res; iter!=nullptr; iter = iter->ai_next)
    // {
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0){
        perror ("Cannot create socket");	
        exit(1);
        //continue;
    }
    cout<<"made socket"<<endl;

    // connect!
    if (connect(sockfd, res->ai_addr, res->ai_addrlen)<0){
        perror ("Cannot Connect");
        exit(1);
        //break;
    }
    cout<<"connected"<<endl;

    if((bytes_sent = send(sockfd, request_c_string, reqlength,0)) < 0)
    {
        perror("Could not send:");
    }
    if(bytes_sent != sizeof(request_c_string))
    {
        cout<<"Could not send whole request"<<endl;
        exit(1);
    }
    cout<<"Request sent..."<<endl;




    fstream  afile;
    afile.open(filename, ios::out  );

    cout<<"Response recieved ..\n\n"<<endl;

    
     
    char * ptr;
    bool found_headers = false;
    
    while((bytes_recv = recv(sockfd,responsebuf,responselength,0)) > 0)
    {
        // if(!found_headers && (ptr = std::strstr(responsebuf, "\r\n\r\n")))
        // {
        //     found_headers = true;
        //     // auto delta = ptr - responsebuf + sizeof("\r\n\r\n") - 1;
            
        //     // afile.write(responsebuf + delta,bytes_recv - delta);
        // }
        // else{
        //     //afile.write(responsebuf, bytes_recv);
        //     afile<<responsebuf;
        // }
        afile<<responsebuf;
        cout<<responsebuf;
    }

    
    cout<<"Done."<<endl;


    afile.close();

    close(sockfd);
}