
all: readPDF.o

lib=PDFParser.o PDFVersion.o Objects.o

readPDF.o: readPDF_main.o $(lib)
	$(CXX) readPDF_main.o $(lib) -lz -o readPDF.o
	cp readPDF.o ../executables/

.cpp.o:
	$(CXX) -c $<

readPDF_main.o: PDFParser.hpp PDFVersion.hpp Objects.hpp
PDFParser.o: PDFParser.hpp PDFVersion.hpp Objects.hpp
PDFVersion.o: PDFVersion.hpp
Objects.o: Objects.hpp
