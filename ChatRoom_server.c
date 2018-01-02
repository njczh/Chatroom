/************************************************************************
	> File Name: ChatRoom_server.c
	> Author: Cao Zhenghui 
	> Mail: 97njczh@gmail.com
	> Created Time: Fri 29 Dec 2017 09:17:11 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>			// UNIx STanDard
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>

#include <netdb.h>
#include <netinet/in.h>

#define CONNECT_QUEUE_LENGTH 32 // maximum connection queue length
#define MAX_CONNECT 100
#define BUFFER_SIZE 1024

// struct message{
    // char name[20];
    // char content[BUFFER_SIZE];
    // int  socket;
// };

int ssock[MAX_CONNECT+1];		// slave server socket

/*----------------- Function Declaration -----------------*/
int	errexit(const char *format, ...);
int	passiveTCP(const char *service, int queue_len);
int	passivesock(const char *service, const char *transport, int qlen);
int ChatRoom(int fd);

/*------------------------- Main -------------------------*/
int main(int argc, char *argv[])
{
	char *service; 				// service name or port number
	
	//-- struct sockaddr_in fsin; 	// the address of a client
	//-- unsigned int alen;		 	// length of client's address

	int msock;					// master server socket

	memset(ssock, -1, sizeof(ssock));

	pthread_t th;
	// pthread_attr_t ta;

	switch (argc){
	case 1: 
		service = "20197"; break; // none arguments
	case 2:
		service = argv[1]; break;
	default:
		errexit("usage: ChatRoom [port]\n");
	}

	// (void) pthread_attr_init(&ta);
	// (void) pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

	/* 服务器端创建套接字+绑定+开始监听 */
	msock = passiveTCP(service, CONNECT_QUEUE_LENGTH);

	while (1)
	{
		int i;
		for (i = 0; i < MAX_CONNECT;i++)
			if(ssock[i] == -1)
				break;
		//-- alen = sizeof(fsin);
		//-- ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
		ssock[i] = accept(msock, NULL, NULL);
		if (ssock[i] < 0){
			if (errno == EINTR)
				continue;
			errexit("accept: %s\n", strerror(errno));
		}

		if(i == MAX_CONNECT)
		{
			char *str = "Sorry!The chat rooom is full!";
			send(ssock[MAX_CONNECT], str, sizeof(str), 0);
			close(ssock[MAX_CONNECT]);
		}

		/* 调用pthread_create创建一个新线程来处理连接 */
		if (pthread_create(&th, NULL, 
				(void * (*)(void *))ChatRoom,
		    	(void *)ssock[i]) < 0)
			errexit("pthread_create: %s\n", strerror(errno));
	}

	return 0;
}


/*----------------------------------------------------------------------*\
  Function:		ChatRoom
  Description:	
\*----------------------------------------------------------------------*/
int ChatRoom(int fd) 
{
  	int cc;
	char buf[BUFFER_SIZE];

	strcpy(buf,"Connect established！Welcome！\n");
	if(write(fd, buf, strlen(buf))<0)
		errexit("write: %s\n", strerror(errno));
	memset(buf, 0, strlen(buf));

	while (cc = read(fd, buf, sizeof(buf))) {
    	if (cc < 0)
      		errexit("read: %s\n", strerror(errno));
		int i;
		for (i = 0; i < MAX_CONNECT;i++)
			if(-1 != ssock[i] && fd != ssock[i] )
				if (write(ssock[i], buf, cc) < 0)
					errexit("write: %s\n", strerror(errno));
  	}

	(void) close(fd);

}

/*----------------------------------------------------------------------*\
  Function:		errexit 
  Description:	print an error message and exit
\*----------------------------------------------------------------------*/
int errexit(const char *format, ...)
{
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}

/*----------------------------------------------------------------------*\
  Function:		passiveTCP
  Description:	create a passive socket for use in a TCP server
  Arguments:		
 		service - service associated with the desired port
 		qlen    - maximum server request queue length
\*----------------------------------------------------------------------*/
int passiveTCP(const char *service, int qlen)
{
	return passivesock(service, "tcp", qlen);
}

/*----------------------------------------------------------------------*\
  Function:		passivesock
  Description:	allocate & bind a server socket using TCP or UDP
  Arguments:
    	service   - service associated with the desired port
    	transport - transport protocol to use ("tcp" or "udp")
    	qlen      - maximum server request queue length
\*----------------------------------------------------------------------*/
int passivesock(const char *service, const char *transport, int qlen)
{
	struct servent *pse;			// pointer to service information entry	
	struct protoent *ppe;   		// pointer to protocol information entry
	struct sockaddr_in sin; 		// an Internet endpoint address			
	int s, type;					// socket descriptor and socket type

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;	// 允许在任意IP地址下接收数据

	/*********** Map service name to port number ***********/
	if (pse = getservbyname(service, transport)) 
		// 从服务名映射到熟知端口号
		sin.sin_port = htons(ntohs((unsigned short)pse->s_port)); 
		// 或直接将端口号转换成网络字节序
	else if ((sin.sin_port = htons((unsigned short)atoi(service))) == 0) 
		errexit("can't get \"%s\" service entry\n", service);

	/********* Map protocol name to protocol number *********/
		// 将协议名称映射到协议号，如TCP协议的协议号为6 UDP为17
	if ((ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocol entry\n", transport);

	/********* Use protocol to choose a socket type *********/
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;	// udp
	else
		type = SOCK_STREAM;	// tcp

	/****************** Allocate a socket ******************/
	s = socket(PF_INET, type, ppe->p_proto);
	if (s < 0)
		errexit("can't create socket: %s\n", strerror(errno));

	/******************* Bind the socket *******************/
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("can't bind to %s port: %s\n", service,
				strerror(errno));

	/****************** Listen the socket ******************/			
	if (type == SOCK_STREAM && listen(s, qlen) < 0)
		errexit("can't listen on %s port: %s\n", service,
				strerror(errno));

	return s;
}
