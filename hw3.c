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

#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3
#define MaxClientNum 5

int linelen(int fd,char *ptr,int maxlen);
void clean_array(char array[],int size);
int main(int argc, char* argv[], char *envp[])
{
	char ip[MaxClientNum][50];
	char port[MaxClientNum][20];
	char file[MaxClientNum][20];
	for(int i=0;i<MaxClientNum;i++){
		clean_array(ip[i],50);
		clean_array(port[i],20);
		clean_array(file[i],20);
	}
	
	char *queryString=getenv("QUERY_STRING");
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
			
			connect(csock[i],(struct sockaddr *)&dest[i],sizeof(dest[i]));
			
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
		
		printf("</table>\n");
	printf("<body>\n");
	printf("<h2>%s</h2>", s);
	printf("</body>");
	printf("</html>");
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