#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<string.h>
#include<signal.h>
#include<fcntl.h>
#include<termios.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<time.h>
#include<errno.h>
typedef long long int lli;

typedef struct mems *Mems;
struct mems{
    char* vmpeak;
    char* vmsize;
    char* vmrss;
};

lli get_child_cpustate(char *proc1);
lli get_cpustate(void);
Mems get_mem_state(char *proc1);
void display_mems(int pid,int handled_status,int handled_pid);
