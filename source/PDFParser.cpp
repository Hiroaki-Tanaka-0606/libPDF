// class PDFParser

#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <cstdlib>
#include "PDFParser.hpp"

using namespace std;
PDFParser::PDFParser(char* fileName):
	// Member initializer list
	file(fileName),
	error(false),
	encrypted(false),
	lastXRefStm(-1),
	encryptObjNum(-1)
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
		cout << endl;
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
	cout << endl << "Trailer dictionary" << endl;
	trailer.Print();
	cout << endl;

	// prepare xref database
	int SizeType;
	void* SizeValue;
  if(trailer.Read((unsigned char*)"Size", (void**)&SizeValue, &SizeType) && SizeType==Type::Int){
	}else{
		cout << "Error in read size of xref references" << endl;
		error=true;
		return;
	}
	ReferenceSize=*((int*)SizeValue);
	printf("Make XRef database with size %d\n", ReferenceSize);
	Reference=new Indirect*[ReferenceSize];
	int i;
	for(i=0; i<ReferenceSize; i++){
		Reference[i]=new Indirect();
	}
	prevXRef=lastXRef;
	while(true){
		cout << endl;
		prevXRef=readXRef(prevXRef);
		if(prevXRef==0){
			// This is the first Trailer
			break;
		}else if(prevXRef<0){
			error=true;
			return;
		}
	}
	cout << endl;
	
	for(i=0; i<ReferenceSize; i++){
		if(i!=Reference[i]->objNumber){
			printf("Error: missing ref #%d\n", i);
			error=true;
			return;
		}
		printf("Ref #%d: genNumber %d, status %s, ",
					 i, Reference[i]->genNumber,
					 Reference[i]->used ? "used" : "free");
		if(Reference[i]->used){
			if(Reference[i]->objStream){
				printf("index %d of objStream (#%d)\n", Reference[i]->objStreamIndex, Reference[i]->objStreamNumber);
			}else{
				printf("position %d\n", Reference[i]->position);
			}
		}else{
			cout << endl;
		}
		}

	// find /Version in Document catalog dictrionary
	cout << "Find /Version in Document catalog dictionary" << endl;
	int dCatalogType;
	void* dCatalogValue;
	Indirect* dCatalogRef;
	Dictionary* dCatalog;
	if(trailer.Read((unsigned char*)"Root", (void**)&dCatalogValue, &dCatalogType) && dCatalogType==Type::Indirect){
		dCatalogRef=(Indirect*)dCatalogValue;
		if(readRefObj(dCatalogRef, (void**)&dCatalogValue, &dCatalogType) && dCatalogType==Type::Dict){
			dCatalog=(Dictionary*)dCatalogValue;
			int versionType;
			void* versionValue;
			if(dCatalog->Read((unsigned char*)"Version", (void**)&versionValue, &versionType) && versionType==Type::Name){
				unsigned char* version=(unsigned char*)versionValue;
				V_catalog.set((char*)version);
				V_catalog.print();
				printf("Version in document catalog dictionary is %s\n", V_catalog.v);
			}else{
				cout << "No version information in document catalog dictionary" << endl;
			}
		}else{
			cout << "Error in read Document catalog dictionary" << endl;
			error=true;
			return;
		}
	}else{
		cout << "Error in Document catalog dictionary" << endl;
		error=true;
		return;
	}
	cout << endl;

	// find /Encrypt dictionary in Document catalog dictionary
	cout << "Find /Encrypt in Document catalog dictionary" << endl;
	int encryptType;
	void* encryptValue;
	if(trailer.Search((unsigned char*)"Encrypt")>=0){
		cout << "This document is encrypted" << endl;
		int IDType;
		void* IDValue;
		Array* ID;
		trailer.Read((unsigned char*)"ID", (void**)&IDValue, &IDType);
		if(IDType==Type::Array){
			ID=(Array*)IDValue;
		}else{
			cout << "Error in read ID" << endl;
			error=true;
			return;
		}
		trailer.Read((unsigned char*)"Encrypt", (void**)&encryptValue, &encryptType);
		if(encryptType==Type::Indirect){
			Indirect* encryptRef=(Indirect*)encryptValue;
			encryptObjNum=encryptRef->objNumber;
			if(readRefObj(encryptRef, (void**)&encryptValue, &encryptType) && encryptType==Type::Dict){
				encrypt=(Dictionary*)encryptValue;
			}else{
				cout << "Error in read encrypt reference" << endl;
				error=true;
				return;
			}
		}else if(encryptType==Type::Dict){
			encrypt=(Dictionary*)encryptValue;
		}else{
			cout << "Invalid type of /Encrypt" << endl;
			error=true;
			return;
		}
		encryptObj=new Encryption(encrypt, ID);
		encryptObj_ex=new Encryption(encryptObj);
		encrypted=true;
	}else{
		cout << "This document is not encrypted" << endl;
	}
	cout << endl;
	
	cout << "Read page information" << endl;
	// construct Pages array
	Indirect* rootPagesRef;
	Dictionary* rootPages;
	void* rootPagesValue;
	int rootPagesType;
	if(dCatalog->Read((unsigned char*)"Pages", (void**)&rootPagesValue, &rootPagesType) && rootPagesType==Type::Indirect){
		rootPagesRef=(Indirect*)rootPagesValue;
		if(readRefObj(rootPagesRef, (void**)&rootPagesValue, &rootPagesType) && rootPagesType==Type::Dict){
			rootPages=(Dictionary*)rootPagesValue;
			// rootPages->Print();
		}else{
			cout << "Error in read rootPages reference" << endl;
			error=true;
			return;
		}
	}else{
		cout << "Error in Pages" << endl;
		error=true;
		return;
	}
	void* totalPagesValue;
	int totalPages;
	int totalPagesType;
	if(rootPages->Read((unsigned char*)"Count", (void**)&totalPagesValue, &totalPagesType) && totalPagesType==Type::Int){
		totalPages=*((int*)totalPagesValue);
		printf("Total pages: %d\n", totalPages);
	}else{
		cout << "Error in read count in rootPages" << endl;
		error=true;
		return;
	}
	Pages=new Page*[totalPages];
	PagesSize=totalPages;
	int pageCount=0;
	if(!investigatePages(rootPagesRef, &pageCount)){
		error=true;
		return;
	}
}

bool PDFParser::investigatePages(Indirect* pagesRef, int* pageCount){
	void* pagesValue;
	int pagesType;
	Dictionary* pages;
	if(readRefObj(pagesRef, (void**)&pagesValue, &pagesType) && pagesType==Type::Dict){
		pages=(Dictionary*)pagesValue;
	}else{
		cout << "Error in read pages reference" << endl;
		return false;
	}
	
	void* kidsValue;
	int kidsType;
	Array* kids;
	if(pages->Read((unsigned char*)"Kids", (void**)&kidsValue, &kidsType) && kidsType==Type::Array){
		kids=(Array*)kidsValue;
		int kidsSize=kids->getSize();
		void* kidValue;
		int kidType;
		Indirect* kidRef;
		Dictionary* kid;
		int i;
		for(i=0; i<kidsSize; i++){
			// printf("KID %d\n", i);
			if(kids->Read(i, (void**)&kidValue, &kidType) && kidType==Type::Indirect){
				kidRef=(Indirect*)kidValue;
				if(readRefObj(kidRef, (void**)&kidValue, &kidType) && kidType==Type::Dict){
					kid=(Dictionary*)kidValue;
					// kid->Print();
					void* kidTypeValue;
					unsigned char* kidType;
					int kidTypeType;
					if(kid->Read((unsigned char*)"Type", (void**)&kidTypeValue, &kidTypeType) && kidTypeType==Type::Name){
						kidType=(unsigned char*)kidTypeValue;
						if(unsignedstrcmp(kidType, (unsigned char*)"Page")){
							// cout << "Page" << endl;
							Pages[*pageCount]=new Page();
							Pages[*pageCount]->Parent=pagesRef;
							Pages[*pageCount]->PageDict=kid;
							Pages[*pageCount]->objNumber=kidRef->objNumber;
							*pageCount=*pageCount+1;
						}else if(unsignedstrcmp(kidType, (unsigned char*)"Pages")){
							// cout << "Pages" << endl;
							if(!investigatePages(kidRef, pageCount)){
								return false;
							}
						}
					}
				}else{
					cout << "Error in read kid" << endl;
					return false;
				}
			}else{
				cout << "Error in read kid reference" << endl;
				return false;
			}
		}
	}else{
		cout << "Error in read Kids" << endl;
		return false;
	}

	
	return true;
}


bool PDFParser::readPage(int index, unsigned char* key, void** value, int* type, bool inheritable){
	Page* page=Pages[index];
	if(page->PageDict->Search(key)<0){
		// key not found
		if(inheritable){
			Indirect* parentRef=page->Parent;
			void* parentValue;
			int parentType;
			Dictionary* parent;
			while(true){
				if(readRefObj(parentRef, (void**)&parentValue, &parentType) && parentType==Type::Dict){
					parent=(Dictionary*)parentValue;
					// parent->Print();
					if(parent->Search(key)>=0){
						parent->Read(key, value, type);
						return true;
					}
					if(parent->Read((unsigned char*)"Parent", (void**)&parentValue, &parentType) && parentType==Type::Indirect){
						parentRef=(Indirect*)parentValue;
					}else{
						// cout << "Parent not found" << endl;
						return false;
					}
				}
			}
		}else{
			return false;
		}
	}else{
		page->PageDict->Read(key, value, type);
		return true;
	}
}


bool PDFParser::readRefObj(Indirect* ref, void** object, int* objType){
	int i;
	int objNumber=ref->objNumber;
	int genNumber=ref->genNumber;
	printf("Read indirect reference: objNumber %d, genNumber %d\n", objNumber, genNumber);
	Indirect* refInRef=Reference[objNumber];
	if(refInRef->objNumber!=objNumber || refInRef->genNumber!=genNumber){
		printf("Error: obj and gen numbers in XRef (%d, %d) is different from arguments (%d, %d)\n", refInRef->objNumber, refInRef->genNumber, objNumber, genNumber);
		return false;
	}
	if(refInRef->objStream){
		cout << "The object is in an object stream" << endl;
		
		Stream* objStream;
		Indirect objStreamReference;
		objStreamReference.objNumber=refInRef->objStreamNumber;
		objStreamReference.genNumber=0;
		void* objStreamValue;
		int objStreamType;
		if(readRefObj(&objStreamReference, (void**)&objStreamValue, &objStreamType) && objStreamType==Type::Stream){
			objStream=(Stream*)objStreamValue;
		}else{
			cout << "Error in read an object stream" << endl;
			return false;
		}
		if(encrypted){
			cout << "Decrypt stream" << endl;
			if(!encryptObj->DecryptStream(objStream)){
				cout << "Error in decrypting stream" << endl;
				return false;
			}
		}
		objStream->Decode();

		// get the offset
		int First;
		int FirstType;
		void* FirstValue;
		int N;
		int NType;
		void* NValue;
		
		int index=refInRef->objStreamIndex;
		if(objStream->StmDict.Read((unsigned char*)"First", (void**)&FirstValue, &FirstType) && FirstType==Type::Int){
			First=*((int*)FirstValue);
		}else{
			cout << "Error in read First" << endl;
			return false;
		}
		if(objStream->StmDict.Read((unsigned char*)"N", (void**)&NValue, &NType) && NType==Type::Int){
			N=*((int*)NValue);
		}else{
			cout << "Error in read N" << endl;
			return false;
		}
		string data((char*)objStream->decoded, objStream->dlength);
		istringstream is(data);
		int positionInObjStream;
		for(i=0; i<N; i++){
			int v1, v2;
			readInt(&v1, &is);
			readInt(&v2, &is);
			// printf("%d %d\n", v1, v2);
			if(i==index){
				if(v1==objNumber){
					positionInObjStream=v2+First;
					printf("object #%d is at %d\n", v1, positionInObjStream);
					break;
				}else{
					printf("Error: objNumber mismatch %d %d\n", v1, objNumber);
					return false;
				}
			}
		}
		// cout << objStream->decoded << endl;
		is.seekg(positionInObjStream, ios_base::beg);
		// cout << "Judge" << endl;
		*objType=judgeType(&is);
		printf("ObjType: %d\n", *objType);
		switch(*objType){
		case Type::Bool:
			*object=new bool();
			if(!readBool((bool*)*object, &is)){
				cout << "Error in read bool" << endl;
				return false;
			}
			break;
		case Type::Int:
			*object=new int();
			if(!readInt((int*)*object, &is)){
				cout << "Error in read int" << endl;
				return false;
			}
			break;
		case Type::Real:
			*object=new double();
			if(!readReal((double*)*object, &is)){
				cout << "Error in read real" << endl;
				return false;
			}
			break;
		case Type::String:
			*object=readString(&is);
			if(*object==NULL){
				cout << "Error in read string" << endl;
				return false;
			}
			break;
		case Type::Name:
			*object=readName(&is);
			if(*object==NULL){
				cout << "Error in read name" << endl;
				return false;
			}
			break;
		case Type::Array:
			*object=new Array();
			if(!readArray((Array*)*object, &is)){
				cout << "Error in read array" << endl;
				return false;
			}
			break;
		case Type::Dict:
			*object=new Dictionary();
			if(!readDict((Dictionary*)*object, &is)){
				cout << "Error in read dict" << endl;
				return false;
			}
			// ((Dictionary*)*object)->Print();
			break;
		default:
			cout << "Error: invalid type" << endl;
		}
		cout << "ReadRefObj from object stream finished" << endl;
	}else{
		printf("The object is at %d\n", refInRef->position);
		if(refInRef->position<0){
			cout << "Value in the Indirect object" << endl;
			*objType=refInRef->type;
			*object=refInRef->value;
			return true;
		}
		file.seekg(refInRef->position+offset, ios_base::beg);
		int objNumber2;
		int genNumber2;
		if(!readObjID(&objNumber2, &genNumber2)){
			cout << "Error in reading object ID" << endl;
			return false;
		}
		if(objNumber2!=objNumber || genNumber2!=genNumber){
			cout << "Error: object or generation numbers are different" << endl;
			return false;
		}
		int type=judgeType();
		bool* boolValue;
		int* intValue;
		double* realValue;
		uchar* stringValue;
		unsigned char* nameValue;
		Array* arrayValue;
		Stream* streamValue;
		Dictionary* dictValue;
		int currentPos;
		switch(type){
		case Type::Bool:
			boolValue=new bool();
			if(readBool(boolValue)){
				*objType=Type::Bool;
				*object=(void*)boolValue;
			}else{
				cout << "Error in read bool" << endl;
				return false;
			}
			break;
		case Type::Int:
			intValue=new int();
			if(readInt(intValue)){
				*objType=Type::Int;
				*object=(void*)intValue;
			}else{
				cout << "Error in read int" << endl;
				return false;
			}
			break;
		case Type::Real:
			realValue=new double();
			if(readReal(realValue)){
				*objType=Type::Real;
				*object=(void*)realValue;
			}else{
				cout << "Error in read real" << endl;
				return false;
			}
			break;
		case Type::String:
			stringValue=readString();
			if(stringValue!=NULL){
				*objType=Type::String;
				*object=(void*)stringValue;
			}else{
				cout << "Error in read string" << endl;
				return false;
			}
			break;
		case Type::Name:
			nameValue=readName();
			if(nameValue!=NULL){
				*objType=Type::Name;
				*object=(void*)nameValue;
			}else{
				cout << "Error in read name" << endl;
				return false;
			}
			break;
		case Type::Array:
			arrayValue=new Array();
			if(readArray(arrayValue)){
				*objType=Type::Array;
				*object=(void*)arrayValue;
			}else{
				cout << "Error in read array" << endl;
				return false;
			}
			break;
		case Type::Null:
			*objType=Type::Null;
			*object=NULL;
			break;
		case Type::Dict:
			// it can be a Dictionary and a Stream
			currentPos=(int)file.tellg();
			file.seekg(refInRef->position+offset, ios_base::beg);
			streamValue=new Stream();
			if(readStream(streamValue, false)){
				*objType=Type::Stream;
				*object=(void*)streamValue;
			}else{
				file.seekg(currentPos, ios_base::beg);
				dictValue=new Dictionary();
				if(readDict(dictValue)){
					*objType=Type::Dict;
					*object=(void*)dictValue;
				}else{
					cout << "Error in read dictionary or stream" << endl;
					return false;
				}
			}
			break;
		default:
			cout << "Error: invalid type" << endl;
			return false;
		}
		// encryption check
		if(encrypted && objNumber!=encryptObjNum){
			// decryption of strings
			void* elementValue;
			int elementType;
			unsigned char* elementKey;
			switch(*objType){
			case Type::String:
				if(encryptObj->DecryptString((uchar*)(*object), objNumber, genNumber)){
					cout << "Decryption of string finished" << endl;
				}
				break;
			case Type::Array:
				for(i=0; i<((Array*)(*object))->getSize(); i++){
					if(((Array*)(*object))->Read(i, &elementValue, &elementType)){
						if(elementType==Type::String){
							if(encryptObj->DecryptString((uchar*)elementValue, objNumber, genNumber)){
								cout << "Decryption of string in array" << endl;
							}else{
								cout << "Error in decryption" << endl;
								return false;
							}
						}
					}else{
						cout << "Error in reading an array" << endl;
					}
				}
				break;
			case Type::Dict:
				for(i=0; i<((Dictionary*)(*object))->getSize(); i++){
					if(((Dictionary*)(*object))->Read(i, &elementKey, &elementValue, &elementType)){
						if(elementType==Type::String){
							if(encryptObj->DecryptString((uchar*)elementValue, objNumber, genNumber)){
								cout << "Decryption of string in dict" << endl;
							}else{
								cout << "Error in decryption" << endl;
								return false;
							}
						}
					}else{
						cout << "Error in reading a dict" << endl;
					}
				}
				break;
			case Type::Stream:
				if(!encryptObj->DecryptStream((Stream*)*object)){
					cout << "Error in decrypting stream" << endl;
					return false;
				}
				cout << "Decryption of stream finished" << endl;
			}
		}
		if(*objType==Type::Stream){
			((Stream*)(*object))->Decode();
		}
		cout << "ReadRefObj finished" << endl;
	}

	return true;
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

bool PDFParser::isOctal(char a){
	return a=='0' || a=='1' || a=='2' || a=='3' || a=='4' || a=='5' || a=='6' || a=='7';
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
	// check whether the pointer is at EOF
	int currentPos=(int) file.tellg();
	file.seekg(0, ios_base::end);
	int endPos=(int)file.tellg();
	file.seekg(currentPos, ios_base::beg);
	if(currentPos==endPos){
		// already at EOF
		cout << "EOF1" << endl;
		return true;
	}
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
		}else{
			currentPos==(int)file.tellg();
			if(currentPos==endPos){
				cout << "EOF2" << endl;
				return true;
			}
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
	cout << "findFooter finish" << endl;
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
			return;
		}else{
			file.seekg(-1, ios_base::cur);
			// cout << a << endl;
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
	file.seekg(position+offset, ios_base::beg);
	
	// XRef stream: begins with "%d %d obj" line
	int objNumber;
	int genNumber;
	bool XRefStream=readObjID(&objNumber, &genNumber);
	file.seekg(position+offset, ios_base::beg);

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
		cout << "Skip ObjID row" << endl;
		readLine(buffer, 32);
	}

	// read Trailer dictionary
	Dictionary trailer2;
	if(!readDict(&trailer2)){
		return -1;
	}
	// merge trailer2 to trailer
	trailer.Merge(trailer2);
	
	// check Prev key exists
	int PrevType;
	void* PrevValue;
	if(trailer2.Read((unsigned char*)"Prev", (void**)&PrevValue, &PrevType) && PrevType==Type::Int){
		return *((int*)PrevValue);
	}
	return 0;
}


int PDFParser::readXRef(int position){
	printf("Read XRef at %d\n", position);
	file.seekg(position+offset, ios_base::beg);

  // XRef table: begins with "xref" line
	bool XRefTable=true;
	int i, j, k;
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
	file.seekg(position+offset, ios_base::beg);
	
	// XRef stream: begins with "%d %d obj" line
	int objNumber;
	int genNumber;
	bool XRefStream=readObjID(&objNumber, &genNumber);
	file.seekg(position+offset, ios_base::beg);

	if(!XRefTable && !XRefStream){
		cout << "Error: none of XRef Table or XRef Stream" << endl;
		return -1;
	}

	char buffer[32];
	Dictionary trailer2;
	if(XRefTable){
		cout << "Read XRef table" << endl;
		// skip "xref" row
		readLine(buffer, 32);
		// cout << buffer << endl;
		int objNumber;
		int numEntries;
		char position_c[11];
		char genNumber_c[6];
		char used_c[2];
		char* strtolPointer;
		position_c[10]='\0';
		genNumber_c[6]='\0';
		used_c[1]='\0';
		while(true){
			// check whether the line is "trailer" (end of xref table)
			int currentPosition=(int) file.tellg();
			bool flag=true;
			for(i=0; i<strlen(TRAIL); i++){
				file.get(a);
				if(a!=TRAIL[i]){
					flag=false;
					break;
				}
			}
			if(!flag){
				file.seekg(currentPosition, ios_base::beg);
			}else{
				if(!gotoEOL(1)){
					cout << "Error in reading 'trailer' row" << endl;
					return -1;
				}else{
					break;
				}
			}
			bool a1=readInt(&objNumber);
			bool a2=readInt(&numEntries);
			bool a3=gotoEOL(1);
			// cout << a1 << a2 << a3 << endl;
			if(a1 && a2 && a3){
				for(i=0; i<numEntries; i++){
					// xref table content: (offset 10 bytes) (genNumber 5 bytes) (n/f)(EOL 2 bytes)
					int currentObjNumber=objNumber+i;
					if(currentObjNumber>=ReferenceSize){
						break;
					}
				  file.read(position_c, 10);
					file.get(a);
					file.read(genNumber_c, 5);
					file.get(a);
					file.read(used_c, 1);
					file.get(a);
					file.get(a);
					// cout << offset_c << " | " << genNumber_c << endl;
					// check a is CR of LF
					if(!isEOL(a)){
						return -1;
					}
					if(Reference[currentObjNumber]->objNumber>=0){
						// reference is already read
						printf("XRef #%d is already read\n", currentObjNumber);
						continue;
					}
					Reference[currentObjNumber]->objNumber=currentObjNumber;
					// convert string to integer
					Reference[currentObjNumber]->position=strtol(position_c, &strtolPointer, 10);
					if(strtolPointer!=&position_c[10]){
						cout << "Error in parsing position" << endl;
						return -1;
					}
					Reference[currentObjNumber]->genNumber=strtol(genNumber_c, &strtolPointer, 10);
					if(strtolPointer!=&genNumber_c[5]){
						cout << "Error in parsing genNumber" << endl;
						return -1;
					}
					// convert used (n/f)
					if(strcmp(used_c, "n")==0){
						Reference[currentObjNumber]->used=true;
					}else if(strcmp(used_c, "f")==0){
						Reference[currentObjNumber]->used=false;
					}else{
						cout << "Invalid n/f value" << endl;
						return -1;
					}					
				}
			}else{
				cout << "Error in reading xref table header" << endl;
				return -1;
			}
		}
		// read Trailer dictionary
		if(!readDict(&trailer2)){
			return -1;
		}
		// if XRefStm entry exists in the Trailer dictionary, read it
		int XRefStmType;
		void* XRefStmValue;
		if(trailer2.Read((unsigned char*)"XRefStm", (void**)&XRefStmValue, &XRefStmType) && XRefStmType==Type::Int){
			int XRefStmPosition=*((int*)XRefStmValue);
			cout << "Read hybrid XRef stream" << endl;
			if(readXRef(XRefStmPosition)<0){
				return -1;
			}
		}
		// check Prev key exists
		int PrevType;
		void* PrevValue;
		if(trailer2.Read((unsigned char*)"Prev", (void**)&PrevValue, &PrevType) && PrevType==Type::Int){
			return *((int*)PrevValue);
		}
	}else{
		// read xref stream
		Stream XRefStm;
		if(!readStream(&XRefStm)){
			return -1;
		}
		printf("XRef stream: objNumber %d, genNumber %d\n",
					 XRefStm.objNumber,
					 XRefStm.genNumber);
		/*
			cout << "XRef data:" << endl;
			for(i=0; i<XRefStm.length; i++){
			printf("%02x ", (unsigned int)XRefStm.data[i]);
			}
			cout << endl;*/

		if(lastXRefStm<0){
			// this is the last XRef stream
			lastXRefStm=XRefStm.objNumber;
			cout << "This is the last XRef stream" << endl;
		}
		
		// decode xref stream
		if(!XRefStm.Decode()){
			return -1;
		}

		// read W
		int WType;
		void* WValue;
		int field[3];
		int sumField=0;
		int fieldType;
		if(XRefStm.StmDict.Read((unsigned char*)"W", (void**)&WValue, &WType) && WType==Type::Array){
			Array* WArray=(Array*)WValue;
			void* fieldValue;
			for(i=0; i<3; i++){
				if(WArray->Read(i, (void**)&fieldValue, &fieldType)){
					if(fieldType==Type::Int){
						field[i]=*((int*)fieldValue);
						sumField+=field[i];
					}else{
						cout << "Error: invalid type of W element" << endl;
						return 0;
					}
				}else{
					cout << "Error in reading W array" << endl;
					return 0;
				}
			}
		}else{
			cout << "Error in reading W" << endl;
			return 0;
		}

		// read Index
		int numSubsections=1;
		if(XRefStm.StmDict.Search((unsigned char*)"Index")>=0){
			int IndexType;
			void* IndexValue;
			if(XRefStm.StmDict.Read((unsigned char*)"Index", (void**)&IndexValue, &IndexType) && IndexType==Type::Array){
				Array* IndexArray=(Array*)IndexValue;
				int IndexArraySize=IndexArray->getSize();
				if(IndexArraySize%2!=0){
					cout << "Error: Index Array contains odd number of elements" << endl;
					return 0;
				}
				numSubsections=IndexArraySize/2;
			}
		}
				
		int firstIndex[numSubsections];
		int numEntries[numSubsections];
		if(XRefStm.StmDict.Search((unsigned char*)"Index")>=0){
			int IndexType;
			void* IndexValue;
			if(XRefStm.StmDict.Read((unsigned char*)"Index", (void**)&IndexValue, &IndexType) && IndexType==Type::Array){
				Array* IndexArray=(Array*)IndexValue;
				void* IndexElementValue;
				int IndexElementType;
				for(i=0; i<numSubsections; i++){
					if(IndexArray->Read(2*i, (void**)&IndexElementValue, &IndexElementType)){
						if(IndexElementType==Type::Int){
							firstIndex[i]=*((int*)IndexElementValue);
						}else{
							cout << "Error: invalid type of Index element" << endl;
							return 0;
						}
					}else{
						cout << "Error in reading Index element" << endl;
						return 0;
					}
					if(IndexArray->Read(2*i+1, (void**)&IndexElementValue, &IndexElementType)){
						if(IndexElementType==Type::Int){
							numEntries[i]=*((int*)IndexElementValue);
						}else{
							cout << "Error: invalid type of Index element" << endl;
							return 0;
						}
					}else{
						cout << "Error in reading Index element" << endl;
						return 0;
					}
				}
			}else{
				cout << "Error in reading Index array" << endl;
				return 0;
			}
		}else{
			// if Index does not exist, the default value is [0 Size]
			int SizeType;
			void* SizeValue;
			firstIndex[0]=0;
			if(XRefStm.StmDict.Read((unsigned char*)"Size", (void**)&SizeValue, &SizeType) && SizeType==Type::Int){
				numEntries[0]=*((int*)SizeValue);
			}else{
				cout << "Error in reading Size" << endl;
				return 0;
			}
		}
		
		int totalEntries=0;
		for(i=0; i<numSubsections; i++){
			totalEntries+=numEntries[i];
		}
		printf("Decoded XRef: %d bytes, %d entries\n", XRefStm.dlength, totalEntries);
			for(i=0; i<XRefStm.dlength; i++){
			printf("%02x ", (unsigned int)XRefStm.decoded[i]);
			if((i+1)%sumField==0){
			cout << endl;
			}
			}

		// record the information to Reference
		if(XRefStm.dlength/sumField!=totalEntries){
			cout << "Error: size mismatch in decoded data" << endl;
			return 0;
		}
		int XRefStmIndex=0;
		int XRefStmValue[3];
		int i2;
		for(i2=0; i2<numSubsections; i2++){
			for(i=0; i<numEntries[i2]; i++){
				for(j=0; j<3; j++){
					XRefStmValue[j]=0;
					for(k=0; k<field[j]; k++){
						XRefStmValue[j]=256*XRefStmValue[j]+(unsigned int)XRefStm.decoded[XRefStmIndex];
						XRefStmIndex++;
					}
				}
				// printf("%d %d %d\n", XRefStmValue[0], XRefStmValue[1], XRefStmValue[2]);
				int currentObjNumber=i+firstIndex[i2];
				if(Reference[currentObjNumber]->objNumber>=0){
					printf("XRef #%d is already read\n", currentObjNumber);
					continue;
				}
				switch(XRefStmValue[0]){
				case 0:
					// free object
					Reference[currentObjNumber]->used=false;
					Reference[currentObjNumber]->objNumber=currentObjNumber;
					Reference[currentObjNumber]->position=XRefStmValue[1];
					Reference[currentObjNumber]->genNumber=XRefStmValue[2];
					break;
				case 1:
					// used object
					Reference[currentObjNumber]->used=true;
					Reference[currentObjNumber]->objNumber=currentObjNumber;
					Reference[currentObjNumber]->position=XRefStmValue[1];
					Reference[currentObjNumber]->genNumber=XRefStmValue[2];
					break;
				case 2:
					// object in compressed object
					Reference[currentObjNumber]->used=true;
					Reference[currentObjNumber]->objStream=true;
					Reference[currentObjNumber]->objNumber=currentObjNumber;
					Reference[currentObjNumber]->objStreamNumber=XRefStmValue[1];
					Reference[currentObjNumber]->objStreamIndex=XRefStmValue[2];
				}
			}
		}

		// check Prev key exists
		int PrevType;
		void* PrevValue;
		if(XRefStm.StmDict.Read((unsigned char*)"Prev", (void**)&PrevValue, &PrevType) && PrevType==Type::Int){
			return *((int*)PrevValue);
		}
	}
	return 0;
}


bool PDFParser::readObjID(int* objNumber, int* genNumber){
	// Indirect object identifier row: %d %d obj
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
	return readRefID(objNumber, genNumber, &file);
}
bool PDFParser::readRefID(int* objNumber, int* genNumber, istream* is){
	// Indirect object reference row: %d %d R
	readInt(objNumber, is);
	readInt(genNumber, is);
	char buffer[32];
	if(!readToken(buffer, 32, is)){
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
	return readInt(value, &file);
}
bool PDFParser::readInt(int* value, istream* is){
	char buffer[32];
	if(!readToken(buffer, 32, is)){
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

bool PDFParser::readBool(bool* value){
	return readBool(value, &file);
}
bool PDFParser::readBool(bool* value, istream* is){
	char buffer[32];
	if(!readToken(buffer, 32, is)){
		return false;
	}
	if(strcmp(buffer, "true")==0){
		*value=true;
	}else if(strcmp(buffer, "false")==0){
		*value=false;
	}else{
		return false;
	}
	return true;
}

bool PDFParser::readReal(double* value){
	return readReal(value, &file);
}
bool PDFParser::readReal(double* value, istream* is){
	char buffer[32];
	if(!readToken(buffer, 32, is)){
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
	return readToken(buffer, n, &file);
}
bool PDFParser::readToken(char* buffer, int n, istream* is){
	if(!skipSpaces(is)){
		return false;
	}
	char a;
	int i=0;
	while(true){
		is->get(a);
		if(isSpace(a) || isDelimiter(a)){
			buffer[i]='\0';
			is->seekg(-1, ios_base::cur);
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
	return skipSpaces(&file);
}		
bool PDFParser::skipSpaces(istream* is){
	char a;
	int pos=(int) is->tellg();
	// cout << pos << endl;
	while(true){
		is->get(a);
		// cout << a << endl;
		if(isSpace(a)){
			continue;
		}else if(a==EOF){
			return false;
		}else{
			break;
		}
	}
	is->seekg(-1, ios_base::cur);
	return true;
}

bool PDFParser::readDict(Dictionary* dict){
	return readDict(dict, &file);
}
bool PDFParser::readDict(Dictionary* dict, istream* is){
	// begins with "<<"
	if(!skipSpaces(is)){
		return false;
	}
	if(judgeDelimiter(true, is)!=Delimiter::LLT){
		return false;
	}
	
	// key (name), value (anything)
	while(true){
		int delim=judgeDelimiter(false, is);
		if(delim==Delimiter::SOL){
			// /name
		  unsigned char* name=readName(is);
			if(name==NULL){
				return false;
			}
			// value
			int type=judgeType(is);
			// printf("Name: %s, Type: %d\n", name, type);
			char buffer[128];
			uchar* stringValue;
			int* intValue;
			bool* boolValue;
			double* realValue;
			unsigned char* nameValue;
			int* objNumValue;
			int* genNumValue;
			Indirect* indirectValue;
			Dictionary* dictValue;
			Array* arrayValue;
			switch(type){
			case Type::Bool:
				boolValue=new bool();
				if(!readBool(boolValue, is)){
					cout << "Error in read bool" << endl;
					return false;
				}
				dict->Append(name, boolValue, type);
				break;
			case Type::Int:
				intValue=new int();
				if(!readInt(intValue, is)){
					cout << "Error in read integer" << endl;
					return false;
				}
				dict->Append(name, intValue, type);
				break;
			case Type::Real:
				realValue=new double();
				if(!readReal(realValue, is)){
					cout << "Error in read real number" << endl;
					return false;
				}
				dict->Append(name, realValue, type);
				break;
			case Type::Null:
				readToken(buffer, 128, is);
				break;
			case Type::String:
			  stringValue=readString(is);
				if(stringValue==NULL){
					cout << "Error in read string" << endl;
					return false;
				}
				dict->Append(name, stringValue, type);
				break;
			case Type::Name:
			  nameValue=readName(is);
				if(nameValue==NULL){
					cout << "Error in read name" << endl;
					return false;
				}
				dict->Append(name, nameValue, type);
				break;
			case Type::Indirect:
				indirectValue=new Indirect();
				if(!readRefID(&(indirectValue->objNumber), &(indirectValue->genNumber), is)){
					cout << "Error in read indirect object" << endl;
					return false;
				}
				dict->Append(name, indirectValue, type);
				break;
			case Type::Dict:
				dictValue=new Dictionary();
				if(!readDict(dictValue, is)){
					cout << "Error in read dictionary" << endl;
					return false;
				}
				dict->Append(name, dictValue, type);
				break;
			case Type::Array:
				arrayValue=new Array();
				if(!readArray(arrayValue, is)){
					cout << "Error in read array" << endl;
					return false;
				}
				dict->Append(name, arrayValue, type);
				break;
			}
			//dict->Print();
			//break;
		}else if(delim==Delimiter::GGT){
			// >> (end of dictionary)
			judgeDelimiter(true, is);
			break;
		}else{
			cout << "SOMETHING STRANGE" << endl;
			return NULL;
		}
	}
	return true;
}

bool PDFParser::readArray(Array* array){
	return readArray(array, &file);
}
bool PDFParser::readArray(Array* array, istream* is){
	// begins with "["
	if(!skipSpaces(is)){
		return false;
	}
	if(judgeDelimiter(true, is)!=Delimiter::LSB){
		return false;
	}
	
	// value (anything)
	while(true){
		int delim=judgeDelimiter(false, is);
		if(delim!=Delimiter::RSB){
			int type=judgeType(is);
			// printf("Type: %d\n", type);
			char buffer[128];
			uchar* stringValue;
			int* intValue;
			bool* boolValue;
			double* realValue;
			unsigned char* nameValue;
			int* objNumValue;
			int* genNumValue;
			Indirect* indirectValue;
			Dictionary* dictValue;
			Array* arrayValue;
			switch(type){
			case Type::Bool:
				boolValue=new bool();
				if(!readBool(boolValue, is)){
					cout << "Error in read bool" << endl;
					return false;
				}
				array->Append(boolValue, type);
				break;
			case Type::Int:
				intValue=new int();
				if(!readInt(intValue, is)){
					cout << "Error in read integer" << endl;
					return false;
				}
				array->Append(intValue, type);
				break;
			case Type::Real:
				realValue=new double();
				if(!readReal(realValue, is)){
					cout << "Error in read real number" << endl;
					return false;
				}
				array->Append(realValue, type);
				break;
			case Type::Null:
				readToken(buffer, 128, is);
				array->Append(NULL, type);
				break;
			case Type::String:
			  stringValue=readString(is);
				if(stringValue==NULL){
					cout << "Error in read string" << endl;
					return false;
				}
				array->Append(stringValue, type);
				break;
			case Type::Name:
			  nameValue=readName(is);
				if(nameValue==NULL){
					cout << "Error in read name" << endl;
					return false;
				}
				array->Append(nameValue, type);
				break;
			case Type::Indirect:
				indirectValue=new Indirect();
				if(!readRefID(&(indirectValue->objNumber), &(indirectValue->genNumber), is)){
					cout << "Error in read indirect object" << endl;
					return false;
				}
				array->Append(indirectValue, type);
				break;
			case Type::Dict:
				dictValue=new Dictionary();
				if(!readDict(dictValue, is)){
					cout << "Error in read dictionary" << endl;
					return false;
				}
				array->Append(dictValue, type);
				break;
			case Type::Array:
				arrayValue=new Array();
				if(!readArray(arrayValue, is)){
					cout << "Error in read array" << endl;
					return false;
				}
				array->Append(arrayValue, type);
				break;
			}
			
		}else{
			// ] (end of array)
			judgeDelimiter(true, is);
			break;
		}
	}
	return true;
}

int PDFParser::judgeDelimiter(bool skip){
	return judgeDelimiter(skip, &file);
}
int PDFParser::judgeDelimiter(bool skip, istream* is){
	// cout << "Delim" << endl;
	int pos=(int) is->tellg();
	// cout << pos << endl;
	int delimID=-1;
	if(!skipSpaces(is)){
		cout << "SS" << endl;
		return -1;
	}
	char a1, a2;
	is->get(a1);
	// cout << a1 << endl;
	if(a1=='('){
		delimID=Delimiter::LP;
	}else if(a1==')'){
		delimID=Delimiter::RP;
	}else if(a1=='<'){
		is->get(a2);
		if(a2=='<'){
			delimID=Delimiter::LLT;
		}else{
			is->seekg(-1, ios_base::cur);
			delimID=Delimiter::LT;
		}
	}else if(a1=='>'){
		is->get(a2);
		if(a2=='>'){
			delimID=Delimiter::GGT;
		}else{
			is->seekg(-1, ios_base::cur);
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
		is->seekg(pos, ios_base::beg);
	}
	return delimID;
}

unsigned char* PDFParser::readName(){
	return readName(&file);
}
unsigned char* PDFParser::readName(istream* is){
	if(!skipSpaces(is)){
		return NULL;
	}
	char a;
	is->get(a);
	if(a!='/'){
		return NULL;
	}
	string value1=string("");
	while(true){
		is->get(a);
		if(isSpace(a) || isDelimiter(a)){
			is->seekg(-1, ios_base::cur);
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

uchar* PDFParser::readString(){
	return readString(&file);
}
uchar* PDFParser::readString(istream* is){
	if(!skipSpaces(is)){
		return NULL;
	}
	char a;
	is->get(a);
	if(a!='<' && a!='('){
		return NULL;
	}
	char startDelim=a;
	bool isLiteral=(startDelim=='(');
	char endDelim= isLiteral ? ')' : '>';
	string value1=string("");
	int depth=1;
	while(true){
		is->get(a);
		if(a==endDelim){
			depth--;
			if(depth==0){
				break;
			}
		}else{
			if(a==startDelim){
				depth++;
			}else if(a=='\\'){
				value1+=a;
				is->get(a);
			}
			value1+=a;
		}
	}
	if(isLiteral){
		uchar* literal=new uchar();
	  literal->data=new unsigned char[value1.length()+1];
		int i=0;
		int j=0;
		for(i=0; i<value1.length(); i++){
			if(value1[i]=='\\'){
				// escape
				if(value1[i+1]=='n'){
					literal->data[j]='\n';
					j++;
					i++;
				}else if(value1[i+1]=='r'){
					literal->data[j]='\r';
					j++;
					i++;
				}else if(value1[i+1]=='t'){
					literal->data[j]='\t';
					j++;
					i++;
				}else if(value1[i+1]=='b'){
					literal->data[j]='\b';
					j++;
					i++;
				}else if(value1[i+1]=='f'){
					literal->data[j]='\f';
					j++;
					i++;
				}else if(value1[i+1]=='('){
					literal->data[j]='(';
					j++;
					i++;
				}else if(value1[i+1]==')'){
					literal->data[j]=')';
					j++;
					i++;
				}else if(value1[i+1]=='\\'){
					literal->data[j]='\\';
					j++;
					i++;
				}else if(value1[i+1]=='\r' || value1[i+1]=='\n'){
					if(value1[i+1]=='\r' && value1[i+2]=='\n'){
						i+=2;
					}else{
						i++;
					}
				}else if(isOctal(value1[i+1])){
					int length;
					if(isOctal(value1[i+2])){
						if(isOctal(value1[i+3])){
							length=3;
						}else{
							length=2;
						}
					}else{
						length=1;
					}
					char code[length+1];
					int l;
					for(l=0; l<length; l++){
						code[l]=value1[i+1+l];
					}
					code[length]='\0';
					literal->data[j]=(unsigned char)strtol(code, NULL, 8);
					i+=length;
					j++;
				}
			}else{
				literal->data[j]=(unsigned char)value1[i];
				j++;
			}
		}
		literal->data[j]='\0';
		literal->length=j;
		return literal;
	}else{
		if(value1.length()%2==1){
			value1+='0';
		}
		uchar* hexadecimal=new uchar();
		hexadecimal->data=new unsigned char[value1.length()/2+1];
		int i;
		char hex[3];
		hex[2]='\0';
		for(i=0; i<value1.length()/2; i++){
			hex[0]=value1[i*2];
			hex[1]=value1[i*2+1];
			hexadecimal->data[i]=(unsigned char)strtol(hex, NULL, 16);
		}
		hexadecimal->data[value1.length()/2]=='\0';
		hexadecimal->length=value1.length()/2;
		return hexadecimal;
	}
}

bool PDFParser::readStream(Stream* stm){
	return readStream(stm, true);
}
bool PDFParser::readStream(Stream* stm, bool outputError){
	// read obj id
	if(!readObjID(&(stm->objNumber), &(stm->genNumber))){
		return false;
	}
	// read Stream dictionary
	if(!readDict(&(stm->StmDict))){
		return false;
	}
	// 'stream' row
	skipSpaces();
	int i;
	bool flag=false;
	char a;
	for(i=0; i<strlen(STM); i++){
		file.get(a);
		if(a!=STM[i]){
			flag=true;
			break;
		}
	}
	if(flag){
		if(outputError){
			cout << "Error in reading 'stream'" << endl;
			cout << a << endl;
		}
		return false;
	}
	// here, EOL should be CRLF or LF, not including CR
	file.get(a);
	if(!isEOL(a)){
		if(outputError){
			cout << "Error in 'stream' row" << endl;
		}
		return false;
	}
	if(a==CR){
		file.get(a);
		if(a!=LF){
			if(outputError){
				cout << "Invalid EOL in 'stream' row" << endl;
			}
			return false;
		}
	}
	void* lenValue;
	int lenType;
	if(stm->StmDict.Read((unsigned char*)"Length", (void**)&lenValue, &lenType) && lenType==Type::Int){
		int length=*((int*)lenValue);
		printf("Stream length: %d\n", length);
		stm->data=new unsigned char[length];
		stm->length=length;
		unsigned char a2;
		for(i=0; i<length; i++){
			a2=(unsigned char)file.get();
			stm->data[i]=a2;
		}
		gotoEOL();
		// the last row should be 'endstream'
		flag=false;
		for(i=0; i<strlen(ESTM); i++){
			file.get(a);
			if(a!=ESTM[i]){
				flag=true;
				break;
			}
		}
		if(flag){
			cout << "warning: 'endstream' not found" << endl;
			return true;
		}
	}else{
		return false;
	}
			
	return true;
}

int PDFParser::judgeType(){
	return judgeType(&file);
}
int PDFParser::judgeType(istream* is){
	// cout << "Type" << endl;
	if(!skipSpaces(is)){
		return -1;
	}
	// cout << "Skip" << endl;
	int delimID=judgeDelimiter(false, is);
	// cout << delimID << endl;
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
	int position=(int) is->tellg();
	int typeID=-1;
	char buffer[128];
	if(!readToken(buffer, 128, is)){
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
	is->seekg(position, ios_base::beg);
	if(readRefID(&v1, &v2, is)){
		typeID=Type::Indirect;
	}else{
		is->seekg(position, ios_base::beg);
		if(readInt(&v1, is)){
			typeID=Type::Int;
		}else{
			is->seekg(position, ios_base::beg);
			if(readReal(&v3, is)){
				typeID=Type::Real;
			}
		}
	}
		

	is->seekg(position, ios_base::beg);
	return typeID;
}
