// class PDFParser

#include <fstream>
#include <string>
#include "PDFVersion.hpp"
#include "Objects.hpp"

using namespace std;

class PDFParser{
private:
	ifstream file;
	bool error;
	int fileSize;
	int offset;
	int lastXRef;
	bool findHeader();
	bool isSpace(char a);
	bool isEOL(char a);
	bool isDelimiter(char a);
	bool isOctal(char a);
	int judgeDelimiter(bool skip);
	int judgeType();
	bool gotoEOL();
	bool gotoEOL(int flag);
	bool findFooter();
	void backtoEOL();
	bool findSXref();
	void readLine(char* buffer, int n);
	int readTrailer(int position);
	int readXRef(int position);
	bool readObjID(int* objNumber, int* genNumber);
	bool readInt(int* value);
	bool readRefID(int* objNumber, int* genNumber);
	bool readReal(double* value);
	bool readBool(bool* value);
	bool readToken(char* buffer, int n);
	bool readArray(Array* array);
	unsigned char* readName();
	unsigned char* readString();
	bool skipSpaces();
	bool readDict(Dictionary* dict);
	bool readStream(Stream* stm);
	bool readStream(Stream* stm, bool outputError);
	bool investigatePages(Indirect* pages, int* pageCount);
public:
	PDFParser(char* fileName);
	PDFVersion V_header;
	PDFVersion V_catalog;
	Dictionary trailer;
	Indirect** Reference;
	int ReferenceSize;
	Page** Pages;
	int PagesSize;
	bool hasError();
	bool readRefObj(Indirect* ref, void** object, int* objType);
	bool readPage(int index, unsigned char* key, void** value, int* type, bool inheritable);
};
