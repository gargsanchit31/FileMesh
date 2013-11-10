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
#define MAXBUFLEN 10000
#define BACKLOG 64
#define confFile "FileMesh.cfg"
//#define UDPCHUNK 100
//#include <sstream>

#include <vector>
#define DELIM ":"
using namespace std;

vector<struct node> nodes;

void sendUDPRequest(int nodeID, char* option, const char* md5, char* myIP) {
	char* msg;
	size_t msg_size =0;
	struct node node_info = nodes[nodeID];
	int sockfd;
	struct sockaddr_storage their_addr;
	struct sockaddr_in my_addr;
	
	msg = new char[MAXBUFLEN];
	sprintf(msg,"%s:%s:%s:%d",option,md5,myIP,TCPPORT);
	msg_size = strlen(msg);
	//cout<<msg_size << " " << sizeof(char*)<<endl;
	
	socklen_t addr_len;
	ssize_t numbytes=0;
    
	memset(&my_addr, 0, sizeof my_addr);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(nodes[nodeID].PORT);
	my_addr.sin_addr.s_addr = inet_addr(node_info.IP);
	
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd == -1) {
		perror("listener: socket");
	}	
	
	if ((numbytes = sendto(sockfd, msg, msg_size, 0, (struct sockaddr *)&my_addr, sizeof my_addr)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
	printf("talker: sent %d bytes\n", numbytes);
	cout<<msg<<" to node: "<<nodeID<<endl;
	close(sockfd);
}

int bindOnTCP(char* myIP) {
	int sockfd,bd;
	struct sockaddr_in my_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd == -1) {
		perror("listener: socket");
	}
	
	memset(&my_addr, 0, sizeof my_addr);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(TCPPORT);
	my_addr.sin_addr.s_addr = inet_addr(myIP);
	

	int yes=1;
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	bd = bind(sockfd, (struct sockaddr *) &my_addr, sizeof my_addr);
	if (bd == -1) {
		close(sockfd);
		perror("listener: bind");
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	printf("listener: waiting to recv...\n");
	return sockfd;
}

void storefileTCP(char* file_path, int socketid) {
	long lSize;
	char* buffer;
	size_t result;
	char file_size[10];
	
	FILE *file1 = fopen (file_path, "rb");
		if (file1!=NULL) {
			fseek(file1,0,SEEK_END);
			lSize = ftell(file1);
			rewind(file1);
			printf("File size : %d\n",lSize);
			sprintf(file_size, "%d", lSize);
			
			int len = send(socketid, file_size, 10, 0);
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

void receivefileTCP(const char* md5, int socketfd){
	char buf[MAXBUFLEN];
	ssize_t numbytes=-1;
	int recv_size;
	if ((recv_size = recv(socketfd, buf, MAXBUFLEN-1, 0)) == 0){
		printf("Error at recv\n");
	}
	
    int file_size = atoi(buf);	//first recieved data is file size
    int remain_data = file_size - (recv_size - 10);    
	printf("length = %d",file_size);
	printf("\n");
	FILE *fout;
	fout = fopen(md5, "wb");
	fwrite(buf+10, sizeof(char), recv_size-10, fout);

	while(remain_data >0) {
		numbytes = recv(socketfd, buf, MAXBUFLEN-1 , 0) ;
		fwrite(buf, sizeof(char), numbytes, fout);
        remain_data -= numbytes;
        printf("Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	printf("Received file!\n");
	cout<<recv_size<<endl;
	fclose(fout);
	close(socketfd);
}

void acceptTCP(int sockfd, char* option, char* filename) {
	int new_fd;
	string storage_path;
	struct sockaddr_storage their_addr;
	
	unsigned int sin_size = sizeof(struct sockaddr_in);
	printf("listener: waiting to accept...\n");
	if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
		perror("accept");
	}
	
	cout<<"connection accepted"<<endl;
	if(strcmp(option, "Store")==0){
		storefileTCP(filename,new_fd);		
	}
	else if(strcmp(option, "Get")==0){
		cout<<"Enter the <file/path/filename>: ";
		cin>>storage_path;
		receivefileTCP(storage_path.c_str(),new_fd);
	}
	else{
		cout<<"Something went wrong. We will figure it out soon."<<endl;		
	}
}
//input nodeid, request tpye, filename
int main(int argc, char* argv[]) {
	char* myIP;
	if(argc!=4) {
		cout<<"usage:<executable> <nodeID> <Store or Get> <filename/md5sum>"<<endl;
		return -1;
	}
	if(!(strcmp(argv[2], "Store")==0) and !(strcmp(argv[2] , "Get") ==0)){
		cout<<"invalid request: enter Store or Get"<<endl;
		return -1;
	}
	myIP = getmyIP();
	parse_conf_file(confFile,nodes);
	string filename = argv[3];
	string md5;
	if((strcmp(argv[2],"Store")) == 0) {
		md5 = md5sum(filename);
	}
	else {
		md5=(string)argv[3];
	}
	
	const char* md =md5.c_str();
	cout<<md<<" "<<argv[3]<<endl;
	int nodeid = atoi(argv[1]);
	cout<<nodeid<<endl;
	int tcpsockfd = bindOnTCP(myIP);
	sendUDPRequest(nodeid,argv[2],md,myIP);
	acceptTCP(tcpsockfd,argv[2],argv[3]);
	return 0;
}

