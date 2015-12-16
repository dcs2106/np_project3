#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3
#define MaxClientNum 5

int main(int argc, char* argv[], char *envp[])
{
	int index=0;
	int conn=0;
	char ip[MaxClientNum][50];
	char port[MaxClientNum][20];
	char file[MaxClientNum][20];
	
	char *queryString=getenv("QUERY_STRING");
	char *temp;
	temp=strtok(queryString,"&");
	
	while(temp!=NULL){
		char head;
		char *parameter;
		head=temp[0];
		if(head=='h'){
			parameter=strchr(temp,'=');
			strcpy(ip[index],parameter+1);
		}
		else if(head=='p'){
			parameter=strchr(temp,'=');
			strcpy(port[index],parameter+1);
		}
		else if(head=='f'){
			parameter=strchr(temp,'=');
			strcpy(file[index],parameter+1);
			index++;
		}
		else{
			fprintf(stderr,"parameter error");
			fflush(stderr);
		}
		temp=strtok(NULL,"&");
	}
	
	char* s = "Test CGI";
	printf("Content-type: text/html\n\n");
	printf("<html>");
	printf("<body>");
	printf("<h2>%s</h2>", s);
	printf("</body>");
	printf("</html>");
}