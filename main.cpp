#include<stdio.h>
#include<iostream>
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
#include "util.h"
#include "ringlist.h"
typedef long long int lli;

typedef struct mems *Mems;
struct mems{
    char* vmpeak;
    char* vmsize;
    char* vmrss;
};
static volatile int handled_pid = 0;
static volatile int handled_status = 0;

static void handler(int sig)
{
    handled_pid = wait((int*)&handled_status);
}

lli get_child_cpustate(char *proc1){
    char proc[30];
    strcpy(proc,proc1);
    char stat[]="/stat";
    strcat(proc,stat);
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
    return m;
}


void display_mems(int pid)
{
    time_t begintime;
    time(&begintime);
    time_t nowtime;
    char proc[50]="/proc/";
    char p[20]={'\0'};
    sprintf(p,"%d",pid);
    strcat(proc,p);
    int status;

    while(1){
        int st = handled_status;
        printf("%d\n",handled_pid);
        if(handled_pid == -1) printf("%s",strerror(errno));
        if(handled_pid>=0){
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
        lli p_cpu1 = get_child_cpustate(proc);
        lli c_cpu1 = get_cpustate();
        sleep(2);
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

int execute3(std::string oprand[],int size, int in, int out){
    int fork_result = fork();
    switch(fork_result) {
        case -1: // error
            perror("fork");
            return -1;
        case 0: // child
            dup2(in,0);
            dup2(out,1);
            char *args[100];
            char tmp[100][100];
            for(int i=0;i<size;i++){
                strcpy(tmp[i],oprand[i].c_str());
                args[i]=tmp[i];
            }
            args[size]=NULL;
            execvp(args[0],args);
            perror("execvp");
            exit(-1);
        default: // parent
            display_mems(fork_result);
            return fork_result;
    }


}

void execute2(ringlist *childpid, std::string oprand[],int size,int in, int out){
    //リダイレクトとパイプライン処理
int q=0;
int last=size;
for(q=0;q<size;q++){
    if(oprand[q]=="<"){
        last = min(last, q);
        if(in != 0) close(in);
        in = open(oprand[q+1].c_str(),O_RDONLY);
        q++;
    }
    else if(oprand[q]==">"){
        last = min(last, q);
        if(out!= 1) close(out);
        out = open(oprand[q+1].c_str(),O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWGRP | S_IWUSR);
        q++;
    }
    else if(oprand[q]=="|"){
        last = min(last, q);
        int pipefd[2];
        int t;
        t = pipe2(pipefd,O_CLOEXEC);
        execute2(childpid,oprand+q+1,size-q-1,pipefd[0],1);
        if(out!= 1) close(out);
        out = pipefd[1];
        break;
    }
}
//ここまで
int cpid = execute3(oprand,last,in,out);
if(in != 0) close(in);
if(out!= 1) close(out);

if(cpid != -1) ringlist_add(childpid, cpid);
}

void execute(std::string oprand[],int size) {
    ringlist *childpid = ringlist_init();
    execute2(childpid,oprand,size,0,1);

    ringlist *it;
    for(it = childpid; it->next; ) {
        int status;
        if(!waitpid(it->num,&status,WUNTRACED | WNOHANG)) {
            it = it->next;
            continue;
        }
        it = ringlist_erase(it);

        //printf("child process done.\n");
        if(WIFEXITED(status)){
            //printf("child process exit status=%d\n",WEXITSTATUS(status));
        }else{
            //printf("exit abnormally\n");
        }
    }
    free(it);
    //printf("all children exited\n");
    return;

}
void SigHandler(int p_signame){
    signal(SIGINT,SigHandler);	
    printf("\n\033[%dm>>\033[0m",31);
    fflush(stdout);
    return;
}	
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
void uparrow(int x){
    FILE *fp;
    char s[256];
    char *ss[300];
    if((fp=fopen(".shrc","r"))==NULL){
        printf("file open error!\n");
        return;
    }
    int i=0;
    while(fgets(s,256,fp)!=NULL){
        ss[i]=(char*)malloc(sizeof(s));
        strcpy(ss[i],s);
        i++;
    }
    if(i<x+1){ return;}
    char ns[256];
    strcpy(ns,ss[i-x-1]);

    int j;
    for(j=0;j<i;j++) free(ss[j]);

    int q=0;
    while(1){
        if(ns[q]=='\n'){ ns[q]='\0'; break;}
        q++;
    }
    if(x==0)printf("%s",ns);
    else{
        printf("\r                       ");
        printf("\r\033[%dm>>\033[0m%s",31,ns);
    }

    char c = 0;
    while(1){
        if(kbhit()){
            c=getchar();
            if(c=='\x1B'){ 
                c=getchar();
                c=getchar();
                fflush(stdout);
                fclose(fp);
                uparrow(x+1);
                break;}
        }else if(c == '\n'){
            char tmp[100];
            strcpy(tmp,ns);
            char *tp;
            tp=strtok(tmp," ");
            std::string ops[10];
            int i=0;
            while(tp!=NULL){
                ops[i]=std::string(tp);
                i++;
                tp=strtok(NULL," ");
            }
            printf("\n");
            fclose(fp);
            execute(ops,i);
            break;
        }
    }

    return;
}

int main(){
    signal(SIGINT,SigHandler);	
    signal(SIGCHLD,handler);

    while(1){
        printf("\033[%dm>>\033[0m",31);
        fflush(stdout);
        std::string oprand;
        int c;
        while(1){
            if(kbhit()){
                c=getchar();
                if(c=='\x1B'){ 
                    c=getchar();
                    c=getchar();
                    oprand="\x1B\x5b";
                    oprand+=(char)c;
                    break;}
                else{
                    ungetc(c,stdin);
                    printf("%c",c);
                    std::getline(std::cin,oprand);
                    break;
                }
            }
        }

        if(oprand[0]!='\x1B'){
            FILE *fp;
            fp=fopen(".shrc","a");
            if(fp==NULL){ printf("cannot open .shrc\n");}
            fputs((oprand+"\n").c_str(),fp);
            fclose(fp);
        }

        char tmp[100];
        strcpy(tmp,oprand.c_str());
        char *tp;
        tp=strtok(tmp," ");
        std::string ops[10];
        int i=0;
        while(tp!=NULL){
            ops[i]=std::string(tp);
            i++;
            tp=strtok(NULL," ");
        }

        if(ops[0]=="\x1B\x5b\x41"){
            uparrow(0);
        }else if(ops[0]=="cd"){
            int t;
            t = chdir(ops[1].c_str());
        }else if(ops[0]=="exit" || ops[0]=="quit" || ops[0]=="q"){
            unlink(".shrc");
            return 0;
        }else{
            execute(ops,i);
        }

    }

    return 0;
}
