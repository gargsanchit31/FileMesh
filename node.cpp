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
#define FILENAME "/home/sanchit/Desktop/b.mp4"

int main() {
	int sockfd,bd;
	struct sockaddr_in my_addr;
	char buf[MAXBUFLEN];
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
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
	ssize_t numbytes=0;
	//printf("listener: waiting to recvfrom...\n");
	
	/* Receiving file size */
    recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len);
    int file_size = atoi(buf);
    int remain_data = file_size;    
	
	FILE *fout;
	fout = fopen(FILENAME, "wb");
	while(remain_data >0 && (numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) >= 0 ) {
		fwrite(buf, sizeof(char), numbytes, fout);
        remain_data -= numbytes;
        //fprintf(stdout, "Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	//printf("Received file!\n");
	fclose(fout);
	close(sockfd);
		
	return 0;
}
