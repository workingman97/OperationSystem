#include <iostream>
#include <fstream>
#include <deque>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

using namespace std;

struct process{
  int id;  // process id
  int priority;  // process priority
  int num_burst;  // burst number
  int num_burst_current;  // current burst number
  int time_burst;  // burst time
  int time_blocked;  // blocked time
  int time_arrival;  // arrival time
  int time_departure;  // departure time
  int time_in;  // the time put in the state
  int cum_new;  // the cummulative time in the state: new
  int cum_ready;  // the cummulative time in the state: ready
  int cum_running;  // the cummulative time in the state: running
  int cum_blocked;  // the cummulative time in the state: blocked
  int time_turnaround; // the turnaround time
};

// Global Variables
int Time = 0;
int FreePCB = 1;
int num_CPU = 1;
int num_PCB = 1;
int time_sim = 0;
int N = 1;
char algo[100] = "FCFS";

// Semaphores
sem_t semTimer, semRunning, semBlocked, semReady, semExit;

// Queues for each state
deque<process> ProcessQueue, NewQueue, ReadyQueue, RunningQueue, BlockedQueue, ExitQueue;

// print the process status when it terminated
void print_process(process p){
    cout << "************************************************************" <<endl;
    cout << "Process " << p.id << " is terminated" << endl;
    cout << "Process identification number: " << p.id << endl;
    cout << "Priority: " << p.priority << endl;
    cout << "Number of CPU bursts: " << p.num_burst << endl;
    cout << "Burst time: " << p.time_burst << endl;
    cout << "Blocked time: " << p.time_blocked << endl;
    cout << "Arrival time: " << p.time_arrival << endl;
    cout << "Departure Time: " << p.time_departure << endl;
    cout << "Cumulative time in New State: " << p.cum_new << endl;
    cout << "Cumulative time in Ready State: " << p.cum_ready << endl;
    cout << "Cumulative time in Running State: " << p.cum_running << endl;
    cout << "Cumulative time in Blocked State: " << p.cum_blocked << endl;
    cout << "Turnaround Time: " << p.time_turnaround << endl;
    cout << "Normalized turnaround time: " << double(p.time_turnaround)/p.cum_running << endl;
    cout << "************************************************************" <<endl;
}

// print the process ID at each state
void print_state(){
    int i;
    cout << "************************************************************" <<endl;
    cout << "Current Time: " << Time << endl;
    cout << "Process ID of each process in the States" << endl;
    cout << "New state:  ";
    for(i=0; i<NewQueue.size(); i++)
        cout << NewQueue[i].id << "  ";
    cout<<endl;
    cout << "Ready state:  ";
    for(i=0; i<ReadyQueue.size(); i++)
      cout << ReadyQueue[i].id << "  ";
    cout<<endl;
    cout << "Running state:  ";
    for(i=0; i<RunningQueue.size(); i++)
      cout << RunningQueue[i].id << "  ";
    cout<<endl;
    cout << "Block state:  ";
    for(i=0; i<BlockedQueue.size(); i++)
      cout << BlockedQueue[i].id << "  ";
    cout<<endl;
    cout << "************************************************************" <<endl;
}

// clock thread
void* ClockThread(void *arg){
    while(Time <= time_sim){
        if (Time % N == 0)
            print_state();
        usleep(10000);     // usleep for microseconds, sleep for seconds
        Time++;
        sem_post(&semTimer);   // signal SemTimer
    }
}

// New state thread
void* NewThread(void *arg){
    while(Time <= time_sim){
        sem_wait(&semTimer);
        // add new process to the NewQueue
        while (ProcessQueue.size()>0 && ProcessQueue.front().time_arrival<=Time){
            NewQueue.push_back(ProcessQueue.front());
            ProcessQueue.pop_front();
            NewQueue.back().time_in=Time;
        }
        // pop from NewQueue to ReadyQueue
        while (NewQueue.size()>0 && FreePCB >0){
            NewQueue.front().cum_new+=Time-NewQueue.front().time_in;
            ReadyQueue.push_back(NewQueue.front());
            NewQueue.pop_front();
            ReadyQueue.back().time_in=Time;
            FreePCB--;
            cout << "Time: " << Time <<" Process: " << ReadyQueue.back().id << " from New state to Ready state" << endl;
            // cout<<"size of NewQueue " << NewQueue.size()<<endl;
        }
        sem_post(&semBlocked);
    }
}

// Blocked state thread
void* BlockedThread(void *arg){
    while(Time <= time_sim){
        sem_wait(&semBlocked);
        for (int i=0; i<BlockedQueue.size(); i++){
            // check for time in blocked state
            if(Time-BlockedQueue[i].time_in >= BlockedQueue[i].time_blocked){
                BlockedQueue[i].cum_blocked+=Time-BlockedQueue[i].time_in;
                // pop from BlockedQueue to ReadyQueue
                ReadyQueue.push_back(BlockedQueue[i]);
                BlockedQueue.erase(BlockedQueue.begin()+i);
                ReadyQueue.back().time_in=Time;
                i--;
                cout << "Time: " << Time <<" Process: " << ReadyQueue.back().id << " from Blocked state to Ready state" << endl;
            }
        }
        sem_post(&semRunning);
    }
}

// Running state thread
void * RunningThread(void *arg){
    while(Time <= time_sim){
        sem_wait(&semRunning);
        for (int i=0; i<RunningQueue.size(); i++){
            // check for time in running state
            if(Time-RunningQueue[i].time_in >= RunningQueue[i].time_burst){
                RunningQueue[i].num_burst_current++;
                RunningQueue[i].cum_running+=Time-RunningQueue[i].time_in;
                // check number of current burst
                if(RunningQueue[i].num_burst_current>=RunningQueue[i].num_burst){
                    // pop from RunningQueue to ExitQueue
                    ExitQueue.push_back(RunningQueue[i]);
                    RunningQueue.erase(RunningQueue.begin()+i);
                    ExitQueue.back().time_in=Time;
                    cout << "Time: " << Time <<" Process: " << ExitQueue.back().id << " from Running state to Exit state" << endl;
                }
                else{
                    // pop from RunningQueue to BlockedQueue
                    BlockedQueue.push_back(RunningQueue[i]);
                    RunningQueue.erase(RunningQueue.begin()+i);
                    BlockedQueue.back().time_in=Time;
                    cout << "Time: " << Time <<" Process: " << BlockedQueue.back().id << " from Running state to Blocked state" << endl;
                }
                i--;
            }
        }
        sem_post(&semReady);
    }
}

// Ready state thread
void* ReadyThread(void *arg){
    while(Time <= time_sim){
        sem_wait(&semReady);
        // check if runing state is avalible
        if(RunningQueue.size()==0 && ReadyQueue.size()>0){
            ReadyQueue.front().cum_ready+=Time-ReadyQueue.front().time_in;
            // pop from ReadyQueue to RunningQueue
            RunningQueue.push_back(ReadyQueue.front());
            ReadyQueue.pop_front();
            RunningQueue.back().time_in=Time;
            cout << "Time: " << Time <<" Process: " << RunningQueue.back().id << " from Ready state to Running state" << endl;
        }
        sem_post(&semExit);
    }
}

// Exit state thread
void* ExitThread(void *arg){
    while(Time <= time_sim){
        sem_wait(&semExit);
        // check if process in ExitQueue
        while(ExitQueue.size()>0){
            ExitQueue.front().time_turnaround=Time-ExitQueue.front().time_arrival;
            ExitQueue.front().time_departure=Time;
            print_process(ExitQueue.front());
            // Release PCB
            FreePCB++;
            cout << "Time: " << Time <<" Process: " << ExitQueue.front().id << " remove from Exit state" << endl;
            // pop process from ExitQueue
            ExitQueue.pop_front();
        }
    }
}

int main(int argc, char **argv)
{  
    char text[]="************************************************************\n"
                "  This is an example for project 2. It is not perfect\n"
                "  but have most of the functions done. You can play with\n"
                "  it and better understand the requests of the project.\n"
                "                                wangdin1@cse.msu.edu        \n"
                "************************************************************";
    cout << text <<endl;
    sleep(1);
    N = atoi(argv[1]);
    ifstream in(argv[2]);

    char buf[100];
    process p={0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // read parameters
    in>>num_CPU;
    in>>num_PCB;
    in>>time_sim;
    in>>buf;
    in>>p.id>>p.priority>>p.num_burst>>p.time_burst>>p.time_blocked>>p.time_arrival;
    // push them in ProcessQueue
    while(in){
        ProcessQueue.push_back(p);
        in>>p.id>>p.priority>>p.num_burst>>p.time_burst>>p.time_blocked>>p.time_arrival;
    }

    // semaphores init
    sem_init(&semTimer, 0, 1);
    sem_init(&semRunning, 0, 0);
    sem_init(&semBlocked, 0, 0);
    sem_init(&semReady, 0, 0);
    sem_init(&semExit, 0, 0);

    // create threads
    pthread_t threads[6];

    if (pthread_create(&threads[0], NULL, ClockThread, NULL))
        cout << "pthread_create failed" << endl;
    if (pthread_create(&threads[1], NULL, NewThread, NULL))
        cout << "pthread_create failed" << endl;
    if (pthread_create(&threads[2], NULL, BlockedThread, NULL))
        cout << "pthread_create failed" << endl;
    if (pthread_create(&threads[3], NULL, RunningThread, NULL))
        cout << "pthread_create failed" << endl;
    if (pthread_create(&threads[4], NULL, ReadyThread, NULL))
        cout << "pthread_create failed" << endl;
    if (pthread_create(&threads[5], NULL, ExitThread, NULL))
        cout << "pthread_create failed" << endl;

    for (int i=0; i<6; i++)
        if (pthread_join(threads[i], NULL))
            cout << "pthread_join failed" << endl;

}
