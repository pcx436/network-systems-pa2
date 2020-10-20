/**
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define MAXLINE     8192  /* max text line length */
#define MAXBUF      8192  /* max I/O buffer size */
#define LISTENQ     1024  /* second argument to listen() */
#define HTTP_OK     200
#define COUNT_TYPES 8

static volatile int killed = 0;

char *contentTypes[COUNT_TYPES][2] = {
		{".html", "text/html"},
		{".txt", "text/plain"},
		{".png", "image/png"},
		{".gif", "image/gif"},
		{".jpg", "image/jpg"},
		{".ico", "image/x-icon"},
		{".css", "text/css"},
		{".js", "application/javascript"}
};

int open_listenfd(int port);

void echo(int connfd);

void *thread(void *vargp);

char *getType(char *uri);

char * isDirectory(char *uri);

void trimSpace(char *str);

void interruptHandler(int useless) {
	killed = 1;
}

int main(int argc, char **argv) {
	int listenfd, *connfdp, port;
	socklen_t clientlen = sizeof(struct sockaddr_in);
	struct sockaddr_in clientaddr;
	pthread_t tid;

	signal(SIGINT, interruptHandler);

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);

	listenfd = open_listenfd(port);
	while (!killed) {
		connfdp = malloc(sizeof(int));
		if ((*connfdp = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen)) > 0)
			pthread_create(&tid, NULL, thread, connfdp);
		else
			free(connfdp);
	}
	return 0;
}

/* thread routine */
void *thread(void *vargp) {
	int connfd = *((int *) vargp);
	pthread_detach(pthread_self());
	free(vargp);

	echo(connfd);

	close(connfd);
	return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) {
	size_t bytesRead;
	char receiveBuffer[MAXLINE], *method, *uri, *version, *response = (char *)malloc(MAXBUF);
	char errorMessage[] = "HTTP/%s %d Internal Server Error";
	// char response[] = "HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";
	bzero(receiveBuffer, MAXLINE);  // fill receiveBuffer with \0
	bzero(response, MAXBUF);  // fill response with \0

	bytesRead = read(connfd, receiveBuffer, MAXLINE);
	printf("server received the following %ldB request:\n%s\n", bytesRead, receiveBuffer);

	// logic time!
	// get HTTP request method, request URI, and request version separately
	method = strtok(receiveBuffer, " ");
	uri = strtok(NULL, " ");
	version = strtok(NULL, " ");

	if (method && uri && version){

	} else { // Invalid HTTP request, assume version 1.1
		sprintf(response, errorMessage, "1.1", HTTP_ERROR);
	}

	printf("server returning a http message with the following content.\n%s\n", response);
	write(connfd, response, strlen(response));

}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) {
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	/* Create a socket descriptor */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	/* Eliminates "Address already in use" error from bind. */
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
	               (const void *) &optval, sizeof(int)) < 0)
		return -1;

	/* listenfd will be an endpoint for all requests to port
	   on any IP address for this host */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short) port);
	if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		return -1;

	/* Make it a listening socket ready to accept connection requests */
	if (listen(listenfd, LISTENQ) < 0)
		return -1;
	return listenfd;
} /* end open_listenfd */
