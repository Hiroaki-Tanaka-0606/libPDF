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
#define XREF   "xref"


#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
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
	readInt(&lastXRef);
	printf("Last XRef is at %d\n", lastXRef);
	// one line before xref
	backtoEOL();
	backtoEOL();
	// this line should be "startxref"
  if(!findSXref()){
		error=true;
		return;
	}
	cout << "startxref found" << endl;

	// read trailer(s)

	int prevXRef=lastXRef;
	while(true){
		prevXRef=readTrailer(prevXRef);
		if(prevXRef==0){
			// This is the first Trailer
			break;
		}else if(prevXRef<0){
			// error
			error=true;
			return;
		}
	}

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

bool PDFParser::isDelimiter(char a){
	return a=='(' || a==')' || a=='<' || a=='>' || a=='[' || a==']' || a=='/' || a=='%' || a=='{' || a=='}';
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

bool PDFParser::findSXref(){
	// startxref
	int i;
	char a;
	for(i=0; i<strlen(SXREF); i++){
		file.get(a);
		if(a!=SXREF[i]){
			cout << "Error in startxref " << a << endl;
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

void PDFParser::readLine(char* buffer, int n){
	// n is buffer size, so (n-1) characters other than \0 can be read
	char a;
	int i;
	for(i=0; i<n-1; i++){
		file.get(a);
		if(!isEOL(a)){
			buffer[i]=a;
		}else{
			buffer[i]='\0';
			file.seekg(-1, ios_base::cur);
			break;
		}
	}
	buffer[n-1]='\0';
	gotoEOL();
}

int PDFParser::readTrailer(int position){
	printf("Read trailer at %d\n", position);
	file.seekg(position+offset, ios_base::beg);

  // XRef table: begins with "xref" line
	bool XRefTable=true;
	int i;
	char a;
	for(i=0; i<strlen(XREF); i++){
		file.get(a);
		if(a!=XREF[i]){
			XRefTable=false;
			break;
		}
	}
	if(XRefTable && !gotoEOL(1)){
		XRefTable=false;
	}
	file.seekg(position, ios_base::beg);
	
	// XRef stream: begins with "%d %d obj" line
	int objNumber;
	int genNumber;
	bool XRefStream=readObjID(&objNumber, &genNumber);
	file.seekg(position, ios_base::beg);

	if(!XRefTable && !XRefStream){
		cout << "Error: none of XRef Table or XRef Stream" << endl;
		return -1;
	}

	char buffer[32];
	if(XRefTable){
		cout << "Skip XRef table" << endl;
	  int i;
		char a;
		while(true){
			bool flag=false;
			for(i=0; i<strlen(TRAIL); i++){
				file.get(a);
				if(a!=TRAIL[i]){
					flag=true;
					break;
				}
			}
			if(flag){
				gotoEOL();
				continue;
			}
			if(!gotoEOL(1)){
				return -1;
			}
			break;
		}
				 
	}else{
		cout << "Skip ObjID raw" << endl;
		readLine(buffer, 32);
	}

	// read Trailer dictionary
	if(!readDict(trailer)){
		return -1;
	}
	return 0;
}

bool PDFParser::readObjID(int* objNumber, int* genNumber){
	// Indirect object identifier raw: %d %d obj
	readInt(objNumber);
	readInt(genNumber);
	char buffer[32];
	if(!readToken(buffer, 32)){
		return false;
	}
	if(strcmp(buffer, "obj")!=0){
		return false;
	}
	if(!gotoEOL(1)){
		return false;
	}
	if(*objNumber>0 && *genNumber>=0){
		return true;
	}else{
		return false;
	}
}

bool PDFParser::readRefID(int* objNumber, int* genNumber){
	// Indirect object reference raw: %d %d obj
	readInt(objNumber);
	readInt(genNumber);
	char buffer[32];
	if(!readToken(buffer, 32)){
		return false;
	}
	if(strcmp(buffer, "R")!=0){
		return false;
	}
	if(*objNumber>0 && *genNumber>=0){
		return true;
	}else{
		return false;
	}
}

bool PDFParser::readInt(int* value){
	char buffer[32];
	if(!readToken(buffer, 32)){
		return false;
	}
	char* pointer;
	*value=strtol(buffer, &pointer, 10);
	if(pointer==&buffer[strlen(buffer)]){
		return true;
	}else{
		return false;
	}
}

bool PDFParser::readReal(double* value){
	char buffer[32];
	if(!readToken(buffer, 32)){
		return false;
	}
	char* pointer;
	*value=strtod(buffer, &pointer);
	if(pointer==&buffer[strlen(buffer)]){
		return true;
	}else{
		return false;
	}
}

bool PDFParser::readToken(char* buffer, int n){
	if(!skipSpaces()){
		return false;
	}
	char a;
	int i=0;
	while(true){
		file.get(a);
		if(isSpace(a) || isDelimiter(a)){
			buffer[i]='\0';
			file.seekg(-1, ios_base::cur);
			return true;
		}else{
			buffer[i]=a;
			i++;
			if(i>=n-1){
				// token is too long
				buffer[n-1]='\0';
				return false;
			}
		}
	}
}

bool PDFParser::skipSpaces(){
	char a;
	while(true){
		file.get(a);
		if(isSpace(a)){
			continue;
		}else if(a==EOF){
			return false;
		}else{
			break;
		}
	}
	file.seekg(-1, ios_base::cur);
	return true;
}

bool PDFParser::readDict(Dictionary dict){
	// begins with "<<"
	if(!skipSpaces()){
		return false;
	}
	if(judgeDelimiter(true)!=Delimiter::LLT){
		return false;
	}
	
	// key (name), value (anything)
	while(true){
		int delim=judgeDelimiter(false);
		if(delim==Delimiter::SOL){
			// /name
		  unsigned char* name=readName();
			if(name==NULL){
				return false;
			}
			// value
			int type=judgeType();
			printf("Name: %s, Type: %d\n", name, type);
			char buffer[128];
			unsigned char* name2;
			int v1, v2;
			Dictionary dict2;
			Array array2;
			switch(type){
			case Type::Bool:
			case Type::Int:
			case Type::Real:
			case Type::Null:
				readToken(buffer, 128);
				break;
			case Type::String:
			  name2=readString();
				break;
			case Type::Name:
			  name2=readName();
				break;
			case Type::Indirect:
				readRefID(&v1, &v2);
				break;
			case Type::Dict:
				readDict(dict2);
				break;
			case Type::Array:
				readArray(array2);
				break;
			}
			//break;
		}else if(delim==Delimiter::GGT){
			// >> (end of dictionary)
			judgeDelimiter(true);
			break;
		}
	}
	return true;
}

bool PDFParser::readArray(Array array){
	// begins with "["
	if(!skipSpaces()){
		return false;
	}
	if(judgeDelimiter(true)!=Delimiter::LSB){
		return false;
	}
	
	// value (anything)
	while(true){
		int delim=judgeDelimiter(false);
		if(delim!=Delimiter::RSB){
			int type=judgeType();
			printf("Type: %d\n", type);
			char buffer[128];
			unsigned char* name2;
			int v1, v2;
			Dictionary dict2;
			Array array2;
			switch(type){
			case Type::Bool:
			case Type::Int:
			case Type::Real:
			case Type::Null:
				readToken(buffer, 128);
				break;
			case Type::String:
			  name2=readString();
				break;
			case Type::Name:
				name2=readName();
				break;
			case Type::Indirect:
				readRefID(&v1, &v2);
				break;
			case Type::Dict:
				readDict(dict2);
				break;
			case Type::Array:
				readArray(array2);
				break;
			}
		}else{
			// ] (end of array)
			judgeDelimiter(true);
			break;
		}
	}
	return true;
}


int PDFParser::judgeDelimiter(bool skip){
	int pos=(int) file.tellg();

	int delimID=-1;
	if(!skipSpaces()){
		return -1;
	}
	char a1, a2;
	file.get(a1);
	if(a1=='('){
		delimID=Delimiter::LP;
	}else if(a1==')'){
		delimID=Delimiter::RP;
	}else if(a1=='<'){
		file.get(a2);
		if(a2=='<'){
			delimID=Delimiter::LLT;
		}else{
			file.seekg(-1, ios_base::cur);
			delimID=Delimiter::LT;
		}
	}else if(a1=='>'){
		file.get(a2);
		if(a2=='>'){
			delimID=Delimiter::GGT;
		}else{
			file.seekg(-1, ios_base::cur);
			delimID=Delimiter::GT;
		}
	}else if(a1=='['){
		delimID=Delimiter::LSB;
	}else if(a1==']'){
		delimID=Delimiter::RSB;
	}else if(a1=='{'){
		delimID=Delimiter::LCB;
	}else if(a1=='}'){
		delimID=Delimiter::RCB;
	}else if(a1=='/'){
		delimID=Delimiter::SOL;
	}else if(a1=='%'){
		delimID=Delimiter::PER;
	}	
	
	if(!skip){
		file.seekg(pos, ios_base::beg);
	}
	return delimID;
}

unsigned char* PDFParser::readName(){
	if(!skipSpaces()){
		return NULL;
	}
	char a;
	file.get(a);
	if(a!='/'){
		return NULL;
	}
	string value1=string("");
	while(true){
		file.get(a);
		if(isSpace(a) || isDelimiter(a)){
			file.seekg(-1, ios_base::cur);
			break;
		}else{
			value1+=a;
		}
	}
	// conversion: #xx -> an unsigned char
	unsigned char* ret=new unsigned char[value1.length()+1];
	int i;
	int j=0;
	for(i=0; i<value1.length(); i++){
		if(value1[i]!='#'){
			ret[j]=(unsigned char)value1[i];
			j++;
		}else{
			ret[j]=(unsigned char)stoi(value1.substr(i+1, 2), nullptr, 16);
			j++;
			i+=2;
		}
	}
	ret[i]='\0';
	return ret;
}


unsigned char* PDFParser::readString(){
	if(!skipSpaces()){
		return NULL;
	}
	char a;
	file.get(a);
	if(a!='<' && a!='('){
		return NULL;
	}
	char startDelim=a;
	char endDelim= startDelim=='(' ? ')' : '>';
	string value1=string("");
	while(true){
		file.get(a);
		if(a==endDelim){
			break;
		}else{
			value1+=a;
		}
	}
	return NULL;
}

int PDFParser::judgeType(){
	if(!skipSpaces()){
		return -1;
	}
	int delimID=judgeDelimiter(false);
	// begins with delimiter: string, name, dict, array
	if(delimID==Delimiter::LP || delimID==Delimiter::LT){
		return Type::String;
	}else if(delimID==Delimiter::SOL){
		return Type::Name;
	}else if(delimID==Delimiter::LLT){
		return Type::Dict;
	}else if(delimID==Delimiter::LSB){
		return Type::Array;
	}
	// token
	int position=(int) file.tellg();
	int typeID=-1;
	char buffer[128];
	if(!readToken(buffer, 128)){
	}
	if(strcmp(buffer, "true")==0 || strcmp(buffer, "false")==0){
		typeID=Type::Bool;
	}
	if(strcmp(buffer, "null")==0){
		typeID=Type::Null;
	}

	//
	int v1, v2;
	double v3;
	file.seekg(position, ios_base::beg);
	if(readRefID(&v1, &v2)){
		typeID=Type::Indirect;
	}else{
		file.seekg(position, ios_base::beg);
		if(readInt(&v1)){
			typeID=Type::Int;
		}else{
			file.seekg(position, ios_base::beg);
			if(readReal(&v3)){
				typeID=Type::Real;
			}
		}
	}
		

	file.seekg(position, ios_base::beg);
	return typeID;
}