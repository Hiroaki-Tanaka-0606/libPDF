// class PDFVersion

#include <iostream>
#include <cstring>
#include "PDFVersion.hpp"
using namespace std;

PDFVersion::PDFVersion():
	error(false),
	valid(false)
{
	
}

bool PDFVersion::set(char* label){
	// label: "x.y"
	// (x, y) = (1, 0) ~ (1, 7), (2, 0)
	if(label[0]=='1'){
		major=1;
	}else if(label[0]=='2'){
		major=2;
	}else{
		cout << "Error 1 in version label" << endl;
		error=true;
		return false;
	}
	if(label[1]!='.'){
		cout << "Error 2 in version label" << endl;
		error=true;
		return false;
	}
	char minor_c[2];
	minor_c[0]=label[2];
	minor_c[1]='\0';
	minor=atoi(minor_c);
	if(major==1){
		if((label[2]=='0' || minor>0) && minor<=7){
			// ok
		}else{
			cout << "Error 3 in version label" << endl;
			error=true;
			return false;
		}
	}else if(major==2){
		if(label[2]=='0'){
			// ok
		}else{
			cout << "Error 4 in version label" << endl;
			error=true;
			return false;
		}
	}
	print();
	valid=true;
	return true;
}

void PDFVersion::print(){
	if(error){
		return;
	}
	sprintf(v, "%1d.%1d", major, minor);
	// cout << int(v[3]) << endl;
	// cout << strlen(v) << endl;
}

bool PDFVersion::isValid(){
	return (!error) && valid;
}
