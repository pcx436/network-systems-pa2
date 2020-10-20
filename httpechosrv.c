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
	long fileSize;
	char *exeResolved = NULL, *absoluteURI = NULL;
	char errorMessage[] = "%s 500 Internal Server Error\r\n";
	char receiveBuffer[MAXLINE], *response = (char *)malloc(MAXBUF);
	char *method, *uri, *version, *savePtr, *relativeURI = (char *)malloc(PATH_MAX), *cleanedURI;
	char *contentType;
	exeResolved = realpath("./www/", NULL);
	FILE *fp;

	// char response[] = "HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";
	bzero(receiveBuffer, MAXLINE);  // fill receiveBuffer with \0
	bzero(response, MAXBUF);  // fill response with \0
	bzero(relativeURI, PATH_MAX);

	read(connfd, receiveBuffer, MAXLINE);

	// logic time!
	// get HTTP request method, request URI, and request version separately
	method = strtok_r(receiveBuffer, " ", &savePtr);
	uri = strtok_r(NULL, " ", &savePtr);
	version = strtok_r(NULL, "\n", &savePtr);
	trimSpace(method);
	trimSpace(uri);
	trimSpace(version);

	if (method && uri && version && strcmp(method, "GET") == 0){  // happy path
		// relative path to www folder
		if(uri[0] == '/') {
			sprintf(relativeURI, "./www%s", uri);
		} else {
			sprintf(relativeURI, "./www/%s", uri);
		}
		absoluteURI = realpath(relativeURI, NULL);

		// get absoluteURI of request and ensure it is not trying to escape
		if (absoluteURI && strncmp(exeResolved, absoluteURI, strlen(exeResolved)) == 0) {
			/*
			 * Verify file presence steps:
			 * Determine if path is a directory
			 * If it is, look for URI/index.html and URI/index.htm
			 */
			cleanedURI = isDirectory(absoluteURI);
			free(absoluteURI);

			contentType = getType(cleanedURI);
			fp = fopen(cleanedURI, "r");
			if (cleanedURI && contentType && fp) {
				// get file size
				fseek(fp, 0L, SEEK_END);
				fileSize = ftell(fp);
				fseek(fp, 0L, SEEK_SET);

				sprintf(response, "%s %d Document Follows\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", version, HTTP_OK, contentType, fileSize);
				send(connfd, response, strlen(response), 0);

				do {
					bzero(response, MAXBUF);

					bytesRead = fread(response, sizeof(char), MAXBUF, fp);

					if (send(connfd, response, bytesRead, 0) == -1) {
						perror("[-]Error in sending file.");
						break;
					}
				} while(bytesRead == MAXBUF);
				bzero(response, MAXBUF);
				fclose(fp);
				free(cleanedURI);
			} else {
				sprintf(response, errorMessage, version);
			}
		} else {
			// attempted directory escaping, return error
			sprintf(response, errorMessage, version);
		}
	} else { // Invalid HTTP request, assume version 1.1
		sprintf(response, errorMessage, "HTTP/1.1");
	}

	if (strlen(response) > 0){
		send(connfd, response, strlen(response), 0);
	}

	free(response);
	free(exeResolved);
	if(relativeURI)
		free(relativeURI);
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

/**
 * Determine Content-Type of provided URI
 * @param uri
 * @return
 */
char *getType(char *uri) {
	int i;
	unsigned long extensionLength;
	char *currentExtension, *currentType;

	// If we cannot read the file, return NULL
	if (access(uri, R_OK) != 0)
		return NULL;

	for(i = 0; i < COUNT_TYPES; i++){
		currentExtension = contentTypes[i][0];
		currentType = contentTypes[i][1];
		extensionLength = strlen(currentExtension);

		if (extensionLength <= strlen(uri) && strcmp(uri + strlen(uri) - extensionLength, currentExtension) == 0) {
			return currentType;
		}
	}

	return NULL;
}

/**
 * Removes trailing spaces from a string.
 * @param str The string to trim space from.
 */
void trimSpace(char *str){
	int end = strlen(str) - 1;
	while(isspace(str[end])) end--;
	str[end+1] = '\0';
}
