#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <errno.h>
#include <netdb.h> 
#include "libnetfiles.h"

static struct hostent *server = NULL;
static struct sockaddr_in serv_addr;
static const int buf_size = BUF_SIZE;
static int f_mode = -1;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}
int netserverinit(char* hostname, int filemode){
	long portno = PORTNUM;

	server = gethostbyname(hostname);
	if(server == NULL){
		return -1;
	}
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

	/* Set file mode */
	f_mode = filemode;
	return 0;
}

int start_session(int *sockfd){
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd < 0){
        error("ERROR opening socket");
	}
	if(server == NULL){
		error("Host not set\n");
	}
    if (connect(*sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        error("ERROR connecting");
	}
	printf("Socket connection established!\n");
	return 0;	
}
int end_session(int *sockfd){
	printf("Socket connection closed!\n");
	close(*sockfd);
	return 0;
}
int netopen(const char *pathname, int flags){
	char buf[buf_size];
	int sockfd, status;
	char **tokens;
	int n, num_tokens, fd = -1;	

	status = start_session(&sockfd);
	if(status != 0){
		error("Socket Error");
	}
	if(server == NULL){
		h_errno = HOST_NOT_FOUND;
		return -1;
	}
	/* Concatenate parameters of netopen */
	bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf), "open,%d,%s,%d", f_mode, pathname, flags);
	/* Send message to socket */
	n = write(sockfd, buf, buf_size);
	bzero(buf, sizeof(buf));
	n = read(sockfd, buf, buf_size);
	tokens = tokenize(buf, ',', &num_tokens);
	assert(num_tokens == 2);	
	status = atoi(tokens[0]);
	if(status == -1){
		errno = atoi(tokens[1]);
	}
	else
	if(status == 0){
		fd = atoi(tokens[1]);	
	}	
	end_session(&sockfd);
	return fd;		
}
ssize_t netread(int fildes, void *data_buf, size_t nbytes){
	char buf[buf_size];
	int sockfd, status;
	char **tokens;
	int n, num_tokens, fd = -1;	
	ssize_t nbytes_read = -1;

	status = start_session(&sockfd);
	if(status != 0){
		error("Socket Error");
	}
	if(server == NULL){
		h_errno = HOST_NOT_FOUND;
		return -1;
	}

	fd = fildes;
	/* Concatenate parameters of netread */
	bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf), "read,%d,%d,%ld", f_mode, fd, nbytes);
	/* Send message to socket */
	n = write(sockfd, buf, buf_size);
	bzero(buf, sizeof(buf));
	n = read(sockfd, buf, buf_size);
	tokens = tokenize(buf, ',', &num_tokens);
	assert(num_tokens >= 2);	
	status = atoi(tokens[0]);
	if(status == -1){
		assert(num_tokens == 2);
		errno = atoi(tokens[1]);
	}
	else
	if(status == 0){
		assert(num_tokens >= 3);
		nbytes_read = atoi(tokens[1]);	
		if(tokens[2] != NULL){
			strcpy(data_buf, tokens[2]);
		}
	}	
	end_session(&sockfd);
	return nbytes_read;	
}
ssize_t netwrite(int fildes, const void *data_buf, size_t nbytes){
	char buf[buf_size];
	int sockfd, status;
	char **tokens;
	int n, num_tokens, fd = -1;	
	ssize_t nbytes_written = -1;

	status = start_session(&sockfd);
	if(status != 0){
		error("Socket Error");
	}
	if(server == NULL){
		h_errno = HOST_NOT_FOUND;
		return -1;
	}

	fd = fildes;
	/* Concatenate parameters of netread */
	bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf), "write,%d,%d,%ld,%s", f_mode, fd, nbytes, (char*)data_buf);
	/* Send message to socket */
	n = write(sockfd, buf, buf_size);
	bzero(buf, sizeof(buf));
	n = read(sockfd, buf, buf_size);
	tokens = tokenize(buf, ',', &num_tokens);
	assert(num_tokens == 2);	
	status = atoi(tokens[0]);
	if(status == -1){
		errno = atoi(tokens[1]);
	}
	else
	if(status == 0){
		nbytes_written = atoi(tokens[1]);	
	}	
	end_session(&sockfd);
	return nbytes_written;	
}

int netclose(int fd){
	char buf[buf_size];
	int sockfd, status;
	char **tokens;
	int n, num_tokens;	

	status = start_session(&sockfd);
	if(status != 0){
		error("Socket Error");
	}
	if(server == NULL){
		h_errno = HOST_NOT_FOUND;
		return -1;
	}
	/* Concatenate parameters of netclose */
	bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf), "close,%d,%d", f_mode, fd);
	/* Send message to socket */
	n = write(sockfd, buf, buf_size);
	bzero(buf, sizeof(buf));
	n = read(sockfd, buf, buf_size);
	tokens = tokenize(buf, ',', &num_tokens);
	assert(num_tokens == 2 || num_tokens == 1);	
	status = atoi(tokens[0]);
	if(status == -1){
		errno = atoi(tokens[1]);
	}
	end_session(&sockfd);
	return status;	
}

int main(int argc, char *argv[])
{
	int fd, status;
	char buffer[buf_size];
	size_t nbytes = 100;
	ssize_t nbytes_read, nbytes_written;

	if(argc < 2){
		error("Usage: client [hostname]\n");
	}
	if(netserverinit(argv[1], UNRES) == -1){
		herror("Init Error");
		exit(-1);
	}		
	/* Open a file */
	fd = netopen("/.autofs/ilab/ilab_users/zl230/test_1.c", O_RDWR);
	if(fd == -1){
		error("netopen error");
	}
	else{
		printf("netopen file descriptor: %d\n", fd);
	}
	
	/* Read a file */
	bzero(buffer, sizeof(buffer));
	nbytes_read = netread(fd, buffer, nbytes);
	if(nbytes_read == -1){
		error("netread error");
	}
	else{
		printf("netread returned: %ld bytes read\n", nbytes_read);
		printf("netread data: %s\n", buffer); 
	}
	
	/* Write to a file */
	bzero(buffer, sizeof(buffer));
	sprintf(buffer, "THIS IS THE DATA WRITTEN TO FILE\n"); 
	nbytes_written = netwrite(fd, buffer, 33);
	if(nbytes_written == -1){
		error("netwrite error");
	}
	else{
		printf("netwrite returned: %ld bytes written\n", nbytes_written);
	}
	/* Close a file */
	status = netclose(fd);
	if(status == -1){
		error("netclose error");
	}
	else{
		printf("netclose returned with success\n");
	}
	
	return 0;
}
