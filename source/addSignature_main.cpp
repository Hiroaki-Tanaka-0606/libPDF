// read PDF executable

#include <fstream>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <cstring>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "PDFParser.hpp"
#include "PDFExporter.hpp"

using namespace std;

int findString(ifstream* file, char* keyword);

int main(int argc, char** argv){
	// constants
	int signBufferSize=16384;
	char* DigestMethod=new char[64];
	strcpy(DigestMethod, "SHA256");
	char* Signer=new char[32];
	strcpy(Signer, "Hiroaki Tanaka");
	uchar* uSigner=new uchar();
	uSigner->length=strlen(Signer);
	uSigner->data=new unsigned char[strlen(Signer)];
	unsignedstrcpy(uSigner->data, (unsigned char*)Signer);
	char* SigFieldName=new char[32];
	strcpy(SigFieldName, "Signature");
	uchar* uSigFieldName=new uchar();
	uSigFieldName->length=strlen(SigFieldName);
	uSigFieldName->data=new unsigned char[strlen(SigFieldName)];
	unsignedstrcpy(uSigFieldName->data, (unsigned char*)SigFieldName);

	// end of constants
	cout << "Add a digital signature" << endl;
	if(argc<5){
		printf("Usage: %s input output private_key.pem public_key.pem page_number (certificates.pem)\n", argv[0]);
		return -1;
	}
	printf("Input file: %s\n", argv[1]);

	int inputSize=(int)filesystem::file_size(argv[1]);
	printf("File size: %d\n", inputSize);

	PDFParser PP(argv[1]);
	
	if(PP.hasError()){
		cout << "The input file has error(s)" << endl;
		return -1;
	}else{
		cout << "Read succeeded" << endl;
	}

	cout << endl;
	// get document catalog dictionary
	cout << "Document catalog dictionary" << endl;
	int dCatalogType;
	void* dCatalogValue;
	Indirect* dCatalogRef;
	Dictionary* dCatalog;
	if(PP.trailer.Read((unsigned char*)"Root", (void**)&dCatalogValue, &dCatalogType) && dCatalogType==Type::Indirect){
	  dCatalogRef=(Indirect*)dCatalogValue;
		if(PP.readRefObj(dCatalogRef, (void**)&dCatalogValue, &dCatalogType) && dCatalogType==Type::Dict){
		  dCatalog=(Dictionary*)dCatalogValue;
			dCatalog->Print();
		}else{
			cout << "Error in document catalog dictionary" << endl;
			return -1;
		}
	}else{
		cout << "Error2 in document catalog dictionary" << endl;
		return -1;
	}
	cout << endl;

	// /Perms in document catalog dictionary
	cout << "/Perms in the Document catalog dictionary" << endl;
	int PermsType;
	void* PermsValue;
	Dictionary* Perms;
	if(dCatalog->Search((unsigned char*)"Perms")<0){
		cout << "/Perms not found in document catalog dictionary, add it" << endl;
		Perms=new Dictionary();
		dCatalog->Append((unsigned char*)"Perms", Perms, Type::Dict);
	}else{
		cout << "/Perms found" << endl;
		if(dCatalog->Read((unsigned char*)"Perms", (void**)&PermsValue, &PermsType) && PermsType==Type::Dict){
			Perms=(Dictionary*)PermsValue;
		}else{
			cout << "Error in read /Perms" << endl;
		}
	}

	// update the XRef stream
	Indirect* dCatalogRef_in_XRef=PP.Reference[dCatalogRef->objNumber];
	dCatalogRef_in_XRef->position=-1;
	dCatalogRef_in_XRef->value=dCatalog;
	dCatalogRef_in_XRef->type=Type::Dict;

	// DocMDP
	int DocMDPType;
	void* DocMDPRefValue;
	Indirect* DocMDPRef;
	void* DocMDPValue;
	Dictionary* DocMDP;
	int i;
	if(Perms->Search((unsigned char*)"DocMDP")<0){
		cout << "/DocMDP not found in permissions dictionary, make it" << endl;
		// new DocMDP (Indirect dictionary)
		DocMDP=new Dictionary();
		int DocMDPRefIndex=PP.ReferenceSize;
		PP.ReferenceSize++;
		Indirect** Reference_new=new Indirect*[PP.ReferenceSize];
		for(i=0; i<PP.ReferenceSize-1; i++){
			Reference_new[i]=PP.Reference[i];
		}
	  DocMDPRef=new Indirect();
		DocMDPRef->objNumber=DocMDPRefIndex;
		printf("Added object number: %d\n", DocMDPRefIndex);
		DocMDPRef->genNumber=0;
		DocMDPRef->used=true;
		DocMDPRef->type=Type::Dict;
		DocMDPRef->objStream=false;
		Reference_new[DocMDPRefIndex]=DocMDPRef;
		PP.Reference=Reference_new;
	}else{
		if(Perms->Read((unsigned char*)"DocMDP", (void**)&DocMDPRefValue, &DocMDPType) && DocMDPType==Type::Indirect){
			DocMDPRef=(Indirect*)DocMDPRefValue;
			if(PP.readRefObj((Indirect*)DocMDPRef, (void**)&DocMDPValue, &DocMDPType) && DocMDPType==Type::Dict){
				DocMDP=(Dictionary*)DocMDPValue;
			}else{
				cout << "Error in read indirect object" << endl;
				return -1;
			}
		}else{
			cout << "Error in read /DocMDP in permissions dictionary" << endl;
		}
	}

	// remove elements in DocMDP
  while(DocMDP->getSize()>0){
		DocMDP->Delete(0);
	}

	// remove elements in Perms
	while(Perms->getSize()>0){
		Perms->Delete(0);
	}

	// update DocMDP in XRef
	Indirect* DocMDP_in_XRef=PP.Reference[DocMDPRef->objNumber];
	DocMDP_in_XRef->position=-1;
	DocMDP_in_XRef->value=DocMDP;
	DocMDP_in_XRef->type=Type::Dict;
	
	// add DocMDP in Perms
	Perms->Append((unsigned char*)"DocMDP", DocMDPRef, Type::Indirect);

	// DocMDP data
	DocMDP->Append((unsigned char*)"Type", (unsigned char*)"Sig", Type::Name);
	DocMDP->Append((unsigned char*)"Filter", (unsigned char*)"Adobe.PPKLite", Type::Name);
	DocMDP->Append((unsigned char*)"SubFilter", (unsigned char*)"adbe.pkcs7.detached", Type::Name);

	uchar* signBuffer=new uchar();
	signBuffer->length=signBufferSize;
	signBuffer->data=new unsigned char[signBufferSize];
	for(i=0; i<signBufferSize; i++){
		signBuffer->data[i]='\0';
	}
	signBuffer->hex=true;
	DocMDP->Append((unsigned char*)"Contents", signBuffer, Type::String);

	int approxOutSize=inputSize+signBufferSize;
	int tentRangeDigit=1;
	while(approxOutSize>pow(10, tentRangeDigit)){
		tentRangeDigit++;
	}
	int tentRangeValue=pow(10, tentRangeDigit);
	Array* tentByteRange=new Array();
	for(i=0; i<4; i++){
		tentByteRange->Append(&tentRangeValue,Type::Int);
	}
	DocMDP->Append((unsigned char*)"ByteRange", tentByteRange, Type::Array);

	Array* Reference=new Array();
	DocMDP->Append((unsigned char*)"Reference", Reference, Type::Array);
	Dictionary* DocMDPDict=new Dictionary();
	Reference->Append(DocMDPDict, Type::Dict);
	DocMDPDict->Append((unsigned char*)"Type", (unsigned char*)"SigRef", Type::Name);
	DocMDPDict->Append((unsigned char*)"TransformMethod", (unsigned char*)"DocMDP", Type::Name);
	Dictionary* trParams=new Dictionary();	
	DocMDPDict->Append((unsigned char*)"TransformParams", trParams, Type::Dict);
	trParams->Append((unsigned char*)"Type", (unsigned char*)"TransformParams", Type::Name);
	int trParams_P=1;
	trParams->Append((unsigned char*)"P", &trParams_P, Type::Int);
	//trParams->Append((unsigned char*)"V", (unsigned char*)"1.2", Type::Name);
	DocMDPDict->Append((unsigned char*)"DigestMethod", (unsigned char*)DigestMethod, Type::Name);

	DocMDP->Append((unsigned char*)"Name", uSigner, Type::String);
	DocMDP->Append((unsigned char*)"M", dateString(), Type::String);

	
	Dictionary* Prop_Build=new Dictionary();
	DocMDP->Append((unsigned char*)"Prop_Build", Prop_Build, Type::Dict);
	Dictionary* Prop_Filter=new Dictionary();
	Prop_Build->Append((unsigned char*)"Filter", Prop_Filter, Type::Dict);
	Prop_Filter->Append((unsigned char*)"Name", (unsigned char*) "Adobe.PPKLite", Type::Name);
	
	// ToDo: Prop_Build?

	
	// /AcroForm (indirect dictionary) in document catalog dictionary	
	cout << "/AcroForm in the Document catalog dictionary" << endl;
	int AcroFormType;
	Indirect* AcroFormRef;
	void* AcroFormRefValue;
	void* AcroFormValue;
	Dictionary* AcroForm;
	bool needNew=false;
	if(dCatalog->Search((unsigned char*)"AcroForm")<0){
		needNew=true;
	}else{
		if(dCatalog->Read((unsigned char*)"AcroForm", (void**)&AcroFormRefValue, &AcroFormType)){
			if(AcroFormType==Type::Indirect){
				AcroFormRef=(Indirect*)AcroFormRefValue;
				if(PP.readRefObj(AcroFormRef, (void**)&AcroFormValue, &AcroFormType) && AcroFormType==Type::Dict){
					AcroForm=(Dictionary*)AcroFormValue;
				}else{
					cout << "Error in read AcroForm" << endl;
					return -1;
				}
			}else{
				cout << "/AcroForm is a direct dictionary, remake it" << endl;
				needNew=true;
			}
		}else{
			cout << "Error in read /AcroForm" << endl;
		}
	}
	if(needNew){
		cout << "Make new AcroForm dictionary" << endl;
		int AcroFormIndex=PP.ReferenceSize;
		AcroForm=new Dictionary();
		AcroFormRef=new Indirect();
		AcroFormRef->objNumber=AcroFormIndex;
		printf("Added object number: %d\n", AcroFormIndex);
		AcroFormRef->genNumber=0;
		AcroFormRef->used=true;
		AcroFormRef->type=Type::Dict;
		AcroFormRef->objStream=false;
		PP.ReferenceSize++;
		Indirect** Reference_new=new Indirect*[PP.ReferenceSize];
		for(i=0; i<PP.ReferenceSize-1; i++){
			Reference_new[i]=PP.Reference[i];
		}
		Reference_new[AcroFormIndex]=AcroFormRef;
		PP.Reference=Reference_new;
		
		dCatalog->Append((unsigned char*)"AcroForm", AcroFormRef, Type::Indirect);
	}

	Indirect* AcroForm_in_XRef=PP.Reference[AcroFormRef->objNumber];
	AcroForm_in_XRef->position=-1;
	AcroForm_in_XRef->value=AcroForm;
	AcroForm_in_XRef->type=Type::Dict;

	// Annots in page
	int pageNum=atoi(argv[5]);
	Dictionary* PD=PP.Pages[pageNum-1]->PageDict;
	int pageObjNum=PP.Pages[pageNum-1]->objNumber;
	Array* annotsArray;
	int annotsArrayType;
	void* annotsArrayValue;
	if(PD->Search((unsigned char*)"Annots")<0){
		printf("Annots does not exist in page %d, make it\n", pageNum);
		annotsArray=new Array();
		PD->Append((unsigned char*)"Annots", annotsArray, Type::Array);
	}else{
		if(PD->Read((unsigned char*)"Annots", (void**)&annotsArrayValue, &annotsArrayType) && annotsArrayType==Type::Array){
			annotsArray=(Array*)annotsArrayValue;
		}
	}
	PP.Reference[pageObjNum]->position=-1;
	PP.Reference[pageObjNum]->value=PD;
	PP.Reference[pageObjNum]->type=Type::Dict;
	printf("Page %d dictionary: %d\n", pageNum, pageObjNum);
	
	Dictionary* SigField=new Dictionary();
	
	SigField->Append((unsigned char*)"FT", (unsigned char*)"Sig", Type::Name);
	SigField->Append((unsigned char*)"T", uSigFieldName, Type::String);
	SigField->Append((unsigned char*)"V", DocMDPRef, Type::Indirect);

	// entries of Widget annotations
	SigField->Append((unsigned char*)"Type", (unsigned char*)"Annot", Type::Name);
	SigField->Append((unsigned char*)"Subtype", (unsigned char*)"Widget", Type::Name);
	int F_value=0b0000000010;
	SigField->Append((unsigned char*)"F", &F_value, Type::Int);
	Array* Sig_rect=new Array();
	int zero=0;
	for(i=0; i<4; i++){
		Sig_rect->Append(&zero, Type::Int);
	}
	SigField->Append((unsigned char*)"Rect", Sig_rect, Type::Array);

	
	int SigFieldIndex=PP.ReferenceSize;
	Indirect* SigFieldRef=new Indirect();
	SigFieldRef->objNumber=SigFieldIndex;
	SigFieldRef->genNumber=0;
	SigFieldRef->used=true;
	SigFieldRef->type=Type::Dict;
	SigFieldRef->objStream=false;
	PP.ReferenceSize++;
	Indirect** Reference_new=new Indirect*[PP.ReferenceSize];
	for(i=0; i<PP.ReferenceSize-1; i++){
		Reference_new[i]=PP.Reference[i];
	}
	Reference_new[SigFieldIndex]=SigFieldRef;
	PP.Reference=Reference_new;
	SigFieldRef->position=-1;
	SigFieldRef->value=SigField;
	
	annotsArray->Append(SigFieldRef, Type::Indirect);

	
	int fieldsIndex=AcroForm->Search((unsigned char*)"Fields");
	Array* fields;
	void* fieldsValue;
	int fieldsType;
	if(fieldsIndex>=0){
		if(AcroForm->Read((unsigned char*)"Fields", (void**)fieldsValue, &fieldsType) && fieldsType==Type::Array){
			fields=(Array*)fieldsValue;
		}else{
			cout << "Error in read /Fields" << endl;
			return -1;
		}
	}else{
		fields=new Array();
		AcroForm->Append((unsigned char*)"Fields", fields, Type::Array);
	}
	fields->Append(SigFieldRef, Type::Indirect);

	int SigFlagsIndex=AcroForm->Search((unsigned char*)"SigFlags");
	int SigFlagsValue=2; //AppendOnly
	if(SigFlagsIndex>=0){
		if(AcroForm->Update((unsigned char*)"SigFlags", &SigFlagsValue, Type::Int)){
			//ok
		}else{
			cout << "Error in updateing /SigFlags" << endl;
			return -1;
		}
	}else{
		AcroForm->Append((unsigned char*)"SigFlags", &SigFlagsValue, Type::Int);
		}
	
	// export
	PDFExporter PE(&PP);
	if(PE.exportToFile(argv[2],PP.encrypted)){
		cout << "Export succeeded" << endl;
	}else{
		cout << "Export failed" << endl;
	}
	cout << endl;

	int outputSize=(int)filesystem::file_size(argv[2]);
	printf("Output file size: %d\n", outputSize);
	
	// modify ByteRange
	int sigDictPos=DocMDP_in_XRef->position;
	printf("Signature dictionary (DocMDP) position in the output: %d\n",sigDictPos);

	ifstream* file_in=new ifstream(argv[2], ios::binary);
	file_in->seekg(sigDictPos,ios_base::beg);

	int ByteRangePos=findString(file_in, (char*)"/ByteRange");
	int LSBPos=findString(file_in, (char*)"[");
	int RSBPos=findString(file_in, (char*)"]");

	printf("/ByteRange @%d\n", ByteRangePos);
	printf("[ @%d\n", LSBPos);
	printf("] @%d\n", RSBPos);
	int ByteRangeLength=RSBPos-LSBPos-1;

	file_in->seekg(sigDictPos, ios_base::beg);
	int ContentsPos=findString(file_in, (char*)"/Contents");
	int LTPos=findString(file_in, (char*)"<");
	int RTPos=findString(file_in, (char*)">");
	printf("/Contents @%d\n", ContentsPos);
	printf("< @%d\n", LTPos);
	printf("> @%d\n", RTPos);

	// ByteRange is 0 - before <, after > - end (< and > are excluded);
	int ByteRange[4];
	ByteRange[0]=0;
	ByteRange[1]=LTPos;
	ByteRange[2]=RTPos+1;
	ByteRange[3]=outputSize-RTPos-1;
	int rangeSize=ByteRange[1]+ByteRange[3];
	printf("Data length for signature: %d\n", rangeSize);
	char ByteRange_c[ByteRangeLength+1];
	for(i=0; i<ByteRangeLength; i++){
		ByteRange_c[i]=' ';
	}
	ByteRange_c[ByteRangeLength]='\0';
	sprintf(ByteRange_c, "%d %d %d %d", ByteRange[0],ByteRange[1],ByteRange[2],ByteRange[3]);
	ByteRange_c[strlen(ByteRange_c)]=' ';
	printf("ByteRange: \"%s\"\n", ByteRange_c);

	char file_in_all[outputSize];
	file_in->seekg(0, ios_base::beg);
	file_in->read(file_in_all, outputSize);
	file_in->close();

	for(i=0; i<ByteRangeLength; i++){
		file_in_all[i+LSBPos+1]=ByteRange_c[i];
	}
	cout << "Modification of ByteRange finished" << endl;

	cout << "Calculate the signature" << endl;
	char file_for_sign[rangeSize];
	for(i=0; i<ByteRange[1]; i++){
		file_for_sign[i]=file_in_all[i];
	}
	for(i=0; i<ByteRange[3]; i++){
		file_for_sign[i+ByteRange[1]]=file_in_all[ByteRange[2]+i];
	}

	// private key
	printf("Private key from %s\n", argv[3]);
	BIO* mem=BIO_new(BIO_s_mem());
	ifstream file_pri(argv[3]);
	file_pri.seekg(0, ios_base::end);
	int fileSize_pri=file_pri.tellg();
	printf("File size: %d\n", fileSize_pri);
	file_pri.seekg(0, ios_base::beg);
	char data_pri[fileSize_pri];
	file_pri.read(data_pri, fileSize_pri);

	int bioResult=BIO_write(mem, data_pri, fileSize_pri);
	printf("BIO size: %d\n", bioResult);

	EVP_PKEY* private_key=PEM_read_bio_PrivateKey(mem, NULL, NULL, NULL);
	ERR_print_errors_fp(stderr);

	// data
	cout << "Read data to BIO" << endl;
	BIO* data_for_sign=BIO_new(BIO_s_mem());
	int bioResult2=BIO_write(data_for_sign, file_for_sign, rangeSize);
	printf("BIO size: %d\n", bioResult2);

	// public key (certificate)
	printf("Public key (certificate) from %s\n", argv[4]);
	ifstream file_pub(argv[4]);
	file_pub.seekg(0, ios_base::end);
	int fileSize_pub=file_pub.tellg();
	printf("File size: %d\n", fileSize_pub);
	file_pub.seekg(0, ios_base::beg);
	char data_pub[fileSize_pub];
	file_pub.read(data_pub, fileSize_pub);

	BIO* mem3=BIO_new(BIO_s_mem());
	int bioResult3=BIO_write(mem3, data_pub, fileSize_pub);
	printf("BIO size: %d\n", bioResult3);

	X509* cert=PEM_read_bio_X509(mem3, NULL, 0, NULL);
	if(cert==NULL){
		cout << "X509 failed" << endl;
	}

	// certificate chains
	STACK_OF(X509)* certs=NULL;
	if(argc>=7){
		certs=sk_X509_new_null();
		for(i=6; i<argc; i++){
			printf("Certificate chain from %s\n", argv[i]);
			ifstream file_chain(argv[i]);
			file_chain.seekg(0, ios_base::end);
			int fileSize_chain=file_chain.tellg();
			printf("File size: %d\n", fileSize_chain);
			file_chain.seekg(0, ios_base::beg);
			char data_chain[fileSize_chain];
			file_chain.read(data_chain, fileSize_chain);

			BIO* mem4=BIO_new(BIO_s_mem());
			int bioResult4=BIO_write(mem4, data_chain, fileSize_chain);
			printf("BIO size: %d\n", bioResult4);

			X509* chain=PEM_read_bio_X509(mem4, NULL, 0, NULL);
			if(chain==NULL){
				cout << "X509 failed" << endl;
				break;
			}
			int pushResult=sk_X509_push(certs, chain);
			if(pushResult<=0){
				cout << "Push failed" << endl;
				break;
			}else{
				printf("Number of chain elements: %d\n", pushResult);
			}
		}
	}
	
	CMS_ContentInfo* signed_data=CMS_sign(cert, private_key, certs, data_for_sign, CMS_DETACHED | CMS_BINARY | CMS_NOSMIMECAP);
	if(signed_data==NULL){
		cout << "Sign failed" << endl;
	}
	ERR_print_errors_fp(stderr);
	
	unsigned char* sign_binary=NULL;
	int sign_length=i2d_CMS_ContentInfo(signed_data, &sign_binary);
	printf("Sign length: %d\n", sign_length);

	ofstream file_bin("Signed_data.der", ios::binary);
	file_bin.write((char*)sign_binary, sign_length);
	file_bin.close();
	
	char sign_hex[sign_length*2+1];
	for(i=0; i<sign_length; i++){
		sprintf(&sign_hex[i*2], "%02x", sign_binary[i]);
	}
	if(strlen(sign_hex)>signBufferSize){
		cout << "Error: buffer was not enough" << endl;
		return -1;
	}
	for(i=0; i<sign_length*2; i++){
		file_in_all[LTPos+i+1]=sign_hex[i];
	}
  	
	
	ofstream* file_out=new ofstream(argv[2], ios::binary);
	file_out->write(file_in_all, outputSize);
	file_out->close();

	
	
}

int findString(ifstream* file, char* keyword){
	int currentPos=file->tellg();
	file->seekg(0, ios_base::end);
	int endPos=file->tellg();
	file->seekg(currentPos, ios_base::beg);

	int keylen=strlen(keyword);
	char a;
	int i;
	int keywordStartPos;
	while(currentPos<endPos){
		file->get(a);
		currentPos++;
		if(a==keyword[0]){
			bool flag=true;
			keywordStartPos=(int)file->tellg()-1;
			for(i=1; i<keylen; i++){
				file->get(a);
				currentPos++;
				if(a==keyword[i]){
					//ok
				}else{
					flag=false;
					break;
				}
			}
			if(flag==true){
				return keywordStartPos;
			}
		}
	}
	return -1;
}
