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

#include "util.h"
#include "ringlist.h"

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

    if(cpid != -1)
        ringlist_add(childpid, cpid);
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

        printf("child process done.\n");
        if(WIFEXITED(status)){
            printf("child process exit status=%d\n",WEXITSTATUS(status));
        }else{
            printf("exit abnormally\n");
        }
    }
    free(it);
    printf("all children exited\n");
    return;

}
void SigHandler(int p_signame){
    signal(SIGINT,SigHandler);	
    printf("\n\033[%dm>>\033[0m",31);
    fflush(stdout);
    return;
}	

int main(){
    signal(SIGINT,SigHandler);	

    while(1){
        printf("\033[%dm>>\033[0m",31);
        std::string oprand;
        std::getline(std::cin,oprand);
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

        if(ops[0]=="cd"){
            //printf("change dir: %d",chdir(ops[1].c_str()));
            chdir(ops[1].c_str());
        }else if(ops[0]=="exit" || ops[0]=="quit"){
            return 0;
        }else{
            //std::cout << "ops[0]: " << ops[0] << "ops[1]: " << ops[1] << "i: " << i << std::endl;
            execute(ops,i);
        }

    }

    return 0;
}
