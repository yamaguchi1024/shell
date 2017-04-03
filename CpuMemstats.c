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
#include "CpuMemstats.h"
typedef long long int lli;

lli get_child_cpustate(char *proc1){
    char proc[30];
    strcpy(proc,proc1);
    char stat[]="/stat";
    strcat(proc,stat);
    printf("proc %s\n",proc);
    FILE *fp;
    if((fp=fopen(proc,"r"))==NULL){ printf("stat open error!\n"); return -1;}
    int i=0;
    char state[20];
    lli utime;
    lli stime;
    int t;
    for(i=0;i<15;i++){
        if(i==13) t = fscanf(fp,"%lld",&utime);
        if(i==14) t = fscanf(fp,"%lld",&stime);
        else t = fscanf(fp,"%s",state);
    }
    fclose(fp);
    return utime+stime;
}

lli get_cpustate(void){
    FILE *fp;
    if((fp = fopen("/proc/stat","r")) == NULL){ printf("/proc/stat open error!\n"); }
    char tmp[20];
    int t;
    t = fscanf(fp,"%s",tmp);
    int i=0;
    lli time;
    lli res;
    for(i=0;i<10;i++){
        t = fscanf(fp,"%lld",&time);
        res +=time;
    }
    fclose(fp);
    return res;
}

Mems get_mem_state(char *proc1){
    char proc[50];
    strcpy(proc,proc1);
    char stats[]="/status";
    strcat(proc,stats);
    printf("proc %s\n",proc);
    FILE *fp;
    if((fp = fopen(proc,"r"))==NULL){ printf("status open error!\n");}
    Mems m = (Mems)malloc(100);
    int i=0;
    char g[200];
    m->vmpeak = (char*)malloc(100);
    m->vmsize = (char*)malloc(100);
    m->vmrss = (char*)malloc(100);
    char vp[50]="VmPeak:";
    char vs[50]="VmSize:";
    char vr[50]="VmRSS:";
    int t;
    while(1){
        t = fscanf(fp,"%s",g);
        if(!strcmp(g,vp)){ 
            t = fscanf(fp,"%s",g);
            strcat(vp,g);
            t = fscanf(fp,"%s",g);
            strcat(vp,g);
            strcpy(m->vmpeak,vp);
        }
        if(!strcmp(g,vs)){ 
            t = fscanf(fp,"%s",g);
            strcat(vs,g);
            t = fscanf(fp,"%s",g);
            strcat(vs,g);
            m->vmsize = vs;
            strcpy(m->vmsize,vs);
        }
        if(!strcmp(g,vr)){ 
            t = fscanf(fp,"%s",g);
            strcat(vr,g);
            t = fscanf(fp,"%s",g);
            strcat(vr,g);
            m->vmrss =vr;
            strcpy(m->vmrss,vr);
            break;
        }
    }
    fclose(fp);
    return m;
}


//This function is called by main.cpp exucution3.
void display_mems(int pid, int handled_status, int handled_pid)
{
    time_t begintime;
    time(&begintime);
    time_t nowtime;
    char proc[50]="/proc/";
    char p[20]={'\0'};
    sprintf(p,"%d",pid);
    strcat(proc,p);
    int status;

    //if pid doesn't exist, break;
    while(!kill(pid,0)){
        int st = handled_status;
        if(handled_pid == -1) printf("%s",strerror(errno));
        if(handled_pid>0){
            if (WIFEXITED(st)) {
                printf("child exited: %d\n", WEXITSTATUS(st));
                break;
            } else if (WIFSIGNALED(st)) {
                printf("child signaled: %d\n", WTERMSIG(st));
                break;
            } else if (WIFSTOPPED(st)) {
                printf("child signaled: %d\n", WSTOPSIG(st));
                break;
            }
        }
        //call functions to get values of cpu time and memory status
        lli p_cpu1 = get_child_cpustate(proc);
        lli c_cpu1 = get_cpustate();
        sleep(2);
        if(kill(pid,0)) break;
        lli p_cpu2 = get_child_cpustate(proc);
        lli c_cpu2 = get_cpustate();
        printf("cpu utilization: %f\n",(float)(p_cpu2-p_cpu1)/(c_cpu2-c_cpu1));
        Mems c_mem = get_mem_state(proc);
        printf("%s %s %s\n",c_mem->vmpeak,c_mem->vmsize,c_mem->vmrss);
        time(&nowtime);
        printf("passed time: %f\n\n",difftime(nowtime,begintime));
    }
        return;
}
