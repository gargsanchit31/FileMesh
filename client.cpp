#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define MYPORT 4950 // the port users will be connecting to
#define MAXBUFLEN 100000
#define FILENAME "/home/sanchit/Desktop/a.mp4"

int main(int argc, char *argv[]) {
	int sockfd;
	char msg[MAXBUFLEN];
	struct sockaddr_storage their_addr;
	struct sockaddr_in my_addr;
	socklen_t addr_len;
	ssize_t numbytes=0;
	
	char file_size[256];
    struct stat file_stat;
        
	
	memset(&my_addr, 0, sizeof my_addr);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	inet_pton(AF_INET, "127.0.0.1", &(my_addr.sin_addr));
	
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd == -1) {
		perror("listener: socket");
	}
	
	long lSize;
	char* buffer;
	size_t result;
	
	
        
    FILE *file1 = fopen (FILENAME, "rb");
	if (file1!=NULL) {
		fseek(file1,0,SEEK_END);
		lSize = ftell(file1);
		rewind(file1);
		//printf("File size : %d\n",lSize);
		sprintf(file_size, "%d", lSize);
		
		int len = sendto(sockfd, file_size, sizeof(file_size), 0,(struct sockaddr *)&my_addr, sizeof my_addr);
		if (len < 0) {
			fprintf(stderr, "Error! %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
			
		long fs = lSize;
		char line[110];
		while(fs > 0) {
			int chunk= 100;
			if(fs < 100){
				chunk = fs;
			}
			fread(line,1,chunk,file1);	
			//printf("chunk is %d, strlen is %d", chunk, strlen(line));
			
			if ((numbytes = sendto(sockfd, line, chunk, 0, (struct sockaddr *)&my_addr, sizeof my_addr)) == -1) {
				perror("talker: sendto");
				exit(1);
			}
			//printf("talker: sent %d bytes\n", numbytes);
			fs-=numbytes;
			//printf("talker: remaining %d bytes\n", fs);
		}
		//printf("File sent!\n");
	}
	fclose(file1);
	close(sockfd);
	
	return 0;
}
