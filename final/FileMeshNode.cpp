/**The FileMeshNode program that implements the nodes in the mesh */
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
#include "parser.h"			//The functions that parse the config file 
#include "md5.h"			//The md5sum hash and some other functions and calculations
//#define TCPPORT 6401		
#define MAXBUFLEN 10000		//The maximum buffer length for send and recv in the TCP sockets
#define BACKLOG 6401        //Backlogged connections
#define CONF_FILE "./FileMesh.cfg"	//The configuration file
#include <sstream>
#include <vector>
#define DELIM ":"			//Delimiter used for the message that the client sends to the node
using namespace std;

vector<struct node> nodes;	//The vector that contains all the nodes' information after parsing 
struct node whoami;			//struct to let a node know about itself!

/**
This parseMsg(..) function takes the message that the client sends to the node
and parses, recovers the information about Store/Get operation, md5sum of the file,
ip address of the client and the tcp port on which it listens.
*/
void parseMsg(char* msg1, char **option, char **md5, char **ip, int& port){
	char *pch;									//used to temporarily store the item recovered from msg
	char msg[100];								//used to convert msg1 to char*
	strcpy(msg,msg1);							//copy to msg, msg1
	pch = strtok (msg,":");						//recover upto first occurence of ":"
	*option = (char*)malloc(10*sizeof(char));	
	strcpy(*option,pch);						//copy characters recovered in pch to options
	pch = strtok (NULL,":");					//recover upto second occurence of ":"
	*md5 = (char*)malloc(40*sizeof(char));
	strcpy(*md5,pch);							//copy characters recovered in pch to md5
	pch = strtok (NULL,":");
	*ip = (char*)malloc(10*sizeof(char));
	strcpy(*ip,pch);							//copy characters recovered in pch to ip
	pch = strtok (NULL,":");
	port = atoi(pch);							//convert pch to int to recover integer port
}

/**
The connectTCP(..) function opens a SOCK_STREAM socket to establish a TCP connection 
with the client and connects to it.
*/
int connectTCP(char *ip, int port){
	int sockfd;									//The socket used for TCP connection
	struct sockaddr_in their_add;				//The struct used to store all the information about the client
	
	their_add.sin_family = AF_INET; 			// host byte order
	//cout<<"in connectTCP: port no. "<<port<<endl;
	their_add.sin_port = htons(port); 			// port in short, network byte order
	their_add.sin_addr.s_addr = inet_addr(ip);	//set the client's ip
	bzero(&(their_add.sin_zero), 8);			//equivalent to memset
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);		//Create the socket
	if (sockfd == -1) {							//Error check
		perror("listener: tcp socket");
	}
	
	if (connect(sockfd, (struct sockaddr *)&their_add,sizeof(struct sockaddr)) == -1) {		//Connect to the client
		//cout<<"dsfsafsdaf"<<endl;
		perror("connect");
		exit(1);								//Exit in case of an error
	}
	return sockfd;								//return the socket descriptor
}


/**
The storeFile(..) function is called by a node to receive a file from the client (user) in
case of a Store request. It takes the socket descriptor on which TCP connection has been established
and the md5sum as arguments.
*/
void storeFile(int socketfd, char* md5){
	char buf[MAXBUFLEN];						//The buffer to receive bytes from the user
	ssize_t numbytes=-1;						//Number of bytes received for file content
	int recv_size;								//Number of bytes received for file size
	if ((recv_size = recv(socketfd, buf, MAXBUFLEN-1, 0)) == 0){		//Receiving file size first
		printf("Error at recv\n");				//Error check
	}
	
    int file_size = atoi(buf);					//first recieved data is file size
    int remain_data = file_size - (recv_size - 10);		//calculate data remaining.    
	printf("length = %d",file_size);
	printf("\n");
	FILE *fout;									//file to write
	char* file_path = whoami.Folder_Path;		//get the node's file path to store the file	
	strcat(file_path,md5);						//append to it the md5sum
	fout = fopen(file_path, "wb");				//open the file to write
	fwrite(buf+10, sizeof(char), recv_size-10, fout);	//write the first data. This is to adjust the bytes that were received with file size.

	while(remain_data >0) {						//while data still remains to be received
		numbytes = recv(socketfd, buf, MAXBUFLEN-1 , 0) ;	
		fwrite(buf, sizeof(char), numbytes, fout);		//write bytes to file
        remain_data -= numbytes;						//update remaining data
        printf("Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	printf("Received file!\n");
	cout<<recv_size<<endl;
	fclose(fout);								//close file
	close(socketfd);							//close socket
}

/**
The sendFile(..) function sends the requested file to the user. In case the user makes a Get request,
this function is called and over the already established TCP connection, the file is transferred.
*/
void sendFile(char* md5, int socketid) {
	long lSize;											//The file size
	char* buffer;										//Storage buffer
	size_t result;							
	char file_size[10];									//For Storing file size 
	char* file_path = whoami.Folder_Path;				//Get the node's file path
	strcat(file_path,md5);								//Concatenate the md5 (filename)
	FILE *file1 = fopen (file_path, "rb");				//open the file for reading
		if (file1!=NULL) {	
			fseek(file1,0,SEEK_END);					//Seek till the end of file
			lSize = ftell(file1);						//Get file size with ftell
			rewind(file1);								//go back to the beginning of the file	
			printf("File size : %d\n",lSize);
			sprintf(file_size, "%d", lSize);			//lSize sent as char*
			
			int len = send(socketid, file_size, 10, 0);	//send the file size over the socket
			printf("len = %d\n", len);
			if (len < 0) {								//Error check
				fprintf(stderr, "Error! %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			int numbytes;								//bytes sent
			long fs = lSize;							//fs now stores file size
			char line[110];								//buffer for transfer
			while(fs > 0) {
				int chunk= 100;							//chunk to transfer 'chunk' bytes at a time
				if(fs < 100){
					chunk = fs;							//modify chunk if less than chunk to send
				}
				fread(line,1,chunk,file1);				//read the file
				//printf("chunk is %d, strlen is %d", chunk, strlen(line));
				
				if ((numbytes = send(socketid, line, chunk, 0)) == -1) {	//sending bytes over TCP
					perror("talker: sendto");
					exit(1);
				}
				printf("talker: sent %d bytes\n", numbytes);
				fs-=numbytes;							//Bytes remaining
				printf("talker: remaining %d bytes\n", fs);
			}
			printf("File sent!\n");
		}
		fclose(file1);									//Close file
		close(socketid);								//Close socket
}



/**
sendUDPRequest(..) sends the client's message to the correct node in case the current node's ID
does not match the md5sum hash that it calculates.
*/
void sendUDPRequest(int nodeID, char* msg) {
	size_t msg_size =0;									//size of the message
	//char* myIP = getmyIP();
	int sockfd;											//Socket for UDP connection
	struct sockaddr_in my_addr;							//Struct to fill in connection details	
	
	msg_size = strlen(msg);								//calculate message size
	//cout<<msg_size << " " << sizeof(char*)<<endl;
	
	socklen_t addr_len;									
	ssize_t numbytes=0;									//For number of bytes sent
    
	memset(&my_addr, 0, sizeof my_addr);				//Padding in my_addr
	my_addr.sin_family = AF_INET;						//socket family
	my_addr.sin_port = htons(nodes[nodeID].PORT);		//Network byte order
	my_addr.sin_addr.s_addr = inet_addr(nodes[nodeID].IP); 	//The correct node's IP
	
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);				//Create the socket
	if (sockfd == -1) {									//Error check
		perror("listener: socket");
	}	
	
	if ((numbytes = sendto(sockfd, msg, msg_size, 0, (struct sockaddr *)&my_addr, sizeof my_addr)) == -1) {		//Send the message to the node
		perror("talker: sendto");						//Error check
		exit(1);
	}
	printf("talker: sent %d bytes\n", numbytes);
	close(sockfd);										//Close the socket
}

/**
listenUDPRequest() is the main function that makes all the nodes listen to a UDP connection initially.
The client/user sends a Store/Get request to any of the nodes over this UDP connection. And then the node
which receives this requests acts on it accordingly.
*/
void listenUDPRequest(){
	int sockfd,bd;											//The UDP socket
	struct sockaddr_in my_addr;								//Struct to fill in connection details
	char buf[MAXBUFLEN];									//Buffer to receive message
	struct sockaddr_storage their_addr;						//To receive connection details
	socklen_t addr_len;										
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);					//Create the socket
	if (sockfd == -1) {
		perror("listener: socket");							//Error check
	}
	
	memset(&my_addr, 0, sizeof my_addr);					//Padding for the struct
	my_addr.sin_family = AF_INET;							//Socket family
	my_addr.sin_port = htons(whoami.PORT);					//Port in network byte order
	my_addr.sin_addr.s_addr = inet_addr(whoami.IP);			//The node's IP address	
	
	bd = bind(sockfd, (struct sockaddr *) &my_addr, sizeof my_addr);	//Bind the node to listen on its port
	if (bd == -1) {
		close(sockfd);										//Error check
		perror("listener: bind");
	}
	ssize_t numbytes=-1;									//Number of bytes received
	//printf("listener: waiting to recvfrom...\n");
	
	/* Receiving file size */
    numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addr_len);	//Receive request from a user
	if(numbytes >= 0){
		char *option,*md5,*ip;								//variables used to parse/decode the user message
		int port;											
		parseMsg(buf,&option,&md5,&ip,port);				//parsing the message from the user with these variables
		cout<<option<<" "<<md5<<" "<<ip<<" "<<port<<endl;
		int node_no = md5sumhash(md5,nodes.size());			//calculate the md5sumhash of the md5sum received in the message 

		cout<<node_no<<","<<whoami.ID<<endl;

		if(node_no==whoami.ID){								//The node checks if the message is for itself or some other node by nodeID
			cout<<"yes ip matched\n"<<endl;
			int tcpsocket = connectTCP(ip, port);			//If this node is to serve to the request, establish a TCP connection with the client
			if(strcmp(option,"Get")==0){					//In case of a Get request from the client -
															//send the file to client
				sendFile(md5,tcpsocket);					//Call the sendFile function. client gets the file!					
			}
			else{											//In case of a Store request - 
															//recieve and store the file from client
				storeFile(tcpsocket,md5);					//storeFile called, the file gets stored to the node
			}
		}
		else{												//In case the request is to be served by some other node -
			//cout<<buf<<endl;
			sendUDPRequest(node_no,buf);					//Send the message to the correct node (based on md5sumhash) over a UDP connetion
		}
	}    
}

/**
The main function which calls the appropriate functions to make the nodes work.
It takes only one argument, the nodeID to know which node to start.
*/
int main(int argc, char* argv[]){
	if(argc!=2){											//Error check for number of commnad line arguments
		cout<<"usage: <executable> <Node ID>"<<endl;		//Run the executable with nodeID as command line argument
		return -1;
	}

	parse_conf_file(CONF_FILE,nodes);						//Parse the configuration file first
	int n_id = atoi(argv[1]);								//Get nodeID as int
	whoami=nodes[n_id];										//the whoami node struct stores information about the current node	
	cout<<whoami.ID<<","<<whoami.IP<<","<<whoami.PORT<<endl;
	listenUDPRequest();										//Start listening to a UDP connection to accept requests and then process them.
	return 0;
}
