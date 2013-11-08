#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <vector>
#include <stdlib.h>

#include <cstring>

using namespace std;

struct node{
	int ID;
	char *IP;//[INET_ADDRSTRLEN];
	int PORT;
	char *Folder_Path;
};
/*
struct node Split(string l){
	struct node temp;
	char line[100];
	strcpy(line, l.c_str());
	char* pch;
	temp.IP=strtok(line, ": ");
	temp.PORT = atoi(strtok(NULL, " "));
	cout << pch << endl;
	pch = strtok(NULL, " ");
	cout << pch << endl;
	return temp;
}
*/
int parse_conf_file(char * file, vector<struct node> &v){
	ifstream conf_file (file);
    string line;
    int id=0;
    struct node temp;

    if(conf_file){
    	while(getline(conf_file, line)){
    		cout<<line<<endl;
    		sscanf(line,"%s:%d %s", temp.IP,&temp.PORT,temp.Folder_Path);
    		//temp = Split(line);
    		temp.ID=id;
    		cout<<temp.ID<<temp.IP<<temp.PORT<<temp.Folder_Path<<endl;
    		v.push_back(temp);
    		id++;
    	}

    	conf_file.close();
    }
    else{
    	cout<"not opened\n";
    	return 0;
    }
	return 1;
}

int main(int argc, char *argv[]){
	if(argc != 2) {
      cout << "Please give the confugration file as an argument .\n";
      return -1;
    }

    vector<struct node> Nodes;

    parse_conf_file(argv[1], Nodes);
    for (int i=0;i<Nodes.size();i++){
    	cout<<Nodes[i].ID<<endl;
    }
	return 0;
}