// class PDFParser

#include <fstream>
#include "PDFVersion.hpp"

using namespace std;

class PDFParser{
private:
	ifstream file;
	bool error;
	int fileSize;
	int offset;
	PDFVersion V_header;
public:
	PDFParser(char* fileName);
	bool hasError();
	bool findHeader();
	bool isSpace(char a);
	bool isEOL(char a);
	bool gotoEOL();
	bool gotoEOL(int flag);
	bool findFooter();
	void backtoEOL();
	bool findXref();
};
