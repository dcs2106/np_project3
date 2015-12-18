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

int readline(int fd, char * ptr, int maxlen);
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
	listen(sockfd,MaxHost);//listen
	printf("SERVER_PORT: %d\n",port);
	
	
	
	for(;;){
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		
		clientfd = accept(sockfd,(struct sockaddr *)&client_addr, &addrlen);
		
		int childpid;
		
		if((childpid=fork()) < 0){
			printf("fork error\n");
		}
		else if(childpid ==0){//child process
			
		}
		else{//parent process
			close(sockfd);
		}
		
	}
	
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
