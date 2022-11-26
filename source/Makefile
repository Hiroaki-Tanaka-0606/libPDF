
all: readPDF.o exportPDF.o changePerms.o verifyPDFSign.o addSignature.o

lib_r=PDFParser.o PDFVersion.o Objects.o Encryption.o
lib_e=PDFParser.o PDFVersion.o Objects.o Encryption.o PDFExporter.o
lib_c=PDFParser.o PDFVersion.o Objects.o Encryption.o PDFExporter.o
lib_v=PDFParser.o PDFVersion.o Objects.o Encryption.o
lib_a=PDFParser.o PDFVersion.o Objects.o Encryption.o PDFExporter.o

option=-g -lz -L /home/hiroaki/openssl-3.0.2/ -l ssl -l crypto /usr/local/lib/libgsasl.a

readPDF.o: readPDF_main.o $(lib_r)
	$(CXX) readPDF_main.o $(lib_r) $(option) -o readPDF.o
	cp readPDF.o ../executables/

exportPDF.o: exportPDF_main.o $(lib_e)
	$(CXX) exportPDF_main.o $(lib_e) $(option) -o exportPDF.o
	cp exportPDF.o ../executables/

changePerms.o: changePerms_main.o $(lib_c)
	$(CXX) changePerms_main.o $(lib_c) $(option) -o changePerms.o
	cp changePerms.o ../executables/

verifyPDFSign.o: verifyPDFSign_main.o $(lib_v)
	$(CXX) verifyPDFSign_main.o $(lib_v) $(option) -o verifyPDFSign.o
	cp verifyPDFSign.o ../executables/

addSignature.o: addSignature_main.o $(lib_a)
	$(CXX) addSignature_main.o $(lib_a) $(option) -o addSignature.o
	cp addSignature.o ../executables/

.cpp.o:
	$(CXX) -c -g $<

exportPDF_main.o: PDFParser.hpp PDFVersion.hpp Objects.hpp PDFExporter.hpp
readPDF_main.o: PDFParser.hpp PDFVersion.hpp Objects.hpp
changePerms_main.o: PDFParser.hpp PDFVersion.hpp Objects.hpp PDFExporter.hpp
PDFParser.o: PDFParser.hpp PDFVersion.hpp Objects.hpp Encryption.hpp
PDFVersion.o: PDFVersion.hpp
Objects.o: Objects.hpp
Encryption.o: Encryption.hpp Objects.hpp
PDFExporter.o: PDFExporter.hpp PDFParser.hpp PDFVersion.hpp Objects.hpp
