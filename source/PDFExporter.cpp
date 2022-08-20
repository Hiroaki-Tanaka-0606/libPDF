#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>

#include "PDFExporter.hpp"

using namespace std;

PDFExporter::PDFExporter(PDFParser* parser):
	PP(parser),
	count(0),
	literalStringBorder(0.3)
{
}

bool PDFExporter::exportToFile(char* fileName){
	int i, j;
	file=new ofstream(fileName, ios::binary);

	if(!*file){
		cout << "Error in opening the output file" << endl;
		return false;
	}

	char buffer[1024];
	uchar* binary=new uchar();
	binary->data=new unsigned char[1024];
	// Header
	if(PP->V_catalog.isValid()){
		sprintf(buffer, "%s%s%c%c", HEADER, PP->V_catalog.v, CR, LF);
	}else{
		sprintf(buffer, "%s%s%c%c", HEADER, PP->V_header.v, CR, LF);
	}
	writeData(buffer);
	// binary comment
	binary->data[0]='%';
	for(i=0; i<4; i++){
		binary->data[1+i]=0x80;
	}
	binary->length=5;
	writeData(binary);
	sprintf(buffer, "%c%c", CR, LF);
	writeData(buffer);

	// body
	// list of ref numbers with data, excluding data in the object stream
	cout << "List of Ref numbers with data" << endl;
	for(i=0; i<PP->ReferenceSize; i++){
		if(!(PP->Reference[i]->objStream) && PP->Reference[i]->used){
			printf("Ref #%4d: genNumber %2d\n", i, PP->Reference[i]->genNumber);
		}
	}
	
	// export
	for(i=0; i<PP->ReferenceSize; i++){
		if(PP->Reference[i]->objStream ||  !PP->Reference[i]->used){
			continue;
		}
		printf("Export Ref #%4d\n", i);
		void* refObj;
		int objType;
		if(PP->readRefObj(PP->Reference[i], &refObj, &objType)){
			printf("Object type: %d\n", objType);
			// (objNum) (genNum) obj
			PP->Reference[i]->position=count;
			sprintf(buffer, "%d %d obj%c%c", i, PP->Reference[i]->genNumber, CR, LF);
			writeData(buffer);
			vector<unsigned char> data=exportObj(refObj, objType);
			uchar* data_uc=new uchar();
			data_uc->length=data.size();
			data_uc->data=new unsigned char[data.size()];
			for(j=0; j<data.size(); j++){
				data_uc->data[j]=data[j];
			}
			writeData(data_uc);
			sprintf(buffer, "%c%cendobj%c%c", CR, LF, CR, LF);
			writeData(buffer);
		}
	}
	// footer
	// xref table
	cout << "Export xref table" << endl;
	int trailerPosition=count;
	sprintf(buffer, "xref%c%c", CR, LF);
	writeData(buffer);
	sprintf(buffer, "0 %d%c%c", PP->ReferenceSize, CR, LF);
	writeData(buffer);
	for(i=0; i<PP->ReferenceSize; i++){
		int nextFreeNumber;
		if(!(PP->Reference[i]->objStream) && PP->Reference[i]->used){
			// used
			// no offset
			sprintf(buffer, "%010d %05d %c%c%c", PP->Reference[i]->position, PP->Reference[i]->genNumber,'n', CR, LF);
			writeData(buffer);
		}else{
			// free
			nextFreeNumber=0;
			for(j=i+1; j<PP->ReferenceSize; j++){
				if(PP->Reference[j]->objStream || !PP->Reference[j]->used){
					nextFreeNumber=j;
					break;
				}
			}
			sprintf(buffer, "%010d %05d %c%c%c", nextFreeNumber, PP->Reference[i]->genNumber, 'f', CR, LF);
			writeData(buffer);
		}
	}
	// trailer
	// remove "Encrypt"
	int encryptIndex=PP->trailer.Search((unsigned char*)"Encrypt");
	if(encryptIndex>=0){
		PP->trailer.Delete(encryptIndex);
	}
	cout << "Export trailer" << endl;
	sprintf(buffer, "trailer%c%c", CR, LF);
	writeData(buffer);	
	vector<unsigned char> data=exportObj((void*)&(PP->trailer), Type::Dict);
  // PP->trailer.Print();
	uchar* data_uc=new uchar();
	data_uc->length=data.size();
	data_uc->data=new unsigned char[data.size()];
	for(i=0; i<data.size(); i++){
		data_uc->data[i]=data[i];
	}
	writeData(data_uc);
	// startxref
	sprintf(buffer, "%c%cstartxref%c%c%d%c%c%%%%EOF", CR, LF, CR, LF, trailerPosition, CR, LF);
	writeData(buffer);
	file->close();
	return true;
}

void PDFExporter::writeData(char* buffer){
	int bufLen=strlen(buffer);
	file->write(buffer, bufLen);
	count+=bufLen;
	printf("---- %5d bytes written, total %6d bytes\n", bufLen, count);
}

void PDFExporter::writeData(uchar* binary){
	file->write((char*)binary->data, binary->length);
	count+=binary->length;
	printf("---- %5d bytes written, total %6d bytes\n", binary->length, count);
}

vector<unsigned char> PDFExporter::exportObj(void* obj, int objType){
	vector<unsigned char> ret;
	int i, j;
	bool* obj_b;
	int* obj_i;
	double* obj_do;
	unsigned char* obj_n;
	uchar* obj_st;
	int numNormalLetters;
	int len;
	char buffer[1024];
	Array* obj_a;
	int aSize;

	void* value;
	int type;
	unsigned char* key;
	
	Dictionary* obj_d;
	int dSize;
	vector<unsigned char> ret2;
	vector<unsigned char> ret3;

	Stream* obj_s;

	Indirect* obj_in;
	
	switch(objType){
	case Type::Bool:
		obj_b=(bool*)obj;
		if(*obj_b){
			sprintf(buffer, "true");
		}else{
			sprintf(buffer, "false");
		}
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		break;
	case Type::Int:
		obj_i=(int*)obj;
		sprintf(buffer, "%d", *obj_i);
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		break;
	case Type::Real:
		obj_do=(double*)obj;
		sprintf(buffer, "%f", *obj_do);
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		break;
	case Type::String:
		// count the number of normal letters (0x21~0x7E)
		obj_st=(uchar*)obj;
		numNormalLetters=0;
		for(i=0; i<obj_st->length; i++){
			if(0x21<=obj_st->data[i] && obj_st->data[i]<=0x7E){
				numNormalLetters++;
			}
		}
		if(1.0*numNormalLetters/obj_st->length>literalStringBorder){
			// literal
			ret.push_back((unsigned char)'(');
			for(i=0; i<obj_st->length; i++){
				switch(obj_st->data[i]){
				case 0x0A: // line feed
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'n');
					break;
				case 0x0D: // CR
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'r');
					break;
				case 0x09: // tab
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'t');
					break;
				case 0x08: // backspace
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'b');
					break;
				case 0x0C: // FF
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'f');
					break;
				case 0x28: // left parenthesis
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'(');
					break;
				case 0x29: // right parenthesis
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)')');
					break;
				case 0x5C: // backslash
					ret.push_back((unsigned char)'\\');
					ret.push_back((unsigned char)'\\');
					break;
				default:
					if(0x21<=obj_st->data[i] && obj_st->data[i] <=0x7E){
						// normal letter
						ret.push_back(obj_st->data[i]);
					}else{
						// octal
						sprintf(buffer, "\\%03o", obj_st->data[i]);
						for(j=0; j<strlen(buffer); j++){
							ret.push_back((unsigned char)buffer[j]);
						}
					}
					break;
				}
			}
			
			ret.push_back((unsigned char)')');
		}else{
			// hexadecimal
			ret.push_back((unsigned char)'<');
			for(i=0; i<obj_st->length; i++){
				sprintf(buffer, "%02x", obj_st->data[i]);
				ret.push_back((unsigned char)buffer[0]);
				ret.push_back((unsigned char)buffer[1]);
			}
			ret.push_back((unsigned char)'>');
			
		}
		break;
	case Type::Name:
	  obj_n=(unsigned char*)obj;
		len=unsignedstrlen(obj_n);
		ret.push_back((unsigned char)'/');
		for(i=0; i<len; i++){
			// 0x21~0x7E except #(0x23): normal
			if(0x21<=obj_n[i] && obj_n[i]<=0x7E && obj_n[i]!=0x23){
				ret.push_back(obj_n[i]);
			}else{
				// exception: #xx
				ret.push_back((unsigned char)'#');
				sprintf(buffer, "%02x", obj_n[i]);
				ret.push_back((unsigned char)buffer[0]);
				ret.push_back((unsigned char)buffer[1]);
			}
		}
		break;
	case Type::Array:
	  obj_a=(Array*) obj;
	  aSize=obj_a->getSize();
		ret.push_back((unsigned char)'[');
		for(i=0; i<aSize; i++){
			if(obj_a->Read(i, &value, &type)){
			  ret2=exportObj(value, type);
				copy(ret2.begin(), ret2.end(), back_inserter(ret));
				ret.push_back((unsigned char)' ');
			}
		}
		ret.push_back((unsigned char)']');
		break;
	case Type::Dict:
	  obj_d=(Dictionary*) obj;
		dSize=obj_d->getSize();
		ret.push_back((unsigned char)'<');
		ret.push_back((unsigned char)'<');
		for(i=0; i<dSize; i++){
			if(obj_d->Read(i, &key, &value, &type)){
			  ret2=exportObj(key, Type::Name);
				copy(ret2.begin(), ret2.end(), back_inserter(ret));
				ret.push_back((unsigned char)' ');
			  ret3=exportObj(value, type);
				copy(ret3.begin(), ret3.end(), back_inserter(ret));
				ret.push_back((unsigned char)' ');
			}
		}
		ret.push_back((unsigned char)'>');
		ret.push_back((unsigned char)'>');
		break;
	case Type::Stream:
		// stream dictionary
		obj_s=(Stream*)obj;
		ret2=exportObj((void*)&(obj_s->StmDict), Type::Dict);
		copy(ret2.begin(), ret2.end(), back_inserter(ret));
		sprintf(buffer, "%c%cstream%c%c", CR, LF, CR, LF);
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		// stream data
		if(PP->encrypted && obj_s->decrypted==false){
			PP->encryptObj->DecryptStream(obj_s);
		}
		for(i=0; i<obj_s->length; i++){
			ret.push_back(obj_s->data[i]);
		}
		// stream footer
		sprintf(buffer, "%c%cendstream", CR, LF);
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		break;
	case Type::Null:
		sprintf(buffer, "null");
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		break;
	case Type::Indirect:
		obj_in=(Indirect*) obj;
		sprintf(buffer, "%d %d R", obj_in->objNumber, obj_in->genNumber);
		for(i=0; i<strlen(buffer); i++){
			ret.push_back((unsigned char)buffer[i]);
		}
		break;
	}
	return ret;
}
