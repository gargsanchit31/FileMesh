#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define MYPORT 4950 // the port users will be connecting to
#define MAXBUFLEN 1000000
#define FILENAME "/home/abhilash/year3/sem1/378/project1/m.pdf"
#define BACKLOG 10
int main() {
	int sockfd,bd,new_fd;
	struct sockaddr_in my_addr;
	char buf[MAXBUFLEN];
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd == -1) {
		perror("listener: socket");
	}
	
	memset(&my_addr, 0, sizeof my_addr);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	inet_pton(AF_INET, "127.0.0.1", &(my_addr.sin_addr));
	
	bd = bind(sockfd, (struct sockaddr *) &my_addr, sizeof my_addr);
	if (bd == -1) {
		close(sockfd);
		perror("listener: bind");
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	
	ssize_t numbytes=0;
	printf("listener: waiting to recvfrom...\n");
	
	int sin_size = sizeof(struct sockaddr_in);
	if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
		perror("accept");
		//continue;
	}
	/* Receiving file size */
   	if ( (recv(new_fd, buf, MAXBUFLEN-1, 0)) == 0) {
		printf("Error at recv\n");
	}
    int file_size = atoi(buf);
    int remain_data = file_size;    
	printf("length = %d",file_size);
	printf("\n");
	FILE *fout;
	fout = fopen(FILENAME, "wb");
	while(remain_data >0 && (numbytes = recv(new_fd, buf, MAXBUFLEN-1 , 0)) >= 0 ) {
		fwrite(buf, sizeof(char), numbytes, fout);
        remain_data -= numbytes;
        fprintf(stdout, "Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	printf("Received file!\n");
	fclose(fout);
	close(new_fd);
	close(sockfd);
		
	return 0;
}
