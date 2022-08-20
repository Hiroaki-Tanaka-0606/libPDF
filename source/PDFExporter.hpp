// class PDFExporter
#include <vector>

#ifndef INCLUDE_PARSER
#include "PDFParser.hpp"
#define INCLUDE_PARSER 1
#endif

using namespace std;

class PDFExporter{
private:
	PDFParser* PP;
	ofstream* file;
	int count; // current position of the file pointer
	void writeData(char* buffer);
	void writeData(uchar* binary);
	vector<unsigned char> exportObj(void* obj, int objType);
	double literalStringBorder;
public:
	PDFExporter(PDFParser* parser);
	bool exportToFile(char* fileName);
};
