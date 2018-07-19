#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <list>
#include <errno.h>
#include <sys/wait.h>

using namespace std;

int parse(char *line, char **argv)
{
    int argc=0;
    while (*line != '\0') {       // if not the end of line
      while (*line == ' ' || *line == '\t' || *line == '\n')
           *line++ = '\0';     // replace white spaces with 0
      *argv++ = line;          // save the argument position
      argc++;
      while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
           line++;             // skip the argument until
    }
    *argv = '\0';                 // mark the end of argument list
    return argc;
}

int main(int argc, char **argv, char **envp)
{
    char text[]="************************************************************\n"
                "  This is an example for project 1. It is not perfect\n"
                "  but have most of the functions done. You can play with\n"
                "  it and better understand the requests of the project.\n"
                "                                wangdin1@cse.msu.edu        \n"
                "************************************************************";
    list<string> hlist;  //  list of history command
    list<string> dlist;  //  list of directories
    char str[128],*username;
    getcwd(str,sizeof(str));  //  get current directory
    dlist.push_front(str);  //save in directory list
    username=getenv("USER");  //  get username
    cout<<text<<endl;
    while(true){
        cout<<'<'<<hlist.size()+1<<' '<<username<<'>';
        int argc_t;
        char line[128], *argv_t[64];
        cin.getline(line,128);  //  read one ommand line
        argc_t=parse(line, argv_t);  //split command in strings
        if(argc_t==0 || strlen(argv_t[0])==0)//  no command
            continue;
        hlist.push_back(line);  //  save in command list
        //  if call history command
        if (argv_t[0][0]=='!'){
            // find the command line from history command list
            int n=atoi(argv_t[0]+1);
            if(n>0 && n<=hlist.size()){  //  if the history command exsits
                std::list<string>::iterator ptr=hlist.begin();
                std::advance(ptr, n-1);
                strcpy(line,ptr->c_str());
                line[ptr->size()]='\0';
                argc_t=parse(line, argv_t);  //  split history command in strings
            }
            else{
                cout<<"Error: No such command saved!"<<endl;
                continue;
            }
        }
        //  internal function quit
        if(strcmp(argv_t[0],"quit")==0)
            return 0;
        //  internal function date
        else if(strcmp(argv_t[0],"date")==0){
            time_t t;
            time(&t);
            cout<<"Current time is: "<<ctime(&t);
        }
        //  internal function curr
        else if(strcmp(argv_t[0],"curr")==0){
            char str[100];
            getcwd(str,sizeof(str));
            cout<<"Current directory is: "<<str<<endl;
        }
        //  internal function env
        else if(strcmp(argv_t[0],"env")==0){
            for (char **env = envp; *env != 0; env++){
                char *thisEnv = *env;
                printf("%s\n", thisEnv);
            }
        }
        //  internal function cd
        else if(strcmp(argv_t[0],"cd")==0){
            //  two input needed
            if(argc_t<2){
                cout<<"Error: No input directory!"<<endl;
                continue;
            }
            //  if call history directory
            if (argv_t[1][0]=='#'){
                // find the directory line from history directory list
                int n=atoi(argv_t[1]+1);
                if(n>0 && n<=dlist.size()){  //  if the history directory exsits
                    std::list<string>::iterator ptr=dlist.begin();
                    std::advance(ptr, n-1);
                    strcpy(argv_t[1],ptr->c_str());  //  copy the history directory
                }
                else
                    argv_t[1]='\0';
            }
            if(chdir(argv_t[1])==0)  //  if enter new directory succesfully
                dlist.push_front(argv_t[1]);  //  save new directory to list
            else
                cout<<"Error: No such directory!"<<endl;
        }
        //  internal function hlist
        else if(strcmp(argv_t[0],"hlist")==0){
            std::list<string>::iterator ptr, ptr1, ptr2;
            ptr1=hlist.begin();
            int i=0;
            //  set the start point from hlist
            if(hlist.size()>10){
                std::advance(ptr1, hlist.size()-10);
                i=hlist.size()-10;
            }
            // print the history command from hlist
            ptr2=hlist.end();
            for(ptr=ptr1; ptr!=ptr2; i++, ptr++)
                cout<<i+1<<' '<<*ptr<<endl;
        }
        //  internal function dlist
        else if(strcmp(argv_t[0],"dlist")==0){
            std::list<string>::iterator ptr, ptr1, ptr2;
            ptr1=dlist.begin();
            int i=0;
            //  set the start point from dlist
            if(dlist.size()>10){
                std::advance(ptr1, dlist.size()-10);
                i=dlist.size()-10;
            }
            // print the history directory from dlist
            ptr2=dlist.end();
            for(ptr=ptr1; ptr!=ptr2; i++, ptr++)
                cout<<i+1<<' '<<*ptr<<endl;
        }
        else{
            int pid = fork();
            bool background=false;
            if(pid<0) {  // fork function failed
              cout<<"Error: Fork function failed!"<<endl;
              continue;
            }
            if(strcmp(argv_t[argc_t-1],"&")==0){  //  child process need to run in background
                background=true;
                argv_t[argc_t-1]='\0';
            }
            if(pid==0) {  // for the child process:
                if (execvp(*argv_t, argv_t) < 0) {  // execute the command
                   cout<<"Error: Externel command failed!"<<endl;
                }
                return 0;
            }
            else{  // for the parent
                if(!background){
                    cout<<"Waitting for child process..."<<endl;
                    while(wait(NULL)!=pid);  // wait for child process
                }
                else
                    cout<<"Child process running in the background!"<<endl;
            }
        }
    }
    return 0;
}
