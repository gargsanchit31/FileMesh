#include<iostream>
#include<string>
#include<stdlib.h>
#include<string>
#include<fstream>
#include<math.h>
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
int md5sumhash 	(string s, int k){
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
int main(int argc, char* argv[]) {
	
	string s = argv[1];
	//cout<<s<<endl;
	s = md5sum(s);
	cout<<s<<endl;	
	int k = md5sumhash(s,10);
	cout<<"mapping: "<<k<<endl;
	return 1;
}
