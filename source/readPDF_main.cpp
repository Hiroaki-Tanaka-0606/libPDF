// read PDF executable

#include <fstream>
#include <iostream>
#include "PDFParser.hpp"

using namespace std;
int main(int argc, char** argv){
	cout << "Read PDF file" << endl;
	if(argc<2){
		printf("Usage: %s input\n", argv[0]);
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

	return 0;
	
}
