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
	
};

class Array{
private:
	vector<void*> values;
	vector<int> types;
public:
	Array();
};
