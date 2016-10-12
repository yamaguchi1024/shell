#include<cstdio>
#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<string.h>
#include<signal.h>
#include<fcntl.h>

std::string path(std::string opr){
	char env[100];
	char *tmp=getenv("PATH");
	strcpy(env,tmp);
	char *tp;
	tp=strtok(env,":");
	std::string paths[10];
	int i=0;
	while(tp!=NULL){
		paths[i]=std::string(tp);
		i++;
		tp=strtok(NULL,":");
	}
	std::string rtn=opr;
	for(int j=0;j<i;j++){
		if(!access((paths[j]+"/"+opr).c_str(),X_OK)){
			rtn=paths[j]+"/"+opr;
			break;
		}
	}
	return rtn;
}
void execute2(std::string oprand[],int size){

	char *args[100];
	char tmp[100][100];
	for(int i=0;i<size;i++){
		strcpy(tmp[i],oprand[i].c_str());
		args[i]=tmp[i];
	}
	args[size]=NULL;
	printf("execvp exit status%d\n",execvp(args[0],args));

}

void execute(std::string oprand[],int size){
	int fork_result = fork();
	//リダイレクトとパイプライン処理
	if(fork_result==0){
		int q=0;
		for(q=0;q<size;q++){
			if(oprand[q]=="<"){
				int in = open(oprand[q+1].c_str(),O_RDONLY);
				dup2(in,0);
				close(in);
				execute2(oprand,q);
			}
			else if(oprand[q]==">"){
				int out = open(oprand[q+1].c_str(),O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWGRP | S_IWUSR);
				dup2(out,1);
				close(out);
				execute2(oprand,q);
			}
			else if(oprand[q]=="|"){
				int out = open("tmp",O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWGRP | S_IWUSR);
				dup2(out,1);
				close(out);
				execute2(oprand,q);
				int in = open("tmp",O_RDONLY);
				dup2(in,0);
				close(in);
				execute2(oprand+q+1,size);

			}
		}
		//ここまで
		if(size==q){
		       	execute2(oprand,size);
		}
		//std::cout<< "created pid" <<  getpid() << std::endl;
		//printf("%d\n",execl("/bin/ls","/bin/ls",NULL));
		exit(0);
	}
	else if(fork_result==-1) printf("Fork Failed!\n");

	//std::cout <<"Child pid" << fork_result << std::endl;
	int status;
	waitpid(fork_result,&status,WUNTRACED);
	printf("child process done.\n");

	if(WIFEXITED(status)){
		printf("child process exit status=%d\n",WEXITSTATUS(status));
	}else{
		printf("exit abnormally\n");
	}
}
void SigHandler(int p_signame){
	signal(SIGINT,SigHandler);	
	printf("\n\033[%dm>>\033[0m",31);
	std::cout << std::flush ;
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
