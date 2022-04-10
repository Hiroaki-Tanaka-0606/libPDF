
all: readPDF.o

lib=PDFParser.o PDFVersion.o

readPDF.o: readPDF_main.o $(lib)
	$(CXX) readPDF_main.o $(lib) -o readPDF.o
	cp readPDF.o ../executables/

.cpp.o:
	$(CXX) -c $<

PDFParser.cpp: PDFParser.hpp PDFVersion.hpp
PDFVersion.cpp: PDFVersion.hpp
