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
#include <ctype.h>
#include <fcntl.h>
#include <arpa/inet.h>
#define  MaxHost 5
#define Maxlinelen 1024

int readline(int fd, char * ptr, int maxlen);
void clear_array(char array[],int len);
int connectsock(char *service, char *protocol);
int check(struct sockaddr_in client_addr);
int main(int argc , char *argv[])
{
	int port;
	int sockfd;
	int clientfd;
	struct sockaddr_in dest;
	port=atoi(argv[1]);
	clearenv();
	chdir("/net/gcs/104/0456115/np_project3");
	
	bzero((char *)&dest,sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	dest.sin_addr.s_addr = INADDR_ANY;
	
	sockfd = socket(PF_INET,SOCK_STREAM,0);
	if(sockfd < 0){//socket
		fprintf(stderr,"socket error\n");
		exit(1);
	}
	
	bind(sockfd,(struct sockaddr*)&dest,sizeof(dest));
	listen(sockfd,MaxHost);//listen*/
	//sockfd = connectsock(argv[1],"tcp");
	printf("SERVER_PORT: %d\n",port);
	
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	
	for(;;){
		
		clientfd = accept(sockfd,(struct sockaddr *)&client_addr, &addrlen);
		
		int check1=check(client_addr);
		
		int childpid;
		childpid = fork();
		if(childpid==-1){
			printf("fork error\n");
		}
		else if(childpid == 0){//child process
			char buf[Maxlinelen];
			char command[Maxlinelen];
			char filename[Maxlinelen];
			char querystring[Maxlinelen];
			clear_array(command,Maxlinelen);
			clear_array(buf,Maxlinelen);
			clear_array(filename,Maxlinelen);
			clear_array(querystring,Maxlinelen);
			
			read(clientfd,buf,Maxlinelen);
			strcpy(command,strtok(buf,"\r\n"));
			
			if(strstr(command,".cgi")!=NULL){
				if(check1==0){
					if(strstr(command,"?")!=NULL){//GET http://java.csie/hello.cgi?a=b&c=d HTTP/1.1
						char *front;
						char *back;
						front=strtok(command,"?");//GET.........cgi
						back=strtok(NULL,"?");
						strcpy(filename,strrchr(front,'/')+1);//hello.cgi
						strcpy(querystring,strtok(back," "));//a=aa&b=bb
					}
					else{//GET http://java.csie/hello.cgi HTTP/1.1
						char *front;
						front=strtok(command," ");//GET
						front=strtok(NULL," ");//http://java.csie/hello.cgi
						strcpy(filename,strrchr(front,'/')+1);
						strcpy(querystring,"");
						
						//printf("cgiFileName: %s\n", filename);
						//printf("queryString: %s\n", querystring);
						//fflush(stdout);
						
					}
					
					setenv("QUERY_STRING",querystring,1);
					setenv("REQUEST_METHOD","GET",1);
					setenv("SCRIPT_NAME","/net/gcs/104/0456115/public_html/hello.cgi",1);
					setenv("REMOTE_HOST","nplinux0.cs.nctu.edu.tw",1);
					setenv("REMOTE_ADDR","140.113.216.139",1);
					setenv("CONTENT_LENGTH","4096",1);
					setenv("AUTH_TYPE","http",1);
					setenv("REMOTE_USER","Jian_De",1);
					setenv("REMOTE_IDENT","Jian_De",1);
					
					char filepath[Maxlinelen];
					clear_array(filepath,Maxlinelen);
					strcpy(filepath,filename);
					
					if(access(filepath,F_OK)==0){
						char status[Maxlinelen];
						clear_array(status,Maxlinelen);
						strcpy(status,"HTTP/1.1 200 OK\r\n");//return status and then fork & exec test.cgi
						
						write(clientfd,status,strlen(status));
						int cgipid;
						cgipid=fork();
						if(cgipid==-1){
							printf("cgi fork error\n");
						}
						else if (cgipid==0){
							dup2(clientfd,fileno(stdout));
							
							execl(filepath,(char *)NULL);
							exit(1);
						}
						else{
							int *status_;
							wait(status_);
							return 0;
						}
					}
					else{
						char status[Maxlinelen];
						clear_array(status,Maxlinelen);
						strcpy(status,"HTTP/1.1  404 Not Found\r\n");
						write(clientfd,status,strlen(status));
						close(clientfd);
						return 0;
					}
				}
				else{
					char *connect="Connection denied\n";
					char *status;
					strcpy(status,"HTTP/1.1 200 OK\r\n");
					write(clientfd,status,strlen(status));
					write(clientfd,connect,strlen(connect));
					return 0;
				}
			}
			else if(strstr(command,".html")!=NULL){
				if(check1==0){
					int file_fd;
					char *str;//GET http://java.csie/form_get.html HTTP/1.1
					char filename[Maxlinelen];
					str=strtok(command," ");//GET
					str=strtok(NULL," ");//http://java.csie/form_get.html
					
					strcpy(filename,strrchr(str,'/')+1);
					
					file_fd = open(filename,O_RDONLY);
					if(file_fd != -1){
						char response[Maxlinelen];
						char *status = "HTTP/1.1 200 OK\r\nContent-Type: text/html\n\n";
						
						write(clientfd,status,strlen(status));
						
						while(readline(file_fd,response,sizeof(response))){//from formget.html
							write(clientfd,response,strlen(response));//write to browser
						}
						close(clientfd);
						return 0;	
					}
					else{
						char *status = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\n\n";
						write(clientfd,status,strlen(status));

						close(clientfd);
						return 0;
					}
				}
				else{
					char *connect="Connection denied\n";
					char *status;
					strcpy(status,"HTTP/1.1 200 OK\r\n");
					write(clientfd,status,strlen(status));
					write(clientfd,connect,strlen(connect));
					return 0;
				}
			}
		}
		else{//parent process
			close(clientfd);
			continue;
		}
	}
	return 0;
}
int readline(int fd, char * ptr, int maxlen){ 
    int  n, rc;
    char  c; 
    
    for (n = 1; n < maxlen; n++) { 
        if ( (rc = read(fd, &c, 1)) == 1) { 
            *ptr++ = c; 
            if (c == '\n')      break; 
        }
        else if (rc == 0) { 
            if (n == 1) return 0;  /* EOF, no data read */ 
            else  break;  /* EOF, some data was read */ 
        }
        else return -1;  /* error */ 
    } 
    *ptr = 0;
    
    return(n); 
} 
void clear_array(char array[],int len)
{
	for(int i=0;i<len;i++){
		array[i]='\0';
	}
}
int check(struct sockaddr_in client_addr)
{
	char ip[256][50];
	for(int i=0;i<256;i++){
		sprintf(ip[i],"140.113.167.%d",i);
	}
	//char *ip="140.113.167.";
	char *ip2 = inet_ntoa(client_addr.sin_addr);
	
	for(int i=0;i<256;i++){
		if(strcmp(ip[i],ip2)==0)return 0;//140.113.167.*
	}
	
	return -1;//others
}