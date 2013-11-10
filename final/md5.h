/*
 * this contains function which calculates the md5sum and a function which calculates the nodeID
 * of the corresponding calclated md5sum
 */
#ifndef _MD5
#define _MD5
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <math.h>
using namespace std;

string md5sum(string filename) {
	string b1 = "md5sum ";
	string output = ""; char c;
	b1 = b1+ filename+ "> a.txt";
	system("touch a.txt");
	system(b1.c_str());
	ifstream filein;
	filein.open("a.txt");
	//cout<<"fileopen"<<endl;
	c= filein.get();
	while (c != ' ') {
	output = output +c;
	//cout<<output<<" ";
	c=filein.get();
	}
	
	return output;
}

unsigned int hex_to_int(string k) {
	unsigned int x;
    	sscanf(k.c_str(), "%x", &x);
//    	cout<<x<<endl;
	return x;
}
int md5sumhash 	(char* m, int k){
	string s = (string)m;
	char szNumbers[36];
	string a,b,c,d;
	
	a = s.substr(0,8);
	b= s.substr(8,8);
	c = s.substr(16,8);
	d = s.substr(24,8);
	//cout<<a<<" "<<b<<" "<<c<<" "<<d<<endl;
	unsigned int w,x,y,z;

	w = hex_to_int(a);
	x = hex_to_int(b);
	y = hex_to_int(c);
	z = hex_to_int(d);

	long long int  b32 = (long long int )(pow(2,32))%k;
	int nodenum = (z%k + (b32 * y%k)%k + ( ((b32*b32)%k )*x%k)%k + (((b32*b32*b32)%k)*w%k)%k )%k;
	return nodenum;
}

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

#endif
