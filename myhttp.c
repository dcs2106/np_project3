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

#define  MaxHost 5
#define Maxlinelen 1024

int readline(int fd, char * ptr, int maxlen);
void clear_array(char array[],int len);
int connectsock(char *service, char *protocol);
int main(int argc , char *argv[])
{
	int port;
	int sockfd;
	int clientfd;
	struct sockaddr_in dest;
	port=atoi(argv[1]);
	
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
					front=strrchr(command,' ');
					strcpy(filename,strrchr(front,'/')+1);
					strcpy(querystring,"");
					
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
					return -1;
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
int connectsock(char *service, char *protocol)
{
	int s,type;
	struct servent *pse;
	struct protoent *ppe;
	struct sockaddr_in dest;
	int portbase = 0;
	
	bzero((char *)&dest,sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = INADDR_ANY;
	//dest.sin_port = htons(port);
	
	if(pse = getservbyname(service,protocol)){
		//dest.sin_port = htons(port);
		dest.sin_port=htons(ntohs((u_short)pse->s_port) + portbase);
	}
	else if((dest.sin_port = htons((u_short)atoi(service)))==0){
		exit(1);
	}
	if((ppe = getprotobyname(protocol)) == 0){
        fprintf(stderr, "can't get \"%s\" protocol entry\n", protocol); 
        fflush(stderr);
        exit(1);
    }
	
	if(strcmp(protocol,"udp")==0 ) {
		type = SOCK_DGRAM;
	}
	else{
		type = SOCK_STREAM;
	}
	
	s = socket(PF_INET,type,ppe->p_proto);
	bind(s, (struct sockaddr *)&dest, sizeof(dest));
	listen(s, MaxHost);
	
	return s;
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