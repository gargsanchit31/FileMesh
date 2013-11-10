//This file implements a user or a client
//N.B. In case a file is to be stored at a server, its md5sum hash is calculated and 
// sent , and it is saved by this same name at a server.
//In case a file is to be retrived, the user knows the md5sum value of the file the server 
//has stored and it requests by that name(md5sum value).

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
#include <time.h> 
#include "parser.h"
#include "md5.h"
#define TCPPORT 6401		//the tcpport on which the user will be listening
#define MAXBUFLEN 10000		//Maximum buffer length for receiving
#define BACKLOG 6401 		// The max number of connections that can be made to the tcp socket
#define confFile "FileMesh.cfg"		// The configuration file to create the mesh of nodes

#include <vector>
using namespace std;

vector<struct node> nodes;		// vector contains the information of all the nodes in mesh

// Function to send the initial udp connection request to node nodeID
//opens a udp socket and sends a file's md5sum value either asking to store a file
//or to retrive a file stored by its md5sum value in the option field
//also sends its own ip address for the corresponding node to set a tcp connection to it

void sendUDPRequest(int nodeID, struct Message msg) {

	size_t msg_size = sizeof(struct Message);
	struct node node_info = nodes[nodeID];		//nodeId is the node it is to connect to
	int sockfd;									//socket descriptor
	struct sockaddr_in their_addr;		//structs containing socket address information
	ssize_t numbytes=0;					//variable for tracking bytes sent
    
	memset(&their_addr, 0, sizeof their_addr);				//filling data in the struct
	their_addr.sin_family = AF_INET;						//Address Family IPv4
	their_addr.sin_port = htons(nodes[nodeID].PORT);		//server's port with which it is to connect
	their_addr.sin_addr.s_addr = inet_addr(node_info.IP);	//server's ip with which it is to connect
	
	sockfd = socket(AF_INET,SOCK_DGRAM,0);		//udp socket initialisation
	if (sockfd == -1) {							//error checking of socket initialisation
		perror("listener: socket");
	}	
	
	//Sending udp packet containing "msg" as data with error checking
	if ((numbytes = sendto(sockfd, (char *)&msg, msg_size, 0, (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
	printf("talker: sent %d bytes\n", numbytes);
	close(sockfd);		//closing socket
}

//Function to bind a tcp socket to a sockaddr_in struct and listen on a incomming tcp connect request
//p.s. error checking exits on error to prevent further damage
//takes self IP as input and returns the socket descriptor it has opened
int bindOnTCP(char* myIP, int* port_no) {
	//variable lnitialisiation
	int sockfd,bd;
	struct sockaddr_in my_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	
	//tcp socket create with error checking
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd == -1) {
		perror("listener: socket");
	}
	
	// filling data into struct sockaddr_in 
	memset(&my_addr, 0, sizeof my_addr);
	my_addr.sin_family = AF_INET;
	//my_addr.sin_port = htons(TCPPORT);				//tcpport user is listening
	//my_addr.sin_port =0;
	
	srand(time(NULL));
	int port = rand()%50000;
	port+=2000;
	my_addr.sin_port = htons(port);				//tcpport user is listening
	my_addr.sin_addr.s_addr = inet_addr(myIP);		//Self IP
	//my_addr.sin_port = ;				//tcpport user is listening
	// reuse address for the socket referred to by the file descriptor sockfd
	/*int yes=1;
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}*/

	//binding of socket to struct for further listening
	bd = bind(sockfd, (struct sockaddr *) &my_addr, sizeof my_addr);
	if (bd == -1) {
		close(sockfd);
		perror("listener: bind");
	}


	//listening on the socket with max connections possible = BACKLOG
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	
	struct sockaddr_in mine;
	getsockname(sockfd,(struct sockaddr*)&my_addr, &addr_len);
	cout<< ntohs(my_addr.sin_port)<<endl;
	

	
	*port_no = my_addr.sin_port;
	printf("listener: waiting to recv...\n");
	return sockfd;
}

//finally we are to store the file at a server....3 cheers, hip hip hurray!!!
//IT takes the file to be sent for storage and the socket id on which it is connected.
/**
The storeFileTCP(..) function sends a file to  a node for  storage. In case the user makes a Store request,
this function is called and over the already established TCP connection, the file is transferred.
takes path of the file and the socketid as input
*/
void storefileTCP(char* file_path, int socketid) {
	//initialising varriables
	long lSize;				//store file size
	char* buffer;			//storage buffer
	size_t result;
	char file_size[10];		//storing the file size
	
	FILE *file1 = fopen (file_path, "rb");						//open the file for reading
		if (file1!=NULL) {
			fseek(file1,0,SEEK_END);							//Seek till the end of file
			lSize = ftell(file1);								//Get file size with ftell
			rewind(file1);										//go back to the beginning of the file
			printf("File size : %d\n",lSize);
			sprintf(file_size, "%d", lSize);					//lSize sent as char*
			
			int len = send(socketid, file_size, 10, 0);			//send the file size over the socket
			printf("len = %d\n", len);

			//Error check
			if (len < 0) {
				fprintf(stderr, "Error! %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			int numbytes;										//bytes sent
			long fs = lSize;									//fs now stores file size
			char line[110];										//buffer for transfer
			while(fs > 0) {
				int chunk= 100;									//transfer 'chunk' bytes at a time
				if(fs < 100){									//set chunk the fs if less than 'chunk'
					chunk = fs;
				}
				fread(line,1,chunk,file1);						//read the file
				
				//sending bytes over TCP
				if ((numbytes = send(socketid, line, chunk, 0)) == -1) {
					perror("talker: sendto");
					exit(1);
				}
				//print the bytes remaining and bytes sent
				printf("talker: sent %d bytes\n", numbytes);
				fs-=numbytes;		//Bytes remaining
				printf("talker: remaining %d bytes\n", fs);
			}
			printf("File sent!\n");
		}
		fclose(file1);			//Close file
		close(socketid);		//close the socket
}

/**
The receivefileTCP(..) function is called by the client to receive a file from a node in
case of a Get request. It takes the socket descriptor on which TCP connection has been established
and the md5sum as arguments.
*/
void receivefileTCP(const char* md5, int socketfd){


	char buf[MAXBUFLEN];							//The buffer to receive bytes from the user
	ssize_t numbytes=-1;							//Number of bytes received for file content
	int recv_size;									//Number of bytes received for file size
	if ((recv_size = recv(socketfd, buf, MAXBUFLEN-1, 0)) == 0){		//Receiving file size first
		//Error check
		printf("Error at recv\n");
	}
	
    int file_size = atoi(buf);	//first recieved data is file size
    int remain_data = file_size - (recv_size - 10);    	//calculate data remaining.
	printf("length = %d",file_size);
	printf("\n");
	FILE *fout;											//file to write
	fout = fopen(md5, "wb");							//open the file to write
	fwrite(buf+10, sizeof(char), recv_size-10, fout);	//write the first data. This is to adjust the bytes that were received with file size.

	while(remain_data >0) {								//while data still remains to be received
		numbytes = recv(socketfd, buf, MAXBUFLEN-1,0);	//receive bytes
		fwrite(buf, sizeof(char), numbytes, fout);		//write bytes to file
        remain_data -= numbytes;						//update remaining data
        printf("Received %d bytes, To receive :- %d bytes\n", numbytes, remain_data);
	}
	printf("Received file!\n");
	cout<<recv_size<<endl;
	fclose(fout);			//close file
	close(socketfd);		//close socket descriptor
}

//Fuction to accept incoming tcp connection to user from server/node using the 
//socket on which it was listening for incoming tcp connection
//takes the listening socket, option=(Store/Get) and filename to act upon
//the option  and filename field is the same field it sent in the original
//udp request it sent
void acceptTCP(int sockfd, char* option, char* filename) {
	

	int new_fd;
	string storage_path;								//where the file should be stored
	struct sockaddr_storage their_addr;					//other struct to captue incomming request's host info
	
	unsigned int sin_size = sizeof(struct sockaddr_in);	//size of struct sockaddr_in
	printf("listener: waiting to accept...\n");

	if(strcmp(option, "Get")==0){
		cout<<"Enter the <file/path/filename>: ";
		cin>>storage_path;
	}

	//assinging new socket to capture this connection's data while freeing the original csocket 
	//for further listening
	if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
		perror("accept");
	}
	
	cout<<"connection accepted"<<endl;

	//if option = "Store", act accordingly and call storefileTcp
	if(strcmp(option, "Store")==0){
		storefileTCP(filename,new_fd);		//calling storefileTCP...feeling excited!!!!!
	}
	//else if option is equal = Get, again act accordingly and call receivefileTCP, surprise!!! :P
	else if(strcmp(option, "Get")==0){
		receivefileTCP(storage_path.c_str(),new_fd);	//receivefileTCP....yippee????
	}
	//else......?/???
	else{
		cout<<"Something went wrong. We will figure it out soon."<<endl;		
	}
}
//input nodeid, request tpye, filename/md5sum
int main(int argc, char* argv[]) {
	char* myIP;
	struct Message msg;							//message struct contains message info

	//error checking for correct format
	if(argc!=4) {
		cout<<"usage:<executable> <nodeID> <Store or Get> <filename/md5sum>"<<endl;
		return -1;
	}

	if(!(strcmp(argv[2], "Store")==0) and !(strcmp(argv[2] , "Get") ==0)){
		cout<<"invalid request: enter Store or Get"<<endl;
		return -1;
	}

	myIP = getmyIP();								//myIP = myIP i.e. ip of user
	parse_conf_file(confFile,nodes);				//parsing configuration file to know details about nodes
	string filename = argv[3];						//filename to retrieve or store
	string md5;

	//if store, calculate the md5sum value of the file
	if((strcmp(argv[2],"Store")) == 0) {
		md5 = md5sum(filename);
	}
	//if get, the parameter is already the md5sum value by which the required file 
	//at the server, go get it
	else {
		md5=(string)argv[3];
	}
	
	const char* md =md5.c_str();
	int nodeid = atoi(argv[1]);

	//filling the message details 
	strcpy(msg.Option,argv[2]);
	strcpy(msg.md5,md);
	strcpy(msg.IP,myIP);
	

	//int tcpsockfd = bindOnTCP(myIP);			//First bind the TCP for incoming connections
	int tcpsockfd = bindOnTCP(myIP, &(msg.Port));			//First bind the TCP for incoming connections
	//msg.Port = htonl(TCPPORT);
	cout<<msg.Port<<endl;
	sendUDPRequest(nodeid,msg);					//send the UDP request
	acceptTCP(tcpsockfd,argv[2],argv[3]);		//then wait to accept the tcp request which is coming 

	//release the memory
	myIP=NULL;
	delete myIP;
	return 0;
}

