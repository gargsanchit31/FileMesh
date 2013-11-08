#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include "parser.h"
using namespace std;

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
}