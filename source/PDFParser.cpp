// class PDFParser

// White-space characters
#define NUL char(0x00)
#define TAB char(0x09)
#define LF  char(0x0A)
#define FF  char(0x0C)
#define CR  char(0x0D)
#define SP  char(0x20)
// EOL: CR, LF, CR+LF

// header and footer
#define HEADER "%PDF-"
#define FOOTER "%%EOF"
#define SXREF  "startxref"
#define TRAIL  "trailer"

// comment begins with a PERCENT sign
#define PER '%'


#include <fstream>
#include <iostream>
#include <cstring>
#include "PDFParser.hpp"

using namespace std;
PDFParser::PDFParser(char* fileName):
	// Member initializer list
	file(fileName),
	error(false)
{
	if(!file){
		cout << "Error in opening the input file" << endl;
		error=true;
		return;
	}

	// find "%PDF-x.x"
	if(findHeader()==false){
		error=true;
		return;
	}

	// go to the end
	file.seekg(0, ios_base::end);
	fileSize=file.tellg();
	printf("File size: %d bytes\n", fileSize);

	// find "%%EOL"
	if(!findFooter()){
		error=true;
		return;
	}
	// one line before
	backtoEOL();
	char xref_c[32];
	file.getline(xref_c, 32);
	int xref=atoi(xref_c);
	printf("Last XRef is at %d\n", xref);
	// one line before xref
	backtoEOL();
	backtoEOL();
	// this line should be "startxref"
  if(!findXref()){
		error=true;
		return;
	}
	// find "trailer" line
	cout << "startxref found" << endl;
}

bool PDFParser::hasError(){
	return error;
}

bool PDFParser::isSpace(char a){
	return a==NUL || a==TAB || a==LF || a==FF || a==CR || a==SP;
}

bool PDFParser::isEOL(char a){
	return a==CR || a==LF;
}

bool PDFParser::findHeader(){
	// go to the top
	file.seekg(0, ios_base::beg);
	char a;
	int line=0;
	while(true){
		// investigate a line
		int i;
		bool flag=false;
		for(i=0; i<strlen(HEADER); i++){
			file.get(a);
			if(a!=HEADER[i]){
				cout << "Find something before the header" << endl;
				if(a==CR || a==LF){
					file.seekg(-1, ios_base::cur);
				}
				gotoEOL();
				flag=true;
				break;
			}
		}
		if(flag){
			continue;
		}
		// after "%PDF-" is "x.y"
		char version[3];
		for(i=0; i<3; i++){
			file.get(version[i]);
			if(isEOL(version[i])){
				cout << "Error: invalid version label" << endl;
				return false;
			}
		}
		offset=(int)file.tellg()-8;
		if(!V_header.set(version)){
			return false;
		}
		if(!gotoEOL(1)){
			return false;
		}
		printf("Version in the header: %s\n", V_header.v);
		printf("Offset: %d bytes\n", offset);
		break;
	}
	
	return true;
}

// EOL: CR, LF, CR+LF
bool PDFParser::gotoEOL(){
	return gotoEOL(0);
}

// flag
// 0: any character is ok before EOL
// 1: white space characters are ok
// 2: nothing is ok
bool PDFParser::gotoEOL(int flag){
	char a;
	while(true){
		file.get(a);
		if(flag==1){
			if(!isSpace(a)){
				cout << "Invalid character, other than white-space" << endl;
				return false;
			}
		}else if(flag==2){
			if(!isEOL(a)){
				cout << "Invalid character, other than CR or LF" << endl;
				return false;
			}
		}
		if(a==CR){
			file.get(a);
			if(a!=LF){
				file.seekg(-1, ios_base::cur);
			}
			return true;
		}else if(a==LF){
			return true;
		}else if(a==EOF){
			return false;
		}
	}
}

bool PDFParser::findFooter(){
	// find "%%EOF"
	file.seekg(0, ios_base::end);
	backtoEOL();
	int i;
	char a;
	for(i=0; i<strlen(FOOTER); i++){
		file.get(a);
		if(a!=FOOTER[i]){
			cout << "Error in footer" << endl;
			return false;
		}
	}
	if(!gotoEOL(1)){
		cout << "Invalid character in footer" << endl;
		return false;
	}
	backtoEOL();
	return true;
}

// if the pointer is just after EOL, back to another EOL before
// if the pointer is not on EOL, back to the last EOL
void PDFParser::backtoEOL(){
	char a;
	file.seekg(-1, ios_base::cur);
	file.get(a);
	if(isEOL(a)){
		// the pointer is just after EOL
		if(a==LF){
			file.seekg(-2, ios_base::cur);
			file.get(a);
			if(a==CR){
				// EOL is CR+LF
				file.seekg(-1, ios_base::cur);
			}else{
				// EOL is LF
			}
		}else{
			// EOL is CR
		}
	}else{
		// the pointer is not on EOL
	}
	while(true){
		file.seekg(-1, ios_base::cur);
		file.get(a);
		if(isEOL(a)){
			break;
		}else{
			file.seekg(-1, ios_base::cur);
		}
	}
}

bool PDFParser::findXref(){
	// startxref
	int i;
	char a;
	for(i=0; i<strlen(SXREF); i++){
		file.get(a);
		if(a!=SXREF[i]){
			cout << "Error in startxref" << endl;
			return false;
		}
	}
	if(!gotoEOL(1)){
		cout << "Invalid character in startxref" << endl;
		return false;
	}
	backtoEOL();
	return true;
}
