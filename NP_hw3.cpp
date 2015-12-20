#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"

#define SERVER_PORT 6435
#define  MaxHost 5
#define Maxlinelen 1024
#define WM_SOCKET_NOTIFY (WM_USER + 1)

#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3
void clear_array(char array[], int len);
int readline(SOCKET fd, char *ptr, int maxlen);
int readfile(FILE *fp, char *ptr, int maxlen);
BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static HWND hwndEdit;
	static SOCKET msock, ssock;
	static struct sockaddr_in sa;

	static int conn;
	static SOCKET csock[MaxHost];
	static FILE *fp[MaxHost];
	static struct sockaddr_in dest[MaxHost];

	int err;
	char ip[MaxHost][50];
	char port[MaxHost][20];
	char file[MaxHost][20];
	for (int i = 0; i < MaxHost; i++){
		clear_array(ip[i], 50);
		clear_array(ip[i], 20);
		clear_array(ip[i], 20);
	}

	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch (WSAGETSELECTEVENT(lParam))
			{
			case FD_ACCEPT:
				ssock = accept(msock, NULL, NULL);
				Socks.push_back(ssock);
				EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
				break;
			case FD_READ:
				//Write your code for read event here.
				char buf[Maxlinelen];
				char command[Maxlinelen];
				char filename[Maxlinelen];
				char querystring[Maxlinelen];
				clear_array(command, Maxlinelen);
				clear_array(buf, Maxlinelen);
				clear_array(filename, Maxlinelen);
				clear_array(querystring, Maxlinelen);

				recv(ssock, buf, Maxlinelen, 0);
				strcpy(command, strtok(buf, "\r\n"));

				if (strstr(command, ".html") != NULL){
					FILE *file_fd;
					char *str;//GET http://java.csie/form_get.html HTTP/1.1
					char filename[Maxlinelen];
					str = strtok(command, " ");//GET
					str = strtok(NULL, " ");//http://java.csie/form_get.html

					strcpy(filename, strrchr(str, '/') + 1);

					file_fd = fopen(filename, "r");
					if (file_fd != NULL){
						char response[Maxlinelen];
						char *status = "HTTP/1.1 200 OK\r\nContent-Type: text/html\n\n";

						send(ssock, status, strlen(status), 0);

						while (readfile(file_fd, response, sizeof(response))){//from formget.html
							send(ssock, response, strlen(response), 0);//write to browser
						}
						closesocket(ssock);
						return 0;
					}
					else{
						char *status = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\n\n";
						send(ssock, status, strlen(status), 0);

						closesocket(ssock);
						return 0;
					}
				}
				else if (strstr(command, "hw3.cgi") != NULL){
					if (strstr(command, "?") != NULL){//GET http://java.csie/hello.cgi?a=b&c=d HTTP/1.1
						char *front;
						char *back;
						front = strtok(command, "?");//GET.........cgi
						back = strtok(NULL, "?");
						strcpy(filename, strrchr(front, '/') + 1);//hello.cgi
						strcpy(querystring, strtok(back, " "));//a=aa&b=bb
					}
					char *status = "HTTP/1.1 200 OK\r\n";
					send(ssock, status, strlen(status), 0);


					char *temp;
					temp = strtok(querystring, "&");
					while (temp != NULL){
						char head;
						char *parameter;
						int num;
						head = temp[0];
						num = atoi(temp + 1) - 1;
						if (head == 'h'){
							parameter = strchr(temp, '=');
							strcpy(ip[num], parameter + 1);
						}
						else if (head == 'p'){
							parameter = strchr(temp, '=');
							strcpy(port[num], parameter + 1);
						}
						else if (head == 'f'){
							parameter = strchr(temp, '=');
							strcpy(file[num], parameter + 1);
						}
						else{
							fprintf(stderr, "parameter error");
							fflush(stderr);
						}
						temp = strtok(NULL, "&");
					}
					conn = 0;

					for (int i = 0; i < MaxHost; i++){
						csock[i] = NULL;
						if (strcmp(ip[i], "") != 0 && strcmp(port[i], "") != 0 && strcmp(file[i], "") != 0){
							char filepath[Maxlinelen];
							clear_array(filepath, Maxlinelen);
							strcpy(filepath, file[i]);

							fp[i] = fopen(filepath, "r");

							if (fp[i] != NULL){
								struct hostent *host;
								host = gethostbyname(ip[i]);

								dest[i].sin_family = AF_INET;
								dest[i].sin_addr = *((struct in_addr *)host->h_addr);
								dest[i].sin_port = htons(atoi(port[i]));

								csock[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

								err = WSAAsyncSelect(csock[i], hwnd, WM_SOCKET_NOTIFY, FD_WRITE | FD_CLOSE);

								if (connect(csock[i], (SOCKADDR *)&dest[i], sizeof(dest[i])) == SOCKET_ERROR) {
									if (WSAGetLastError() != WSAEWOULDBLOCK){
										EditPrintf(hwndEdit, TEXT("=== Error: csock[%d] connect error ===\r\n"), i);
										closesocket(csock[i]);
										csock[i] = NULL;
									}
								}

							}
							else{
								fprintf(stderr, "File '%s' open failed\n", file[i]);
								fflush(stderr);
							}
						}
					}
					for (int i = 0; i < MaxHost; i++){
						if (csock[i] == wParam){
							conn++;
						}
					}
					send(ssock, "Content-Type: text/html\n\n", strlen("Content-Type: text/html\n\n"), 0);
					send(ssock, "<html>\n", strlen("<html>\n"), 0);
					send(ssock, "<head>\n", strlen("<head>\n"), 0);
					send(ssock, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n", strlen("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n"), 0);
					send(ssock, "</head>\n", strlen("</head>\n"), 0);
					send(ssock, "<body bgcolor=#1E90FF>\n", strlen("<body bgcolor=#1E90FF>\n"), 0);
					send(ssock, "<font face=\"Courier New\" size=2 color=#FFFF99>\n", strlen("<font face=\"Courier New\" size=2 color=#FFFF99>\n"), 0);
					send(ssock, "<table width=\"800\" border=\"1\">\n", strlen("<table width=\"800\" border=\"1\">\n"), 0);
					send(ssock, "<tr>\n", strlen("<tr>\n"), 0);

					for (int i = 0; i < MaxHost; i++){
						char temp[Maxlinelen];
						sprintf(temp, "<td>%s</td>", ip[i]);
						send(ssock, temp, strlen(temp), 0);
					}
					send(ssock, "</tr>\n", strlen("</tr>\n"), 0);
					send(ssock, "<tr>\n", strlen("<tr>\n"), 0);
					for (int i = 0; i < MaxHost; i++){
						char temp[Maxlinelen];
						sprintf(temp, "<td valign=\"top\" id=\"m%d\"></td>", i);
						send(ssock, temp, strlen(temp), 0);
					}
					send(ssock, "</tr>\n", strlen("</tr>\n"), 0);
					send(ssock, "</table>\n", strlen("</table>\n"), 0);
				}
				else{
					char *status = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\n\n";
					send(ssock, status, strlen(status), 0);

					closesocket(ssock);
				}
				break;
			case FD_WRITE:


				//Write your code for write event here
				int index;
				for (index = 0; index < MaxHost; index++){
					if (csock[index] == wParam){
						break;
					}
				}

				char ResultFromServer[Maxlinelen];
				clear_array(ResultFromServer, Maxlinelen);
				int len;
				len=readline(csock[index], ResultFromServer, Maxlinelen);
				if (len > 0 || (len == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)){
					strtok(ResultFromServer, "\r\n");
					if (len > 0){
						char temp[Maxlinelen];
						sprintf(temp, "<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n", index, ResultFromServer);
						send(ssock,temp,sizeof(temp),0);
					}
					if (strstr(ResultFromServer, "&") != NULL){
						int len2;
						char command[Maxlinelen];
						clear_array(command, Maxlinelen);
						len2 = readfile(fp[index], command, Maxlinelen);

						if (len2 > 0){
							send(csock[index], command,strlen(command),0);
							strtok(command, "\r\n");

							char temp[Maxlinelen];
							sprintf(temp, "<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n", index, command);
							send(ssock, temp, sizeof(temp), 0);

							if (strcmp(command, "exit") == 0){
								closesocket(csock[index]);
							}
						}
					}
				}
				break;


				case FD_CLOSE:
					break;
			};
			break;
		
		default:
			return FALSE;


	};

	return TRUE;
}

int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}
int readline(SOCKET fd, char *ptr, int maxlen)
{
	int n, rc;
	char c;
	for (n = 1; n<maxlen; n++){
		if ((rc = recv(fd, &c, 1,0)) == 1){
			*ptr++ = c;
			if (c == '\n')  break;
		}
		else if (rc == 0){
			if (n == 1)     return(0);
			else         break;
		}
		else return(-1);
	}
	*ptr = 0;
	return(n);
}
int readfile(FILE *fp, char *ptr, int maxlen)
{
	int n, rc;
	char c;
	for (n = 1; n<maxlen; n++){
		if ((rc = fread(&c, sizeof(char), 1, fp)) == 1){
			*ptr++ = c;
			if (c == '\n')  break;
		}
		else if (rc == 0){
			if (n == 1)     return(0);
			else         break;
		}
		else return(-1);
	}
	*ptr = 0;
	return(n);
}
void clear_array(char array[], int len)
{
	for (int i = 0; i < len; i++){
		array[i] = '\0';
	}
}