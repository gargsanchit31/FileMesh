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
#define UDPPORT 6400
#define TCPPORT 6401
#define MAXBUFLEN 100000
#define confFile "FileMesh.cfg"
//#define UDPCHUNK 100
#include<sstream>

#include <vector>
#define DELIM ":"
using namespace std;

vector<struct node> nodes;

char* getmyIP() {
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;
	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	//cout<<inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)<<endl;
	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

void sendUDP(int nodeID, char* option, char* md5) {
	char* msg;
	size_t msg_size =0;
	char* myIP = getmyIP();
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
	my_addr.sin_port = htons(UDPPORT);
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
}

void talkOnTCP() {

}

int main(int argc, char* argv[]) {
	if(argc!=4) {
		cout<<"usage: <nodeID> <S or R> <md5Sum>"<<endl;
		//return -1;
	}
	parse_conf_file(confFile,nodes);
	int nodeid = atoi(argv[1]);
	sendUDP(nodeid,argv[2],argv[3]);
	
	
	return 0;
}

