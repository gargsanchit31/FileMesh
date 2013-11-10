/*
 * This file contains the functions parse_conf_file which parses the confugration file "FileMesh.cfg"
 *
 * Also it contains the following Structs
 * Message and node
 */


#ifndef _PARSER
#define _PARSER

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <stdlib.h>

#include <cstring>

using namespace std;

/*node is a struct that have the information about the nodes
 *Its ID                                    ID
 *Its IP address                            IP
 *Port on which its listening               PORT
 *path of the storage Folder                Folder_Path
 */
struct node{
	int ID;
	char IP[20];
	int PORT;
	char Folder_Path[100];
};

/* Message is a struct that stores the message information
 * Option : "Store" or "Get"
 * md5 : md5sum of the file
 * IP : IP of the User
 * Port : Port On which it listens
 */
struct Message{
    char Option[10];
    char md5[40];
    char IP[20];
    int Port;
};

struct thread_arg{
    char md5[40];
    int socket;
};


/*this function takes confugration file name and a vector as inputs and fill the details
 *of each node in the struct and put in the vector
 */
int parse_conf_file(char * file, vector<struct node> &v){
	ifstream conf_file (file);     //open the file
    string line;                   //read line in the variable line 
    int id=0;                       // id counter
    struct node temp;               // temporary node which is pushed back in vector

    if(conf_file){    // if the file is opened then
    	while(getline(conf_file, line)){       //get line until eof is encountered
            
            //read inputs as formatted string IP:PORT FOLDER
    		sscanf(line.c_str(),"%[^:]%*[:]%d%*[ ]%s", temp.IP, &temp.PORT, temp.Folder_Path);
    		temp.ID=id;           //assign the node ID
    		v.push_back(temp);    //push back in vector
    		id++;
    	}
    	conf_file.close();        //close the file
        return 1;
    }
    else{
    	cout<"not opened\n";
    	return 0;
    }
}

//print the node's information
void Print(std::vector<struct node> Nodes){
    for(int i=0;i<Nodes.size();i++){
        cout<<Nodes[i].ID<<" "<<Nodes[i].IP<<" "<<Nodes[i].PORT<<" "<<Nodes[i].Folder_Path<<endl;
    }
}

#endif
