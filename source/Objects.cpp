// classes of objects

#include <iostream>
#include <cstring>
#include "Objects.hpp"

Dictionary::Dictionary(){
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

Array::Array(){
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

Indirect::Indirect(){
}

char* printObj(void* value, int type){
	char* buffer=new char[256];
	strcpy(buffer, "");
	unsigned char* stringValue;
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
	  stringValue=(unsigned char*)value;
		len=unsignedstrlen(stringValue);
		sprintf(buffer, "%-32s ... [L=%d]", stringValue, len);
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
		
	
