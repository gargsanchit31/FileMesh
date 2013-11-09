#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "parser.h"
#include "md5.h"
#define TCPPORT 6401
#define MAXBUFLEN 100000
#define BACKLOG 64
#define CONF_FILE "./FileMesh.cfg"
#include <sstream>

#include <vector>
#define DELIM ":"
using namespace std;

vector<struct node> nodes;
struct node whoami;


void parseMsg(char* msg1, char **option, char **md5, char **ip, int& port){
	char *pch;
	char msg[100];
	strcpy(msg,msg1);
	pch = strtok (msg,":");
	*option = (char*)malloc(10*sizeof(char));
	strcpy(*option,pch);
	pch = strtok (NULL,":");
	*md5 = (char*)malloc(40*sizeof(char));
	strcpy(*md5,pch);
	pch = strtok (NULL,":");
	*ip = (char*)malloc(10*sizeof(char));
	strcpy(*ip,pch);
	pch = strtok (NULL,":");
	port = atoi(pch);
}

int connectTCP(char *ip, int port){
	int sockfd;
	struct sockaddr_in their_add;
	
	their_add.sin_family = AF_INET; // host byte order
	cout<<"in connectTCP: port no. "<<port<<endl;
	their_add.sin_port = htons(port); // short, network byte order
	their_add.sin_addr.s_addr = inet_addr(ip);	//set the ip
	bzero(&(their_add.sin_zero), 8);	//equivilent to memset
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd == -1) {
		perror("listener: tcp socket");
	}
	
	if (connect(sockfd, (struct sockaddr *)&their_add,sizeof(struct sockaddr)) == -1) {
		cout<<"dsfsafsdaf"<<endl;
		perror("connect");
		exit(1);
	}
	return sockfd;
}

void storeFile(int socketfd, char* md5){
	char buf[MAXBUFLEN];
	ssize_t numbytes=-1;
	if ((recv(socketfd, buf, MAXBUFLEN-1, 0)) == 0){
		printf("Error at recv\n");
	}

    int file_size = atoi(buf);	//first recieved data is file size
    int remain_data = file_size;    
	printf("length = %d",file_size);
	printf("\n");
	FILE *fout;
	char* file_path = whoami.Folder_Path;
	strcat(file_path,md5);
	fout = fopen(file_path, "wb");
	while(remain_data >0 && (numbytes = recv(socketfd, buf, MAXBUFLEN-1 , 0)) > 0 ) {
		fwrite(buf, sizeof(char), numbytes, fout);
        remain_data -= numbytes;
        printf("Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	printf("Received file!\n");
	fclose(fout);
	close(socketfd);
}

void sendFile(char* md5, int socketid) {
	long lSize;
	char* buffer;
	size_t result;
	char file_size[256];
	char* file_path = whoami.Folder_Path;
	strcat(file_path,md5);
	FILE *file1 = fopen (file_path, "rb");
		if (file1!=NULL) {
			fseek(file1,0,SEEK_END);
			lSize = ftell(file1);
			rewind(file1);
			printf("File size : %d\n",lSize);
			sprintf(file_size, "%d", lSize);
			
			int len = send(socketid, file_size, sizeof(file_size), 0);
			printf("len = %d\n", len);
			if (len < 0) {
				fprintf(stderr, "Error! %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			int numbytes;	
			long fs = lSize;
			char line[110];
			while(fs > 0) {
				int chunk= 100;
				if(fs < 100){
					chunk = fs;
				}
				fread(line,1,chunk,file1);	
				//printf("chunk is %d, strlen is %d", chunk, strlen(line));
				
				if ((numbytes = send(socketid, line, chunk, 0)) == -1) {
					perror("talker: sendto");
					exit(1);
				}
				printf("talker: sent %d bytes\n", numbytes);
				fs-=numbytes;
				printf("talker: remaining %d bytes\n", fs);
			}
			printf("File sent!\n");
		}
		fclose(file1);
		close(socketid);
}

void sendUDPRequest(int nodeID, char* msg) {
	size_t msg_size =0;
	//char* myIP = getmyIP();
	int sockfd;
	struct sockaddr_in my_addr;
	
	msg_size = strlen(msg);
	//cout<<msg_size << " " << sizeof(char*)<<endl;
	
	socklen_t addr_len;
	ssize_t numbytes=0;
    
	memset(&my_addr, 0, sizeof my_addr);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(nodes[nodeID].PORT);
	my_addr.sin_addr.s_addr = inet_addr(nodes[nodeID].IP);
	
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd == -1) {
		perror("listener: socket");
	}	
	
	if ((numbytes = sendto(sockfd, msg, msg_size, 0, (struct sockaddr *)&my_addr, sizeof my_addr)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
	printf("talker: sent %d bytes\n", numbytes);
	close(sockfd);
}


void listenUDPRequest(){
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
	my_addr.sin_port = htons(whoami.PORT);
	my_addr.sin_addr.s_addr = inet_addr(whoami.IP);
	
	bd = bind(sockfd, (struct sockaddr *) &my_addr, sizeof my_addr);
	if (bd == -1) {
		close(sockfd);
		perror("listener: bind");
	}
	ssize_t numbytes=-1;
	//printf("listener: waiting to recvfrom...\n");
	
	/* Receiving file size */
    numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len);
	if(numbytes >= 0){
		char *option,*md5,*ip;
		int port;
		parseMsg(buf,&option,&md5,&ip,port);
		cout<<option<<" "<<md5<<" "<<ip<<" "<<port<<endl;
		int node_no = md5sumhash(md5,nodes.size());

		cout<<node_no<<","<<whoami.ID<<endl;

		if(node_no==whoami.ID){
			cout<<"yes ip matched\n"<<endl;
			int tcpsocket = connectTCP(ip, port);
			if(strcmp(option,"Get")==0){
				//send the file to client
				sendFile(md5,tcpsocket);
			}
			else{
				//recieve and store the file from client
				storeFile(tcpsocket,md5);
			}
		}
		else{
			//cout<<buf<<endl;
			sendUDPRequest(node_no,buf);
		}
	}    
}


int main(int argc, char* argv[]){
	if(argc!=2){
		cout<<"usage: <executable> <Node ID>"<<endl;
		return -1;
	}

	parse_conf_file(CONF_FILE,nodes);
	int n_id = atoi(argv[1]);
	whoami=nodes[n_id];
	cout<<whoami.ID<<","<<whoami.IP<<","<<whoami.PORT<<endl;
	listenUDPRequest();
	return 0;
}