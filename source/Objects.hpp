// classes of objects

#include <vector>
#include <string>
#include <cstddef>

using namespace std;

class Type{
public:
	static const int Bool    =0;
	static const int Int     =1;
	static const int Real    =2;
	static const int String  =3;
	static const int Name    =4;
	static const int Array   =5;
	static const int Dict    =6;
	static const int Stream  =7;
	static const int Null    =8;
	static const int Indirect=9;
};

class Delimiter{
public:
	static const int LP  =0;  // (
	static const int RP  =1;  // )
	static const int LT  =2;  // <
	static const int GT  =3;  // >
	static const int LSB =4;  // [
	static const int RSB =5;  // ]
	static const int LCB =6;  // {
	static const int RCB =7;  // }
	static const int SOL =8;  // /
	static const int PER =9;  // %
	static const int LLT =10; // <<
	static const int GGT =11; // >>
};

class Dictionary{
private:
	vector<unsigned char*> keys;
	vector<void*> values;
	vector<int> types;
public:
	Dictionary();
	void Append(unsigned char* key, void* value, int type);
	void Print(int indent);
	void Print();
	int Search(unsigned char* key);
	bool Read(unsigned char* key, void** value, int* type);
	void Merge(Dictionary dict2);
};

class Array{
private:
	vector<void*> values;
	vector<int> types;
public:
	Array();
	void Append(void* value, int type);
	void Print();
	void Print(int indent);
};

class Indirect{
public:
	int objNumber;
	int genNumber;
	Indirect();
};

char* printObj(void* value, int type);

int unsignedstrlen(unsigned char* a);
