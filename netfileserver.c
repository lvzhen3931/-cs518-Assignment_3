#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <assert.h>
#include "libnetfiles.h"

#define PORTNUM 2e14
#define NUM_CLIENT 5
static int buf_size = BUF_SIZE;
static int fds[10];
static int modes[10];
static int num_files = 0;

//Queue - Linked List implementation
struct Node {
	char* data;
	struct Node* next;
};
// Two glboal variables to store address of front and rear nodes. 
struct Node* front = NULL;
struct Node* rear = NULL;

// To Enqueue an operation
void Enqueue(char* x) {
	struct Node* temp = 
		(struct Node*)malloc(sizeof(struct Node));
	temp->data =x; 
	temp->next = NULL;
	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	rear->next = temp;
	rear = temp;
}

// To Dequeue an opertion
void Dequeue() {
	struct Node* temp = front;
	if(front == NULL) {
		printf("Queue is Empty\n");
		return;
	}
	if(front == rear) {
		front = rear = NULL;
	}
	else {
		front = front->next;
	}
	free(temp);
}

char* Front() {
	if(front == NULL) {
		printf("Queue is empty\n");
		return;
	}
	return front->data;
}

//put to front
void PutToFront(char* x){
	struct Node* temp = 
		(struct Node*)malloc(sizeof(struct Node));
	temp->data =x; 
	temp->next = front;
	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	front = temp;
}

struct file_metadata{
	char path[50];
	int  fds[5];
	int  modes[5];
	int  permission[5];
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
}
int f_open(char** tokens, const int num_tokens, char* msg){
	char* path;
	int flag;
	int fd = -1;
	int mode;
	
	assert(strcmp(tokens[0], "open") == 0);
	mode = atoi(tokens[1]);
	if(mode < 1 || mode > 3){
		errno = INVALID_FILE_MODE;
	}
	else{
		path = tokens[2];
		flag = atoi(tokens[3]);
		fd = open(path, flag);
	}
	if(fd < 0){
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d,%d", SUCCESS_RET, -fd);
		fds[num_files] = -fd;
		modes[num_files] = flag;			
		num_files++;
	}
	return 0;
}
int f_read(char** tokens, const int num_tokens, char* msg){
	int fd;
	size_t nbytes;
	ssize_t nbytes_read = 0;
	char* data;
	int i, valid;
	
	assert(strcmp(tokens[0], "read") == 0);
	fd = atoi(tokens[2]);
	for(i = 0; i < num_files; ++i){
		if(fds[i] == fd && modes[i] != O_WRONLY){
			valid = 1;
			break;
		}
	}
	if(!valid){
		errno = EBADF;
	}
	else{
		nbytes = (size_t)atoi(tokens[3]);
		data = malloc(nbytes);
		nbytes_read = read(-fd, (void*)data, nbytes);	
	}
	if(nbytes_read == -1){
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d,%ld,%s", SUCCESS_RET, nbytes_read, data);
	}
	return 0;
}
int f_write(char** tokens, const int num_tokens, char* msg){
	int fd;
	size_t nbytes;
	ssize_t nbytes_written = 0;
	int i, valid;
	
	assert(strcmp(tokens[0], "write") == 0);
	assert(num_tokens == 5);
	fd = atoi(tokens[2]);
	for(i = 0; i < num_files; ++i){
		if(fds[i] == fd && modes[i] != O_RDONLY){
			valid = 1;
			break;
		}
	}
	if(!valid){
		errno = EBADF;
	}
	else{
		nbytes = (size_t)atoi(tokens[3]);
		nbytes_written = write(-fd, (void*)(tokens[4]), nbytes);	
	}
	if(nbytes_written == -1){
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d,%ld", SUCCESS_RET, nbytes_written);
	}
	return 0;
}
int f_close(char** tokens, const int num_tokens, char* msg){
	int fd;
	int status = -1;
	int i, valid;
	
	assert(strcmp(tokens[0], "close") == 0);
	assert(num_tokens == 3);
	fd = atoi(tokens[2]);
	for(i = 0; i < num_files; ++i){
		if(fds[i] == fd){
			valid = 1;
			fds[i] = 0;
			modes[i] = 0;
			break;
		}
	}
	if(!valid){
		errno = EBADF;
	}
	else{
		status = close(-fd);
	}
	
	if(status == -1){
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d", SUCCESS_RET);
	}
	return 0;
}
void* process(void* fd) 
{
	int newsockfd; 
	int status, num_tokens;
	char buffer[buf_size];
	char msg[buf_size];
	char** tokens;

	newsockfd = *(int*)fd;
    bzero(buffer, buf_size);
	status = read(newsockfd, buffer, buf_size);
    if (status < 0) error("ERROR reading from socket");
	/* Tokenize massage */
	tokens = tokenize(buffer, ',', &num_tokens); 
	if(strcmp(tokens[0], "open") == 0){
		printf("processing open request\n");
		f_open(tokens, num_tokens, msg);	
	}
	else
	if(strcmp(tokens[0], "read") == 0){
		printf("processing read request\n");
		f_read(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "write") == 0){
		printf("processing write request\n");
		f_write(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "close") == 0){
		printf("processing close request\n");
		f_close(tokens, num_tokens, msg);
	}
	else{
		errno = INVALID_OPERATION_MODE;
		fprintf(stderr, "Invalid request type: %s\n", tokens[0]);	
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	bzero(buffer, buf_size);
	strcpy(buffer, msg);		
   	status = write(newsockfd, buffer, sizeof(buffer));
   	if (status < 0) error("ERROR writing to socket");		
	return 0;
}

void* process_queue(void* fd) 
{
	int newsockfd; 
	int status, num_tokens;
	char buffer[buf_size];
	char msg[buf_size];
	char** tokens;

	newsockfd = *(int*)fd;
    bzero(buffer, buf_size);
	status = read(newsockfd, buffer, buf_size);
    if (status < 0) error("ERROR reading from socket");
	/* Tokenize massage */
	tokens = tokenize(buffer, ',', &num_tokens); 
	Enqueue(tokens)
	if(strcmp(tokens[0], "open") == 0){
		printf("processing open request\n");
		f_open(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "read") == 0){
		printf("processing read request\n");
		f_read(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "write") == 0){
		printf("processing write request\n");
		f_write(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "close") == 0){
		printf("processing close request\n");
		f_close(tokens, num_tokens, msg);
	}
	else{
		errno = INVALID_OPERATION_MODE;
		fprintf(stderr, "Invalid request type: %s\n", tokens[0]);	
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	bzero(buffer, buf_size);
	strcpy(buffer, msg);		
   	status = write(newsockfd, buffer, sizeof(buffer));
   	if (status < 0) error("ERROR writing to socket");		
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd;
	long portno = PORTNUM;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

	pthread_t client_thread;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    listen(sockfd,5);
	clilen = sizeof(cli_addr);
	while( newsockfd = accept(sockfd, 
		   (struct sockaddr *)&cli_addr, &clilen)){
		puts("Connection Success\n");
		if(pthread_create(&client_thread, 
		   NULL, process, (void*)&newsockfd) < 0){
			perror("Thread creation failed");
			return -1;
		}	
	}
	pthread_exit(NULL);
    return 0; 
}
