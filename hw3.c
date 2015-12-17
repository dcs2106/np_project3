#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3
#define MaxClientNum 5
#define MaxCommandLen 15000

int linelen(int fd,char *ptr,int maxlen);
void clean_array(char array[],int size);
int main(int argc, char* argv[], char *envp[])
{
	char ip[MaxClientNum][50];
	char port[MaxClientNum][20];
	char file[MaxClientNum][20];
	int conn=0;
	for(int i=0;i<MaxClientNum;i++){
		clean_array(ip[i],50);
		clean_array(port[i],20);
		clean_array(file[i],20);
	}
	
	char *queryString=getenv("QUERY_STRING");
	setenv("REQUEST_METHOD","GET",1);
	setenv("SCRIPT_NAME","/net/gcs/104/0456115/public_html/hello.cgi",1);
	setenv("REMOTE_HOST","nplinux0.cs.nctu.edu.tw",1);
	setenv("REMOTE_ADDR","140.113.216.139",1);
	setenv("CONTENT_LENGTH","15000",1);
	setenv("AUTH_TYPE","http",1);
	setenv("REMOTE_USER","Jian_De",1);
	setenv("REMOTE_IDENT","Jian_De",1);
	
	
	char *temp;
	temp=strtok(queryString,"&");
	
	while(temp!=NULL){
		char head;
		char *parameter;
		int num;
		head=temp[0];
		num=atoi(temp+1)-1;
		if(head=='h'){
			parameter=strchr(temp,'=');
			strcpy(ip[num],parameter+1);
		}
		else if(head=='p'){
			parameter=strchr(temp,'=');
			strcpy(port[num],parameter+1);
		}
		else if(head=='f'){
			parameter=strchr(temp,'=');
			strcpy(file[num],parameter+1);
		}
		else{
			fprintf(stderr,"parameter error");
			fflush(stderr);
		}
		temp=strtok(NULL,"&");
	}
	//************************************************ parser parameter end *****************************************
	//************************************************ initial client ***********************************************
	int csock[MaxClientNum];
	int file_fd[MaxClientNum];
	struct sockaddr_in dest[MaxClientNum];
	for(int i=0 ; i < MaxClientNum ; i++){
		csock[i]=-1;
		
		if(strcmp(ip[i],"")!=0 && strcmp(port[i],"")!=0 && strcmp(file[i],"")!=0 ){
			char filepath[100];
			strcpy(filepath,"/net/gcs/104/0456115/");
			strcat(filepath,file[i]);
			
			file_fd[i]=open(filepath,O_RDONLY);
			
			bzero(&dest[i],sizeof(dest[i]));
			dest[i].sin_family = AF_INET;
			dest[i].sin_addr.s_addr = INADDR_ANY ;
			dest[i].sin_port = htons(atoi(port[i]));
			
			csock[i] = socket(AF_INET,SOCK_STREAM,0);
			int flag = fcntl(csock[i],F_GETFL,0);
			fcntl(csock[i],F_SETFL,flag|O_NONBLOCK);
			
			if(connect(csock[i],(struct sockaddr *)&dest[i],sizeof(dest[i]))==-1){
				if(errno != EINPROGRESS){
					printf("connect error");
					close(csock[i]);
					csock[i]=-1;
				}
			}
			
			conn++;
		}
	}
	
	//****************************************start I/O*************************************************************
	
	char* s = "Test CGI";
	
	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	
		printf("<head>\n");
			printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n");
			printf("<title>Network Programming Homework 3</title>\n");
		printf("</head>\n");
	
		printf("<body bgcolor=#1E90FF>\n");
		printf("<font face=\"Courier New\" size=2 color=#FFFF99>\n");
		printf("<table width=\"800\" border=\"1\">\n");
	
		printf("<tr>\n");
			for(int i=0;i<MaxClientNum;i++){
				printf("<td>%s</td>",ip[i]);
			}
		printf("</tr>\n");
		
		printf("<tr>\n");
			for(int i=0;i<MaxClientNum;i++){
				printf("<td valign=\"top\" id=\"m%d\"></td>", i);
			}
		printf("</tr>\n");
		printf("</table>\n");
	int nfds;
	int status[MaxClientNum];
	fd_set rfds; /* readable file descriptors*/
	fd_set wfds; /* writable file descriptors*/
	fd_set rs;   /* active file descriptors*/
	fd_set ws;   /* active file descriptors*/
	
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	
	nfds=getdtablesize();
	
	for(int i=0;i<MaxClientNum;i++){
		if(csock[i]!=-1){
			status[i]=F_CONNECTING;
			FD_SET(csock[i],&rfds);
			FD_SET(csock[i],&wfds);
			conn++;
		}
		else{
			status[i]= F_DONE;
		}
	}
	
	while(conn > 0){
		memcpy(&rfds,&rs,sizeof(rfds));
		memcpy(&wfds,&ws,sizeof(wfds));
		
		select(nfds,&rfds,&wfds,(fd_set *)NULL,(struct timeval *)NULL);
		
		for(int i=0;i<MaxClientNum;i++){
			int n=sizeof(errno);
			if(status[i]==F_CONNECTING && ( FD_ISSET(csock[i], &rfds) || FD_ISSET(csock[i], &wfds) )){
				
				if(getsockopt(csock[i],SOL_SOCKET,SO_ERROR,&errno,&n)<0 || errno!=0){
					return (-1);
				}
				status[i]=F_READING;
				FD_CLR(csock[i],&ws);
			}
			else if(status[i]==F_WRITING && FD_ISSET(csock[i],&wfds)){
				char command[MaxCommandLen];
				clean_array(command,MaxCommandLen);
				if(linelen(file_fd[i],command,MaxCommandLen) < 0){
					write(csock[i],command,strlen(command));
					
					strtok(command,"\r\n");
					
					printf("<script>document.all['m%d'].innerHTML += \"%% <b>%s</b><br>\";</script>\n", i, command);
					fflush(stdout);
					
					if(strcmp(command,"exit")==0){
						status[i]=F_DONE;
						FD_CLR(csock[i],&ws);
						conn--;
					}
					else{
						status[i]=F_READING;
						FD_CLR(csock[i],&ws);
						FD_SET(csock[i],&rs);
					}
				}
				else{
					fprintf(stderr,"Command error\n");
					fflush(stderr);
				}
			}
			else if(status[i]==F_READING && FD_ISSET(csock[i],&rfds)){
				char ResultFromServer[MaxCommandLen];
				clean_array(ResultFromServer,MaxCommandLen);
				
				if(linelen(csock[i],ResultFromServer,MaxCommandLen) > 0){
					
					strtok(ResultFromServer,"\r\n");
					
					if(strstr(ResultFromServer,"%")!=NULL){
						status[i]=F_WRITING;
						FD_CLR(csock[i],&rs);
						FD_SET(csock[i],&ws);
					}
					else{
						printf("<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n", i, ResultFromServer);
						fflush(stdout);
					}
				}
				else{
					fprintf(stderr,"Read error\n");
					fflush(stderr);
				}
			}
		}
	}
	
		
		
	printf("</font>\n");
	printf("</body>\n");
	printf("</html>\n");
	
	return 0;
}
int linelen(int fd,char *ptr,int maxlen)
{
	int n,rc;
	char c;
	for(n=1;n<maxlen;n++){
		if((rc=read(fd,&c,1)) == 1){
			*ptr++ = c;	
			if(c=='\n')  break;
		}
		else if(rc==0){
			if(n==1)     return(0);
			else         break;
		}
		else
			return(-1);
	}
	*ptr=0;
	return(n);
}
void clean_array(char array[],int size)
{
	for(int i=0 ; i<size;i++){
		array[i]='\0';
	}
}