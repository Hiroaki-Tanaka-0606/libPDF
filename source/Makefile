
all: readPDF.o

lib=PDFParser.o PDFVersion.o Objects.o

readPDF.o: readPDF_main.o $(lib)
	$(CXX) readPDF_main.o $(lib) -o readPDF.o
	cp readPDF.o ../executables/

.cpp.o:
	$(CXX) -c $<

readPDF_main.cpp: PDFParser.hpp PDFVersion.hpp Objects.hpp
PDFParser.cpp: PDFParser.hpp PDFVersion.hpp Objects.hpp
PDFVersion.cpp: PDFVersion.hpp
Objects.cpp: Objects.hpp
