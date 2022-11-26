// classes of objects

#include <iostream>
#include <cstring>
#include <string>
#include "Objects.hpp"
#include "zlib.h" // /usr/include/zlib.h

#define ASCIIHEX  "ASCIIHexDecode"
#define ASCII85   "ASCII85Decode"
#define LZW       "LZWDecode"
#define FLATE     "FlateDecode"
#define RUNLENGTH "RunLengthDecode"
#define CCITTFAX  "CCITTFaxDecode"
#define JBIG2     "JBIG2Decode"
#define DCT       "DCTDecode"
#define JPXDecode "JPXDecode"
#define CRYPT     "Crypt"

#define ZBUFSIZE 1024
#define DICTSIZE 128
#define ARRAYSIZE 128

using namespace std;

Dictionary::Dictionary(){
	keys.reserve(DICTSIZE);
	values.reserve(DICTSIZE);
	types.reserve(DICTSIZE);
}

Dictionary::Dictionary(Dictionary* original){
	copy(original->keys.begin(), original->keys.end(), back_inserter(keys));
	copy(original->values.begin(), original->values.end(), back_inserter(values));
	copy(original->types.begin(), original->types.end(), back_inserter(types));
}

int Dictionary::Search(unsigned char* key){
	// return the index associated with key, -1 otherwise
	int len=unsignedstrlen(key);
	int i;
	for(i=0; i<keys.size(); i++){
		int len2=unsignedstrlen(keys[i]);
		if(len==len2){
			int j;
			bool flag=true;
			for(j=0; j<len; j++){
				if(key[j]!=keys[i][j]){
					flag=false;
					break;
				}
			}
			if(flag){
				return i;
			}
		}
	}
	return -1;
}

void Dictionary::Merge(Dictionary dict2){
	int i;
	for(i=0; i<dict2.keys.size(); i++){
		Append(dict2.keys[i], dict2.values[i], dict2.types[i]);
	}
}

bool Dictionary::Read(unsigned char* key, void** value, int* type){
	int index=Search(key);
	if(index<0){
		return false;
	}
	*value=values[index];
	*type=types[index];
	return true;
}

bool Dictionary::Read(int index, unsigned char** key, void** value, int* type){
	*key=keys[index];
	*value=values[index];
	*type=types[index];
	return true;
}


void Dictionary::Append(unsigned char* key, void* value, int type){
	// check whether the same key already exists
	if(Search(key)>=0){
		printf("Key %s already exists, skip it\n", key);
		return;
	}	
	keys.push_back(key);
	values.push_back(value);
	types.push_back(type);
}

bool Dictionary::Update(unsigned char* key, void* value, int type){
	int index=Search(key);
	if(index<0){
		printf("Key %s does not exist\n", key);
		return false;
	}
	values[index]=value;
	types[index]=type;
	return true;
}

void Dictionary::Print(){
	Print(0);
}

void Dictionary::Print(int indent){
	int i;
	char indentStr[indent*2+1];
	for(i=0; i<indent; i++){
		indentStr[2*i]=' ';
		indentStr[2*i+1]=' ';
	}
	indentStr[indent*2]='\0';
	for(i=0; i<keys.size(); i++){
		if(types[i]==Type::Dict || types[i]==Type::Array){
			printf("%sName: %s, TypeID: %d\n", indentStr, keys[i], types[i]);
			if(types[i]==Type::Dict){
				Dictionary* dictValue=(Dictionary*)values[i];
				dictValue->Print(indent+1);
			}else{
				Array* arrayValue=(Array*)values[i];
				arrayValue->Print(indent+1);
			}
		}else{
			char* printValue=printObj(values[i], types[i]);
			printf("%sName: %s, TypeID: %d, Value: %s\n", indentStr, keys[i], types[i], printValue);
			delete printValue;
		}
	}
}

void Dictionary::Delete(int index){
	keys.erase(keys.begin()+index);
	values.erase(values.begin()+index);
	types.erase(types.begin()+index);
}

Array::Array(){
	values.reserve(ARRAYSIZE);
	types.reserve(ARRAYSIZE);
}

void Array::Append(void* value, int type){
	values.push_back(value);
	types.push_back(type);
}

void Array::Print(){
	Print(0);
}
void Array::Print(int indent){
	int i;
	char indentStr[indent*2+1];
	for(i=0; i<indent; i++){
		indentStr[2*i]=' ';
		indentStr[2*i+1]=' ';
	}
	indentStr[indent*2]='\0';
	for(i=0; i<values.size(); i++){
		if(types[i]==Type::Dict || types[i]==Type::Array){
			printf("%sElement #%d, TypeID: %d\n", indentStr, i, types[i]);
			if(types[i]==Type::Dict){
				Dictionary* dictValue=(Dictionary*)values[i];
				dictValue->Print(indent+1);
			}else{
				Array* arrayValue=(Array*)values[i];
				arrayValue->Print(indent+1);
			}
		}else{
			char* printValue=printObj(values[i], types[i]);
			printf("%sElement #%d, TypeID: %d, Value: %s\n", indentStr, i, types[i], printValue);
			delete printValue;
		}
	}
}

int Dictionary::getSize(){
	return values.size();
}

int Array::getSize(){
	return values.size();
}

bool Array::Read(int index, void** value, int* type){
	if(index>=0 && index<values.size()){
		*value=values[index];
		*type=types[index];
		return true;
	}
	return false;
}

Indirect::Indirect(){
	objNumber=-1;
	objStream=false;
}

Stream::Stream():
	decrypted(false)
{	
}


char* printObj(void* value, int type){
	char* buffer=new char[256];
	strcpy(buffer, "");
	uchar* stringValue;
	unsigned char* nameValue;
	Indirect* indirectValue;
	int len;
	switch(type){
	case Type::Bool:
		if(*((bool*)value)){
		  strcpy(buffer, "true");
		}else{
			strcpy(buffer, "false");
		}
		break;
	case Type::Int:
		sprintf(buffer, "%d", *((int*)value));
		break;
	case Type::Real:
		sprintf(buffer, "%10.3f", *((double*)value));
		break;
	case Type::String:
	  stringValue=(uchar*)value;
		len=stringValue->length;
		sprintf(buffer, "%-32s ... [L=%d]", stringValue->data, len);
		break;
	case Type::Name:
	  nameValue=(unsigned char*)value;
		len=unsignedstrlen(nameValue);
		sprintf(buffer, "%-16s ... [L=%d]", nameValue, len);
		break;
	case Type::Null:
		strcpy(buffer, "null");
		break;
	case Type::Indirect:
		indirectValue=(Indirect*)value;
		sprintf(buffer, "objNumber=%d, genNumber=%d", indirectValue->objNumber, indirectValue->genNumber);
		break;
	}

	return buffer;
}


int unsignedstrlen(unsigned char* a){
	int len=0;
	while(true){
		if(a[len]!='\0'){
			len++;
		}else{
			return len;
		}
	}
}

void unsignedstrcpy(unsigned char* dest, unsigned char* data){
	int len=unsignedstrlen(data);
	int i;
	for(i=0; i<len; i++){
		dest[i]=data[i];
	}
	dest[len]='\0';
}
		
		
bool Stream::Decode(){
	// cout << "Decode" << endl;
	int filtersType;
	void* filtersValue;
	int i;
	
	if(!StmDict.Read((unsigned char*)"Filter", (void**)&filtersValue, &filtersType)){
		// no filter
		cout << "Stream with no Filter" << endl;
		decoded=new unsigned char[length];
		for(i=0; i<length; i++){
			decoded[i]=data[i];
		}
		return true;
	}

	unsigned char* encoded=new unsigned char[length];
	for(i=0; i<length; i++){
		encoded[i]=data[i];
	}
	int encodedLength=length;
	int decodedLength;
	
	// get decode paramters
	int parmsType;
	void* parmsValue;
	bool parmsExist=StmDict.Read((unsigned char*)"DecodeParms", (void**)&parmsValue, &parmsType);

	Dictionary* parmDict;
	void* parmValue;
	int parmType;
	
	unsigned char* filterName;
	void* filterValue;
	int filterType;
	
	if(filtersType==Type::Array){
		Array* filterArray=(Array*)filtersValue;
		Array* parmsArray=(Array*)parmsValue;
		int filterSize=filterArray->getSize();
		for(i=0; i<filterSize; i++){
			// prepare filter
			printf("Filter #%d\n", i);
			if(!filterArray->Read(i, (void**)&filterValue, &filterType)){
				cout << "Filter load error" << endl;
				return false;
			}
			if(filterType!=Type::Name){
				cout << "Filter name error" << endl;
				return false;
			}
			filterName=(unsigned char*)filterValue;

			// prepare parm
			if(parmsExist){
				if(!parmsArray->Read(i, (void**)&parmValue, &parmType)){
					cout << "DecodeParms load error" << endl;
					return false;
				}
			}else{
				parmType==Type::Null;
			}
			
			if(parmType==Type::Dict){
				parmDict=(Dictionary*)parmValue;
				decodedLength=decodeData(encoded, filterName, parmDict, encodedLength, &decoded);
			}else if(parmType==Type::Null){
				decodedLength=decodeData(encoded, filterName, NULL, encodedLength, &decoded);
			}
			// copy
			encoded=decoded;
			encodedLength=decodedLength;
		}
	}else if(filtersType==Type::Name){
		filterName=(unsigned char*)filtersValue;
		if(parmsExist){
			if(parmsType==Type::Dict){
				parmDict=(Dictionary*)parmsValue;
				decodedLength=decodeData(encoded, filterName, parmDict, encodedLength, &decoded);
			}else if(parmsType==Type::Null){
				decodedLength=decodeData(encoded, filterName, NULL, encodedLength, &decoded);
			}else{
				cout << "Invalid Filter type" << endl;
				return false;
			}
		}else{
			decodedLength=decodeData(encoded, filterName, NULL, encodedLength, &decoded);
		}
	}else{
		cout << "Invalid Filter value" << endl;
		return false;
	}
	dlength=decodedLength;

	return true;
}

bool Stream::Encode(){
	int filtersType;
	void* filtersValue;
	int i;
	
	if(!StmDict.Read((unsigned char*)"Filter", (void**)&filtersValue, &filtersType)){
		// no filter
		cout << "Stream with no Filter" << endl;
		data=new unsigned char[dlength];
		for(i=0; i<dlength; i++){
			data[i]=decoded[i];
			length=dlength;
		}
		return true;
	}

	unsigned char* raw=new unsigned char[dlength];
	for(i=0; i<dlength; i++){
		raw[i]=decoded[i];
	}
	int decodedLength=dlength;
	int encodedLength;
	
	// get decode paramters
	int parmsType;
	void* parmsValue;
	bool parmsExist=StmDict.Read((unsigned char*)"DecodeParms", (void**)&parmsValue, &parmsType);

	Dictionary* parmDict;
	void* parmValue;
	int parmType;
	
	unsigned char* filterName;
	void* filterValue;
	int filterType;
	
	if(filtersType==Type::Array){
		Array* filterArray=(Array*)filtersValue;
		Array* parmsArray=(Array*)parmsValue;
		int filterSize=filterArray->getSize();
		for(i=filterSize-1; i>=0; i--){
			// prepare filter
			printf("Filter #%d\n", i);
			if(!filterArray->Read(i, (void**)&filterValue, &filterType)){
				cout << "Filter load error" << endl;
				return false;
			}
			if(filterType!=Type::Name){
				cout << "Filter name error" << endl;
				return false;
			}
			filterName=(unsigned char*)filterValue;

			// prepare parm
			if(parmsExist){
				if(!parmsArray->Read(i, (void**)&parmValue, &parmType)){
					cout << "DecodeParms load error" << endl;
					return false;
				}
			}else{
				parmType==Type::Null;
			}
			
			if(parmType==Type::Dict){
				parmDict=(Dictionary*)parmValue;
				encodedLength=encodeData(raw, filterName, parmDict, decodedLength, &data);
			}else if(parmType==Type::Null){
				encodedLength=encodeData(raw, filterName, NULL, decodedLength, &data);
			}
			// copy
			raw=data;
			decodedLength=encodedLength;
		}
	}else if(filtersType==Type::Name){
		filterName=(unsigned char*)filtersValue;
		if(parmsExist){
			if(parmsType==Type::Dict){
				parmDict=(Dictionary*)parmsValue;
				encodedLength=encodeData(raw, filterName, parmDict, decodedLength, &data);
			}else if(parmsType==Type::Null){
				encodedLength=encodeData(raw, filterName, NULL, decodedLength, &data);
			}else{
				cout << "Invalid Filter type" << endl;
				return false;
			}
		}else{
			encodedLength=encodeData(raw, filterName, NULL, decodedLength, &data);
		}
	}else{
		cout << "Invalid Filter value" << endl;
		return false;
	}
	length=encodedLength;

	return true;
}

bool unsignedstrcmp(unsigned char* a, unsigned char* b){
	int alength=unsignedstrlen(a);
	int blength=unsignedstrlen(b);
	if(alength!=blength){
		return false;
	}
	int i;
	for(i=0; i<alength; i++){
		if(a[i]!=b[i]){
			return false;
		}
	}
	return true;
}

int decodeData(unsigned char* encoded, unsigned char* filter, Dictionary* parm, int encodedLength, unsigned char** decoded){
	printf("Filter: %s, encoded size: %d\n", filter, encodedLength);

	unsigned char inBuf[ZBUFSIZE];
	unsigned char outBuf[ZBUFSIZE];
	z_stream z;
	z.zalloc=Z_NULL;
	z.zfree=Z_NULL;
	z.opaque=Z_NULL;

	string output("");
	
	int remainingData=encodedLength;
	int decodedData=0;
	int i;
	int zStatus;
	int decodedLength;

	/* for debug 
	for(i=0; i<remainingData; i++){
		printf("%02x ", (unsigned int)encoded[i]);
	}
	cout << endl;*/
	

	if(unsignedstrcmp(filter, (unsigned char*)FLATE)){
		// cout << "Flate" << endl;
		if(parm!=NULL){
			parm->Print();
		}else{
			// cout << "No parameters given" << endl;
		}

		// zlib preparation
	  z.next_in=Z_NULL;
		z.avail_in=0;
		if(inflateInit(&z)!=Z_OK){
			printf("inflateInit error: %s\n", z.msg);
			return 0;
		}

		// output buffer
		z.next_out=&outBuf[0];
		z.avail_out=ZBUFSIZE;

		while(true){
			// copy the data to inBuf
			if(z.avail_in==0){
				int copyLength=min(remainingData, ZBUFSIZE);
				for(i=0; i<copyLength; i++){
					inBuf[i]=encoded[decodedData+i];
				}
				z.next_in=&inBuf[0];
				z.avail_in=copyLength;
			
				decodedData+=copyLength;
				remainingData-=copyLength;
				//printf("Decompress %d bytes\n", copyLength);
			}
			
			// decompress
			zStatus=inflate(&z, Z_NO_FLUSH);
			//printf("Remaining output space: %d bytes\n", z.avail_out);
			//printf("Remaining input buffer: %d bytes\n", z.avail_in);
			if(zStatus==Z_STREAM_END){
				break;
			}else if(zStatus==Z_OK){
				// cout << "inflate ok" << endl;
				if(remainingData==0 && z.avail_in==0){
					break;
				}
			}else{
				printf("inflate error: %s\n", z.msg);
				return 0;
			}
			if(z.avail_out==0){
				// output buffer is full
				for(i=0; i<ZBUFSIZE; i++){
					output+=outBuf[i];
				}
				z.avail_out=ZBUFSIZE;
				z.next_out=&outBuf[0];
			}
		}
		// finish decompression
		if(inflateEnd(&z)!=Z_OK){
			printf("inflateEnd error: %s", z.msg);
			return 0;
		}
		for(i=0; i<(ZBUFSIZE-z.avail_out); i++){
			output+=outBuf[i];
		}
		// cout << output.length() << " LENGTH" << endl;
		*decoded=new unsigned char[output.length()];
		for(i=0; i<output.length(); i++){
			(*decoded)[i]=(unsigned char)output[i];
		}
		decodedLength=output.length();

		// predictor
		if(parm!=NULL && parm->Search((unsigned char*)"Predictor")>=0){
			int predictorType;
			void* predictorValue;
			if(parm->Read((unsigned char*)"Predictor", (void**)&predictorValue, &predictorType) && predictorType==Type::Int){
				int predictor=*((int*)predictorValue);
				if(predictor>=10){
					// PNG predictor
					decodedLength=PNGPredictor(decoded, decodedLength, parm);
					cout << "PNG Predictor finished" << endl;
				}
			}	
				
		}
		return decodedLength;
	}
	
	return 0;
}

int encodeData(unsigned char* decoded, unsigned char* filter, Dictionary* parm, int decodedLength, unsigned char** encoded){
	printf("Filter: %s, decoded length: %d\n", filter, decodedLength);

	unsigned char inBuf[ZBUFSIZE];
	unsigned char outBuf[ZBUFSIZE];
	z_stream z;
	z.zalloc=Z_NULL;
	z.zfree=Z_NULL;
	z.opaque=Z_NULL;

	string output("");
	
	int remainingData=decodedLength;
	int encodedData=0;
	int i;
	int zStatus;
	int encodedLength;

	/* for debug 
	for(i=0; i<remainingData; i++){
		printf("%02x ", (unsigned int)decoded[i]);
	}
	cout << endl;*/
	

	if(unsignedstrcmp(filter, (unsigned char*)FLATE)){
		// cout << "Flate" << endl;
		if(parm!=NULL){
			parm->Print();
		}else{
			// cout << "No parameters given" << endl;
		}

		// zlib preparation
	  z.next_in=Z_NULL;
		z.avail_in=0;
		if(deflateInit(&z, Z_DEFAULT_COMPRESSION)!=Z_OK){
			printf("deflateInit error: %s\n", z.msg);
			return 0;
		}

		// output buffer
		z.next_out=&outBuf[0];
		z.avail_out=ZBUFSIZE;

		while(true){
			// copy the data to inBuf
			if(z.avail_in==0){
				int copyLength=min(remainingData, ZBUFSIZE);
				for(i=0; i<copyLength; i++){
					inBuf[i]=decoded[encodedData+i];
				}
				z.next_in=&inBuf[0];
				z.avail_in=copyLength;
			
				encodedData+=copyLength;
				remainingData-=copyLength;
				printf("Compress %d bytes\n", copyLength);
			}
			int keyword=(z.avail_in==0?Z_FINISH:Z_NO_FLUSH);
			
			// compress
			zStatus=deflate(&z, keyword);
			printf("Remaining output space: %d bytes\n", z.avail_out);
			printf("Remaining input buffer: %d bytes\n", z.avail_in);
			if(zStatus==Z_STREAM_END){
				cout << "end" << endl;
				break;
			}else if(zStatus==Z_OK){
				cout << "deflate ok" << endl;
				if(keyword==Z_FINISH && zStatus==Z_STREAM_END){
					cout << "finish" << endl;
					break;
				}
			}else{
				printf("deflate error: %s, %d\n", z.msg, zStatus);
				return 0;
			}
			if(z.avail_out==0){
				cout << "Buffer full" << endl;
				// output buffer is full
				for(i=0; i<ZBUFSIZE; i++){
					output+=outBuf[i];
				}
				z.avail_out=ZBUFSIZE;
				z.next_out=&outBuf[0];
			}
		}
		// finish compression
		if(deflateEnd(&z)!=Z_OK){
			printf("deflateEnd error: %s", z.msg);
			return 0;
		}
			printf("Remaining output space: %d bytes\n", z.avail_out);
			printf("Remaining input buffer: %d bytes\n", z.avail_in);
		for(i=0; i<(ZBUFSIZE-z.avail_out); i++){
			output+=outBuf[i];
		}
		// cout << output.length() << " LENGTH" << endl;
		*encoded=new unsigned char[output.length()];
		for(i=0; i<output.length(); i++){
			(*encoded)[i]=(unsigned char)output[i];
		}
		encodedLength=output.length();
		return encodedLength;
	}
	
	return 0;
}

int PNGPredictor(unsigned char** pointer, int length, Dictionary* parm){
	// TODO: Average, Paeth predictions
	// TODO: Colors, BitsPerComponent
	// read parameters
	int columnsType;
	void* columnsValue;
	int columns=1;
	if(parm->Read((unsigned char*)"Columns", (void**)&columnsValue, &columnsType) && columnsType==Type::Int){
		columns=*((int*)columnsValue);
	}
		
	if(length%(columns+1)!=0){
		cout << "PNGPredictor error: columns and length mismatch" << endl;
		return 0;
	}
	int numRows=length/(columns+1);
	unsigned char* restored=new unsigned char[numRows*columns];
	int i, j;
	for(i=0; i<numRows; i++){
		int algorithm=(int)(*pointer)[i*(columns+1)];
		switch(algorithm){
		case 0:
			// none
			for(j=0; j<columns;j++){
				restored[i*columns+j]=(*pointer)[i*(columns+1)+j+1];
			}
			break;
		case 1:
			// Sub
			for(j=0; j<columns; j++){
				if(j<1){
					restored[i*columns+j]=(*pointer)[i*(columns+1)+j+1];
				}else{
					restored[i*columns+j]=(*pointer)[i*(columns+1)+j+1]+restored[i*columns+j-1];
				}
			}
			break;
		case 2:
			// Up
			if(i==0){
				for(j=0; j<columns; j++){
					restored[i*columns+j]=(*pointer)[i*(columns+1)+j+1];
				}
			}else{
				for(j=0; j<columns; j++){
					restored[i*columns+j]=(*pointer)[i*(columns+1)+j+1]+restored[(i-1)*columns+j];
				}
			}
			break;
		}
	}
	*pointer=&restored[0];
	return numRows*columns;
}

Page::Page(){
}

uchar::uchar():
	decrypted(false),
	hex(false)
{
}

int byteSize(int n){
	int byte=1;
	while(n>=256){
		byte++;
		n/=256;
	}
	return byte;
}

uchar* dateString(){
	uchar* date=new uchar();
	date->length=23;
	date->data=new unsigned char[24];

	time_t t=time(NULL);
	tm* now=localtime(&t);
	int Y=now->tm_year+1900;
	int M=now->tm_mon+1;
	int D=now->tm_mday;
	int H=now->tm_hour;
	int m=now->tm_min;
	int S=now->tm_sec;

	char* tz=new char[8];
	strcpy(tz, "+09'00'");
	
	sprintf((char*)date->data, "D:%04d%02d%02d%02d%02d%02d%7s",Y,M,D,H,m,S,tz);
	return date;
}
