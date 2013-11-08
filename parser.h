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
 *Its IP                                    ID
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
    		temp.ID=id;   //assign the node ID
    		v.push_back(temp);    //push back in vector
    		id++;
    	}
    	conf_file.close();     //close the file
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
/*
//main function
int main(int argc, char *argv[]){
    //check if the file name has entered or not
	if(argc != 2) {
      cout << "Please give the confugration file as an argument .\n";
      return -1;
    }
    vector<struct node> Nodes;  //Vector to store the nodes information
    //parse the file
    parse_conf_file(argv[1], Nodes);
    //Print(Nodes);
    
	return 0;
}*/

#endif