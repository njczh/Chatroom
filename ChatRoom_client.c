/*************************************************************************
	> File Name: ChatRoom_client.c
	> Author: Cao Zhenghui
	> Mail: 97njczh@gmail.com 
	> Created Time: Fri 29 Dec 2017 09:11:38 PM CST
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdarg.h>
#include <pthread.h>  

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define LINELEN 128
#define BUFFER_SIZE 1024

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */

/*----------------- Function Declaration -----------------*/
int connectTCP(const char *host, const char *service );
int	connectsock(const char *host, const char *service,
		const char *transport);
int	errexit(const char *format, ...);


void ClientRecv(int fd)
{
  	int cc;
	char buf[BUFFER_SIZE]={};

	while (cc = read(fd, buf, sizeof(buf))) {

    	if (cc < 0)
      		errexit("read: %s\n", strerror(errno));

	    fputs(buf, stdout);

		memset(buf,0,strlen(buf));
  	}

	(void) close(fd);

}

void ClientSend(int fd)
{
	int n;					// read count
	char buf[LINELEN+1];	// buffer for one line of text

	while (fgets(buf, sizeof(buf),stdin)){

		buf[LINELEN] = '\0';

		(void)write(fd, buf, strlen(buf));

		memset(buf,0,strlen(buf));
	}

	(void) close(fd);

}

/*------------------------- Main -------------------------*/
int main(int argc, char *argv[])
{
	char *host = "39.108.95.130";	// host to use if none supplied
	char *service = "20197";		// default server name, it means NO.2019's 7 port
	int sfd;

	switch (argc)
	{
	case 1:
		printf("connecting \'default service(%s:%s)\'...\n", host, service);
		break;
	case 3:
		service = argv[2];
		/* FALL THROUGH */
	case 2:
		host = argv[1];
		break;
	default:
		fprintf(stderr, "usage: ChatRoom [host [port]]\n");
		exit(1);
	}

	sfd = connectTCP(host,service);

	pthread_t client_recv, client_send;

	if (pthread_create(&client_recv, NULL, 
				(void * (*)(void *))ClientRecv,
		    	(void *)sfd) < 0)
			errexit("pthread_create: %s\n", strerror(errno));

	if (pthread_create(&client_send, NULL, 
				(void * (*)(void *))ClientSend,
		    	(void *)sfd) < 0)
			errexit("pthread_create: %s\n", strerror(errno));

	pthread_join(client_recv, NULL);        
    pthread_join(client_send, NULL);

	close(sfd);

	exit(0);
}


/*----------------------------------------------------------------------*\
  Function:		connectsock
  Description:	allocate & connect a socket using TCP or UDP
  Arguments:
		host      - name of host to which connection is desired
        service   - service associated with the desired port
        transport - name of transport protocol to use ("tcp" or "udp")
\*----------------------------------------------------------------------*/
int connectsock(const char *host, const char *service, const char *transport )
{
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address			*/
	int	s, type;			/* socket descriptor and socket type	*/

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

    /* Map service name to port number */
	if ( pse = getservbyname(service, transport) )
		sin.sin_port = pse->s_port;
	else if ((sin.sin_port=htons((unsigned short)atoi(service))) == 0)
		errexit("can't get \"%s\" service entry\n", service);

    /* Map host name to IP address, allowing for dotted decimal */
	if ( phe = gethostbyname(host) )									// DNS解析域名或者局域网下的主机名
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )	// 点分十进制转换函数，转换失败返回INADDR_NONE
		errexit("can't get \"%s\" host entry\n", host);

    /* Map transport protocol name to protocol number */
	if ( (ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocol entry\n", transport);

    /* Use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

    /* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s < 0)
		errexit("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("can't connect to %s.%s: %s\n", host, service,
			strerror(errno));
	return s;
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
  Function:		connectTCP
  Description:	connect to a specified TCP service on a specified host
  Arguments:		
        host    - name of host to which connection is desired
        service - service associated with the desired port
\*----------------------------------------------------------------------*/
int connectTCP(const char *host, const char *service )
{
	return connectsock( host, service, "tcp");
}
