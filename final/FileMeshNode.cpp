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
#define confFile "FileMesh.cfg"
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

		if(node_no==whoami.ID){
			if(strcmp(option,"Get")==0){
				//send the file to client
			}
			else{
				//recieve and store the file from client
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
		cout<<"usage: <executable> <confugration file>"<<endl;
		return -1;
	}
	bool flag=false;


	parse_conf_file(argv[1],nodes);
	char* myIP = getmyIP();
	for(int i=0;i<nodes.size();i++){
		if(strcmp(myIP, nodes[i].IP)==0){
			whoami=nodes[i];flag=true;
			break;
		}
	}
	if(!flag){
		cout<<"ip not found";
		return -1;
	}
	//cout<<whoami.ID<<endl;
	listenUDPRequest();
	return 0;
}