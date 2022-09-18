// read PDF executable

#include <fstream>
#include <iostream>
#include "PDFParser.hpp"
#include "PDFExporter.hpp"

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

	PDFExporter PE(&PP);
	if(PE.exportToFile(argv[2],true)){
		cout << "Export succeeded" << endl;
	}else{
		cout << "Export failed" << endl;
	}
	
	return 0;
	
}
