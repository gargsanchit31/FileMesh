/**The FileMeshNode program that implements the nodes in the mesh */
extern "C"
 {
    #include <pthread.h>
 }
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
#include <sstream>
#include <vector>
//#include <pthread.h>

#include "parser.h"			//The functions that parse the config file 
#include "md5.h"			//The md5sum hash and some other functions and calculations	
#define MAXBUFLEN 10000		//The maximum buffer length for send and recv in the TCP sockets
#define BACKLOG 6401        //Backlogged connections
#define CONF_FILE "./FileMesh.cfg"	//The configuration file

using namespace std;

vector<struct node> nodes;	//The vector that contains all the nodes' information after parsing 
struct node whoami;			//struct to let a node know about itself!

/**
The connectTCP(..) function opens a SOCK_STREAM socket to establish a TCP connection 
with the client and connects to it.
*/
int connectTCP(char *ip, int port){
	int sockfd;									//The socket used for TCP connection
	struct sockaddr_in their_add;				//The struct used to store all the information about the client
	
	their_add.sin_family = AF_INET; 			// host byte order
	their_add.sin_port = htons(port); 			// port in short, network byte order
	their_add.sin_addr.s_addr = inet_addr(ip);	//set the client's ip
	bzero(&(their_add.sin_zero), 8);			//equivalent to memset
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);		//Create the socket
	if (sockfd == -1) {							//Error check
		perror("listener: tcp socket");
	}
	
	if (connect(sockfd, (struct sockaddr *)&their_add,sizeof(struct sockaddr)) == -1) {		//Connect to the client
		perror("connection");
		exit(1);								//Exit in case of an error
	}
	return sockfd;								//return the socket descriptor
}


/**
The storeFile(..) function is called by a node to receive a file from the client (user) in
case of a Store request. It takes the socket descriptor on which TCP connection has been established
and the md5sum as arguments.
*/
void* storeFile(void* args){
	struct thread_arg* arg = (struct thread_arg *)args;
	char * md5 = arg->md5;
	int socketfd = arg->socket;
	char buf[MAXBUFLEN];						//The buffer to receive bytes from the user
	ssize_t numbytes=-1;						//Number of bytes received for file content
	int recv_size;								//Number of bytes received for file size
	if ((recv_size = recv(socketfd, buf, MAXBUFLEN-1, 0)) == 0){		//Receiving file size first
		printf("Error at recv\n");				//Error check
	}
	
    int file_size = atoi(buf);					//first recieved data is file size
    int remain_data = file_size - (recv_size - 10);		//calculate data remaining.    
	printf("length = %d\n",file_size);
	FILE *fout;									//file to write
	char* file_path = whoami.Folder_Path;		//get the node's file path to store the file	
	char tmp[100];
	strcpy(tmp,file_path);
	strcat(tmp,md5);						//append to it the md5sum
	fout = fopen(tmp, "wb");				//open the file to write
	fwrite(buf+10, sizeof(char), recv_size-10, fout);	//write the first data. This is to adjust the bytes that were received with file size.

	while(remain_data >0) {						//while data still remains to be received
		numbytes = recv(socketfd, buf, MAXBUFLEN-1 , 0) ;	
		fwrite(buf, sizeof(char), numbytes, fout);		//write bytes to file
        remain_data -= numbytes;						//update remaining data
        printf("Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	printf("Received file!\n");
	fclose(fout);								//close file
	close(socketfd);							//close socket
}

/**
The sendFile(..) function sends the requested file to the user. In case the user makes a Get request,
this function is called and over the already established TCP connection, the file is transferred.
*/
void* sendFile(void* args) {
	struct thread_arg* arg = (struct thread_arg *)args;
	char * md5 = arg->md5;
	int socketid = arg->socket;
	long lSize;											//The file size
	char* buffer;										//Storage buffer
	size_t result;							
	char file_size[10];									//For Storing file size 
	char* file_path = whoami.Folder_Path;				//Get the node's file path
	char tmp[100];

	strcpy(tmp,file_path);
	
	strcat(tmp,md5);								//Concatenate the md5 (filename)
	//cout<<"before oprening file: "<<socketid<<" "<<file_path<<" "<<tmp<<endl;
	FILE *file1 = fopen (tmp, "rb");				//open the file for reading
		if (file1!=NULL) {	
			fseek(file1,0,SEEK_END);					//Seek till the end of file
			lSize = ftell(file1);						//Get file size with ftell
			printf("File size : %d\n",lSize);
			rewind(file1);								//go back to the beginning of the file	
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
		file_path= NULL;
		delete file_path;
		fclose(file1);									//Close file
		close(socketid);								//Close socket
}



/**
sendUDPRequest(..) sends the client's message to the correct node in case the current node's ID
does not match the md5sum hash that it calculates.
*/
void sendUDPRequest(int nodeID, struct Message* msg) {
	size_t msg_size =0;									//size of the message
	int sockfd;											//Socket for UDP connection
	struct sockaddr_in their_addr;						//Struct to fill in connection details	
	
	msg_size = sizeof(struct Message);					//calculate message size
	
	socklen_t addr_len;									
	ssize_t numbytes=0;									//For number of bytes sent
    
	memset(&their_addr, 0, sizeof their_addr);					//Padding in my_addr
	their_addr.sin_family = AF_INET;							//socket family
	their_addr.sin_port = htons(nodes[nodeID].PORT);			//Network byte order
	their_addr.sin_addr.s_addr = inet_addr(nodes[nodeID].IP); 	//The correct node's IP
	
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);				//Create the socket
	if (sockfd == -1) {									//Error check
		perror("listener: socket");
	}	
	
	//Send the message to the node
	if ((numbytes = sendto(sockfd, (char *)msg, msg_size, 0, (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
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
	
	struct sockaddr_storage their_addr;						//To receive connection details
	socklen_t addr_len;										
	ssize_t numbytes=-1;									//Number of bytes received

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

	/* Receiving file size */
	while(true){
		char buf[MAXBUFLEN];									//Buffer to receive message
		numbytes = recvfrom(sockfd, buf, sizeof(struct Message), 0, (struct sockaddr *)&their_addr, &addr_len);	//Receive request from a user
		if(numbytes >= 0){
			struct Message *msg = (struct Message *)buf;				//message recieved storage
			cout<<"88888nb   "<<ntohs(msg->Port)<<endl;
			struct thread_arg	 t_arg;									//holds the argument to be passed in calling new thread
			int node_no = md5sumhash(msg->md5,nodes.size());			//calculate the md5sumhash of the md5sum received in the message 
			int iret1;
			

			if(node_no==whoami.ID){										//The node checks if the message is for itself or some other node by nodeID
				cout<<"yes ip matched\n"<<endl;
				int tcpsocket = connectTCP(msg->IP, ntohs(msg->Port));	//If this node is to serve to the request, establish a TCP connection with the client
				cout<<"tcpsocket: "<<tcpsocket<<endl;
				char tmp_md5[40];
				strcpy(tmp_md5, msg->md5);
				strcpy(t_arg.md5, tmp_md5);
				t_arg.socket = tcpsocket;

				if(strcmp(msg->Option,"Get")==0){					//In case of a Get request from the client -
					//send the file to client
					pthread_t *tcpThread =new pthread_t;
					iret1 = pthread_create( tcpThread, NULL, &sendFile, (void *)&t_arg);
					//sendFile(msg->md5,tcpsocket);					//Call the sendFile function. client gets the file!	
					if (iret1){
			        	printf("ERROR; return code from pthread_create() is %d\n", iret1);
			        	exit(-1);
			      	}

				}
				//In case of a Store request
				else{											 
					//recieve and store the file from client
					//storeFile(tcpsocket,msg->md5);					//storeFile called, the file gets stored to the node
					pthread_t *tcpThread =new pthread_t;
					iret1 = pthread_create( tcpThread, NULL, &storeFile, (void *)&t_arg);
					//sendFile(msg->md5,tcpsocket);					//Call the sendFile function. client gets the file!	
					if (iret1){
			        	printf("ERROR; return code from pthread_create() is %d\n", iret1);
			        	exit(-1);
			      	}
				}
			}
			//In case the request is to be served by some other node
			else{
				sendUDPRequest(node_no,msg);					//Send the message to the correct node (based on md5sumhash) over a UDP connetion
			}
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
	//int rc1,rc2;
	//pthread_t send_handler, receive_handler ; // declare 2 threads each to handle sending and receiving
	//struct thread_arg *t_arg;

	parse_conf_file(CONF_FILE,nodes);						//Parse the configuration file first
	int n_id = atoi(argv[1]);								//Get nodeID as int
	whoami=nodes[n_id];										//the whoami node struct stores information about the current node	
	cout<<whoami.ID<<","<<whoami.IP<<","<<whoami.PORT<<endl;

	listenUDPRequest();										//Start listening to a UDP connection to accept requests and then process them.
	return 0;
}
