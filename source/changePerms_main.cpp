// change permissions, executable

#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "PDFParser.hpp"
#include "PDFExporter.hpp"

#ifndef INCLUDE_OBJECTS
#include "Objects.hpp"
#endif

using namespace std;
int main(int argc, char** argv){
	cout << "Read PDF file" << endl;
	if(argc<3){
		printf("Usage: %s input output\n", argv[0]);
		return -1;
	}
	printf("Input file: %s\n", argv[1]);

	PDFParser PP(argv[1]);
	
	if(PP.hasError()){
		cout << "The input file has error(s)" << endl;
		return -1;
	}else{
		cout << "Read succeeded" << endl;
	}

	printf("Export destination: %s\n", argv[2]);

	// prepare encrypt object
	bool alreadyEncrypted=PP.encrypted;
	if(PP.encrypted==false){
		PP.encryptObj_ex=new Encryption();
	}

	// encryption algorithm version
	cout << "Select encrytion version (V=2, 4, or 5)" << endl;

	char V_c[64];
	int V_new;
	while(true){
		cin >> V_c;
		V_new=atoi(V_c);
		if(V_new>=2 && V_new<=5 && V_new!=3){
			break;
		}
		cout << "Input value error; enter value again" << endl;
	}
	printf("Set V to %d\n", V_new);
	PP.encryptObj_ex->setV(V_new);

	// CFM
	cout << "Select CFM (V2, AESV2, AESV3)" << endl;
	char CFM_c[64];
	while(true){
		cin >> CFM_c;
		if(strcmp(CFM_c, "V2")==0 || strcmp(CFM_c, "AESV2")==0 || strcmp(CFM_c, "AESV3")==0){
			break;
		}
		cout << "Input value error; enter value again" << endl;	
	}
	printf("Set CFM to %s\n", CFM_c);
	PP.encryptObj_ex->setCFM((char*)CFM_c);

	// P
	cout << "Enter P, 1: enabled, 0: disabled" << endl;
	// 3, 4, 5, 6, 9, 11, 12
	cout << "Print, Modify, Copy, Annotations, Fill forms, Assemble, Faithful print" << endl;
	bool p[7];
	char p_c[64];
	int i;
	while(true){
		cin >> p_c;
		bool p_error=false;
		for(i=0; i<7; i++){
			if(p_c[i]=='0' || p_c[i]=='1'){
				p[i]=(p_c[i]=='1');
			}else{
				cout << "Format error; enter value again" << endl;
				p_error=true;
				break;
			}
		}
		if(!p_error){
			break;
		}
	}
	PP.encryptObj_ex->setP(p);

	// user and owner passwords
	cout << "User password" << endl;
	uchar* userPwd;
	uchar* userPwd2;
	while(true){
		userPwd=PP.encryptObj_ex->GetPassword();
		cout << "Enter user password again" << endl;
		userPwd2=PP.encryptObj_ex->GetPassword();
		if(unsignedstrcmp(userPwd->data, userPwd2->data)){
			break;
		}
		cout << "Different passwords are input; enter value again" << endl;
	}
	cout << "Owner password" << endl;
	uchar* ownerPwd;
	uchar* ownerPwd2;
	while(true){
		ownerPwd=PP.encryptObj_ex->GetPassword();
		cout << "Enter owner password again" << endl;
		ownerPwd2=PP.encryptObj_ex->GetPassword();
		if(unsignedstrcmp(ownerPwd->data, ownerPwd2->data)){
			break;
		}
		cout << "Different passwords are input; enter value again" << endl;
	}

	printf("User password: %d bytes\n", userPwd->length);
	printf("Owner password: %d bytes\n", ownerPwd->length);
	// prepare ID in trailer if it does not exist
	Array* ID;
	if(PP.trailer.Search((unsigned char*)"ID")>=0){
		// ID exists
		int IDType;
		void* IDValue;
		PP.trailer.Read((unsigned char*)"ID", (void**)&IDValue, &IDType);
		if(IDType==Type::Array){
			ID=(Array*)IDValue;
		}else{
			cout << "Error in read ID" << endl;
			return -1;
		}
	}else{
		// ID does not exist
		uchar* id1=new uchar();
		id1->length=16;
		id1->data=new unsigned char[16];
		srand((unsigned int)time(NULL));
		int i;
		for(i=0; i<16; i++){
			int r=rand();
			id1->data[i]=(unsigned char)r%256;
		}
		cout << "ID prepared: ";
		for(i=0; i<16; i++){
			printf("%02x", id1->data[i]);
		}
		cout << endl;
		ID=new Array();
		ID->Append(id1, Type::String);
		ID->Append(id1, Type::String);
		PP.trailer.Append((unsigned char*)"ID", ID, Type::Array);
	}

	PP.encryptObj_ex->setPwd(ID, userPwd, ownerPwd);
	
	PDFExporter PE(&PP);
	if(PE.exportToFile(argv[2],true)){
		cout << "Export succeeded" << endl;
	}else{
		cout << "Export failed" << endl;
	}
	return 0;
	
}
