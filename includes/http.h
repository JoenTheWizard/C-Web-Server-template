#ifndef HTTPHONEY_H
#define HTTPHONEY_H

//Standards
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

//Sockets
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

typedef struct {
    int returnCode;
    char* fileName;
} HTTP_REQ;

//Send HTTP Headers to the sockets
int sendHTTP(int fd, char* msg);

//Read Web page (HTML) file
char* readFile(char* path);

//Init the HTTP listener
int init_listener_socket(uint16_t port);

//Send the response back
int send_response(int fd, char* header, char* content_type, char* body);

//Obtain IP addr
void* get_in_addr(struct sockaddr* sa);

//Handle the incoming connections for the HTTP web server
int handle_http_request(int fd);

//Manage with fork() child processess
void sigchld_handler(int s);

//This will handle for each SIGNAL child process when they have been destroyed
void start_reaper(void);
#endif