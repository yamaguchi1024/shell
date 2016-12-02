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

#include "util.h"
#include "ringlist.h"

typedef long long int lli;
lli get_child_cpustate(char *proc){
    FILE *fp;
    if((fp=fopen(proc,"r"))==NULL){ printf("stat open error!\n"); return -1;}
    int i=0;
    char state[20];
    lli utime;
    lli stime;
    for(i=0;i<15;i++){
        if(i==13) fscanf(fp,"%lld",&utime);
        if(i==14) fscanf(fp,"%lld",&stime);
        else fscanf(fp,"%s",state);
    }
    fclose(fp);
    return utime+stime;
}

lli get_cpustate(void){
    FILE *fp;
    if((fp=fopen("/proc/stat","r"))==NULL){ printf("/proc/stat open error!\n"); return -1;}
    char tmp[20];
    fscanf(fp,"%s",tmp);
    int i=0;
    lli time;
    lli res;
    for(i=0;i<10;i++){
        fscanf(fp,"%lld",&time);
        res+=time;
    }
    fclose(fp);
    return res;
}


void display_mems(int pid)
{
    char proc[]="/proc/";
    char stat[]="/stat";
    char p[20]={'\0'};
    sprintf(p,"%d",pid);
    strcat(proc,p);
    strcat(proc,stat);
    printf("%s\n",proc);

    lli p_cpu1 = get_child_cpustate(proc);
    lli c_cpu1 = get_cpustate();
    sleep(5);
    lli p_cpu2 = get_child_cpustate(proc);
    lli c_cpu2 = get_cpustate();
    printf("p_cpu1: %lld p_cpu2: %lld c_cpu1: %lld c_cpu2: %lld\n",p_cpu1,p_cpu2,c_cpu1,c_cpu2);
    printf("cpu: %f\n",(float)(p_cpu2-p_cpu1)/(c_cpu2-c_cpu1));
    printf("mama!!\n");

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
        pipe2(pipefd,O_CLOEXEC);
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
            chdir(ops[1].c_str());
        }else if(ops[0]=="exit" || ops[0]=="quit" || ops[0]=="q"){
            unlink(".shrc");
            return 0;
        }else{
            execute(ops,i);
        }

    }

    return 0;
}
