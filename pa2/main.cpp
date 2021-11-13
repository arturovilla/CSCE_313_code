#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector> 
#include <regex>

using namespace std;

//removes leading and trailing spaces


int cd(char *path) 
{
    return chdir(path);
}

string trim(string input)
{
    return regex_replace(input, std::regex("^ +| +$|( ) +"), "");
    //^beggining of the line + 1 or more | seperator| +$1 or more end of the line | (whitespace) 1 or more replace all that with "" which is no spaces
}

// takes string ans splits phrases into a vector
vector<string> split(string text, char delim = ' ') 
{
    string line;
    vector<string> results;
    stringstream ss(text);
    while(getline(ss, line, delim)) {
        results.push_back(trim(line));
    }
    return results;
}

char** vec_to_char_array(vector<string> parts)
{
    char** result = new char* [parts.size()+1];
    for(int i =0; i < parts.size();i++)
    {
        result[i] = (char*) parts[i].c_str();
    }
    result[parts.size()] = NULL;
    return result;
}


int main()
{
    char* p=getenv("USER");

    vector<int> bgpids;//list of background process ids
    dup2(0,10);
    while(true )
    {
        dup2(10,0);
        ///
        for(int i =0 ; i< bgpids.size(); i++)
        {
            if(waitpid(bgpids[i],0,WNOHANG) == bgpids[i])
            {
                cout<<"::PROCESS "<<bgpids[i]<<" ENDED::"<<endl;
                bgpids.erase(bgpids.begin() + i);
                i--;
            }
        }

        cout<<p<<"$: ";
        string inputline;
        getline(cin,inputline);
    
        if(inputline == string("exit"))
        {
            cerr<<"All done user bye!"<<endl;
            exit(1);

            break;
        }// end of exit branch

        bool backgroundps = false;
        inputline  = trim(inputline);
        if(inputline[inputline.size()-1] == '&')
        {
            cout<<"::BACKGROUND PROCESS STARTED::"<<endl;
            backgroundps = true;
            inputline = trim(inputline.substr(0,inputline.size()-1));
        }

        // if(inputline.substr(0,2) == string("cd"))
        // {
        //     char *changecomm = (char*) inputline.c_str();
        //     cd(changecomm);
        //     continue;
        // }
        

        vector<string> pparts = split(inputline, '|');

        for(int i =0 ;i<pparts.size();i++)
        {


            int fds[2];
            pipe(fds);
            int pid = fork();

            if(pid ==0)// CHILD
            {
                inputline = pparts[i];
                if(i<pparts.size()-1)// all but the last command get redirected
                {
                    dup2(fds[1],1);
                }

                if(inputline.substr(0,2) == string("cd"))
                {
                    string dir = inputline.substr(3);
                    char *changecomm = (char*) trim(dir).c_str();
                    cd(changecomm);// returns 0 if successful
                    break;
                    
                }
                if(inputline.substr(0,5) == string("mkdir"))
                {
                    string dir  = inputline.substr(5);
                    char* changecomm = (char*) trim(dir).c_str();
                    int status = mkdir(changecomm, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    // read/write/search permissions for owner and group, and with read/search permissions for others.
                    break;

                }



                int pos = inputline.find('>');
                if(pos >=0)
                {
                    string command  = trim(inputline.substr(0,pos));
                    string filename = trim(inputline.substr(pos+1));

                    int fd = open(filename.c_str(), O_WRONLY|O_CREAT , S_IWUSR|S_IRUSR);
                    dup2(fd,1);
                    close(fd);
                    inputline = command;
                }
                pos = inputline.find('<');
                if(pos >=0)
                {
                    string command  = trim(inputline.substr(0,pos));
                    string filename = trim(inputline.substr(pos+1));

                    int fd = open(filename.c_str(), O_RDONLY|O_CREAT , S_IWUSR|S_IRUSR);
                    dup2(fd,0);
                    close(fd);
                    inputline = command;
                }


                vector<string> parts = split(inputline);
                char** args = vec_to_char_array(parts);
                execvp(args[0],args);
            }
            else// PARENT
            {
                if(!backgroundps)//no bg processses
                {
                    waitpid(pid,0,0);
                    dup2(fds[0],0);
                    close(fds[1]);
                }
                else
                {
                    bgpids.push_back(pid);
                }//backgroundprocess branch
            }
        }//end of pparts for-loop
    }//end of while loop
}//end of main



// if(!isBG)
// {
//     if(i == pipedparts.size()-1)
//     {
//         waitpid(pid,0,0);
//     } 
//     else 
//     {
//         bgpids.push_back(pid);
//     }

// } 
// else 
// {
//     bgpids.push_back(pid);
// }