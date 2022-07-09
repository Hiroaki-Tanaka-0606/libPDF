
all: readPDF.o

lib=PDFParser.o PDFVersion.o Objects.o Encryption.o
option=-lz -L /home/hiroaki/openssl-3.0.2/ -l ssl -l crypto /usr/local/lib/libgsasl.a

readPDF.o: readPDF_main.o $(lib)
	$(CXX) readPDF_main.o $(lib) $(option) -o readPDF.o
	cp readPDF.o ../executables/

.cpp.o:
	$(CXX) -c $<

readPDF_main.o: PDFParser.hpp PDFVersion.hpp Objects.hpp
PDFParser.o: PDFParser.hpp PDFVersion.hpp Objects.hpp Encryption.hpp
PDFVersion.o: PDFVersion.hpp
Objects.o: Objects.hpp
Encryption.o: Encryption.hpp Objects.hpp
