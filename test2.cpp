#include<cstdio>
#include<iostream>
#include<vector>
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

void execute2(std::vector<int>& childpid, std::string oprand[],int size,int in, int out){
    //リダイレクトとパイプライン処理
    int q=0;
    int last=size;
    for(q=0;q<size;q++){
        if(oprand[q]=="<"){
            last = std::min(last, q);
            if(in != 0) close(in);
            in = open(oprand[q+1].c_str(),O_RDONLY);
            q++;
        }
        else if(oprand[q]==">"){
            last = std::min(last, q);
            if(out!= 1) close(out);
            out = open(oprand[q+1].c_str(),O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWGRP | S_IWUSR);
            q++;
        }
        else if(oprand[q]=="|"){
            last = std::min(last, q);
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
        childpid.push_back(cpid);
}

void execute(std::string oprand[],int size) {
    std::vector<int> children;
    execute2(children,oprand,size,0,1);

    while(children.size())
        for(std::vector<int>::iterator it = children.begin(); it != children.end(); ) {
            int status;
            if(!waitpid(*it,&status,WUNTRACED | WNOHANG)) {
                it++;
                continue;
            }
            children.erase(it);

            printf("child process done.\n");
            if(WIFEXITED(status)){
                printf("child process exit status=%d\n",WEXITSTATUS(status));
            }else{
                printf("exit abnormally\n");
            }
        }
    printf("all children exited\n");
    return;

}
void SigHandler(int p_signame){
    signal(SIGINT,SigHandler);	
    printf("\n\033[%dm>>\033[0m",31);
    std::cout << std::flush ;
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
		//fflush(stdin);
	}
	if(i<x+1){ return;}
	strcpy(s,ss[i-x-1]);
	int q=0;
	while(1){
		if(s[q]=='\n'){ s[q]='\0'; break;}
		q++;
	}
	printf("%s",s);
	while(1){
		if(kbhit()){
			int c;
			c=getchar();
			if(c=='\n'){
				char tmp[100];
				strcpy(tmp,s);
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
				execute(ops,i);
				break;
			}else{
				while(getchar()!=EOF){
				}
				break;
			}
		}
	}

	int j;
	for(j=0;j<i;j++) free(ss[j]);

	fclose(fp);
	return;
}

int main(){
	signal(SIGINT,SigHandler);	
	int uparrow_cnt=-1;

	while(1){
		printf("\033[%dm>>\033[0m",31);
		std::string oprand;
		int c;
		while(1){
			if(kbhit()){
				c=getchar();
				if(c=='\x1B'){ 
					c=getchar();
					c=getchar();
					oprand="\x1B[";
					oprand+=(char)c;
					break;}
				else{printf("%c",c);}
				if(c=='\n') break;
				oprand+=(char)c;
			}
		}

		//fileに書き込み
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

		if(ops[0]=="\x1B[A"){
			uparrow_cnt++;
			uparrow(uparrow_cnt);
		}else if(ops[0]=="cd"){
			uparrow_cnt=-1;
			chdir(ops[1].c_str());
		}else if(ops[0]=="exit" || ops[0]=="quit" || ops[0]=="q"){
			unlink(".shrc");
			return 0;
		}else{
			uparrow_cnt=-1;
			execute(ops,i);
		}

	}

	return 0;
}
