// read PDF executable

#include <fstream>
#include <iostream>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "PDFParser.hpp"

using namespace std;
int main(int argc, char** argv){
	cout << "Verify PDF sign" << endl;
	if(argc<2){
		printf("Usage: %s input (certificate.pem)\n", argv[0]);
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
	int PermsType;
	void* PermsValue;
	Dictionary* Perms;
	if(dCatalog->Search((unsigned char*)"Perms")<0){
		cout << "/Perms not found in document catalog dictionary" << endl;
		return -1;
	}
	if(dCatalog->Read((unsigned char*)"Perms", (void**)&PermsValue, &PermsType) && PermsType==Type::Dict){
		Perms=(Dictionary*)PermsValue;
	}else{
		cout << "Error in read /Perms" << endl;
	}
	cout << "Permissions dictionary:" << endl;
	Perms->Print();

	// DocMDP
	int DocMDPType;
	void* DocMDPRef;
	void* DocMDPValue;
	Dictionary* DocMDP;
	if(Perms->Search((unsigned char*)"DocMDP")<0){
		cout << "/DocMDP not found in permissions dictionary" << endl;
		return -1;
	}
	if(Perms->Read((unsigned char*)"DocMDP", (void**)&DocMDPRef, &DocMDPType) && DocMDPType==Type::Indirect){
		if(PP.readRefObj((Indirect*)DocMDPRef, (void**)&DocMDPValue, &DocMDPType) && DocMDPType==Type::Dict){
			DocMDP=(Dictionary*)DocMDPValue;
		}else{
			cout << "Error in read indirect object" << endl;
			return -1;
		}
	}else{
		cout << "Error in read /DocMDP in permissions dictionary" << endl;
	}
	cout << "Signature dictionary (DocMDP):" << endl;
	DocMDP->Print();

	// ByteRange
	int ByteRangeType;
	void* ByteRangeValue;
	Array* ByteRange;
	int ByteRangeSize;
	if(DocMDP->Search((unsigned char*)"ByteRange")<0){
		cout << "/ByteRange not found in signature dictionary" << endl;
		return -1;
	}
	if(DocMDP->Read((unsigned char*)"ByteRange", (void**)&ByteRangeValue, &ByteRangeType) && ByteRangeType==Type::Array){
		ByteRange=(Array*)ByteRangeValue;
		ByteRangeSize=ByteRange->getSize();
		if(ByteRangeSize%2!=0){
			cout << "Error: ByteRange has odd number of elements" << endl;
			return -1;
		}
	}else{
		cout << "Error in read /ByteRange in signature dictionary" << endl;
	}

	int Range_offsets[ByteRangeSize/2];
	int Range_lengths[ByteRangeSize/2];
	int i;
	int ByteRangeElementType;
	int* ByteRangeElementValue;
	for(i=0; i<ByteRangeSize/2; i++){
		if(ByteRange->Read(2*i, (void**)&ByteRangeElementValue, &ByteRangeElementType) && ByteRangeElementType==Type::Int){
			Range_offsets[i]=*ByteRangeElementValue;
		}else{
			cout << "Error in an element of ByteRange" << endl;
			return -1;
		}
		if(ByteRange->Read(2*i+1, (void**)&ByteRangeElementValue, &ByteRangeElementType) && ByteRangeElementType==Type::Int){
			Range_lengths[i]=*ByteRangeElementValue;
		}else{
			cout << "Error in an element of ByteRange" << endl;
			return -1;
		}
	}
	cout << "Range used in sign" << endl;
	int rangeSum=0;
	for(i=0; i<ByteRangeSize/2; i++){
		printf("Offset: %d, Length: %d\n", Range_offsets[i], Range_lengths[i]);
		rangeSum+=Range_lengths[i];
	}
	printf("Total length: %d\n", rangeSum);

	// Contents
	int ContentsType;
	void* ContentsValue;
	uchar* Contents;
	if(DocMDP->Search((unsigned char*)"Contents")<0){
		cout << "/Contents not found in signature dictionary" << endl;
		return -1;
	}
	if(DocMDP->Read((unsigned char*)"Contents", (void**)&ContentsValue, &ContentsType) && ContentsType==Type::String){
		Contents=(uchar*)ContentsValue;
	}else{
		cout << "Error in raed /Contents" << endl;
	}
	cout << "Contents (first 32 bytes):" << endl;
	for(i=0; i<32; i++){
		printf("%02x ", Contents->data[i]);
	}
	cout << endl;

	// Get data specified by Range_offsets and Range_lengths
	char sign_range[rangeSum];
	int index=0;
	for(i=0; i<ByteRangeSize/2; i++){
		PP.file.seekg(Range_offsets[i], ios_base::beg);
		PP.file.read(&sign_range[index], Range_lengths[i]);
		index+=Range_lengths[i];
	}

	int bioResult;
	BIO* signature_data=BIO_new(BIO_s_mem());
	if(PP.encrypted){
		bioResult=BIO_write(signature_data, (char*)Contents->encrypted, Contents->elength);
	}else{
		bioResult=BIO_write(signature_data, (char*)Contents->data, Contents->length);
	}
	printf("Signature size: %d\n", bioResult);
	CMS_ContentInfo* signature=d2i_CMS_bio(signature_data, NULL);
	if(signature==NULL){
		cout << "d2i_CMS_bio failed" << endl;
		return -1;
	}
	int setResult=CMS_set1_eContentType(signature, OBJ_nid2obj(NID_pkcs7_signed));
	printf("Set eContentType result: %d\n", setResult);

	BIO* file_data=BIO_new(BIO_s_mem());
	bioResult=BIO_write(file_data, sign_range, rangeSum);
	printf("File size (used for sign): %d\n", bioResult);


	// argv[2] is certificate file (if exists)
	bool useCerts=false;
	X509_STORE* certStore=X509_STORE_new();
	STACK_OF(X509)* certs=NULL;
	if(argc>=3){
		ifstream certFile(argv[2]);
		certFile.seekg(0, ios_base::end);
		int certSize=certFile.tellg();
		certFile.seekg(0, ios_base::beg);
		char cert_text[certSize];
		certFile.read(cert_text, certSize);
		BIO* cert_data=BIO_new(BIO_s_mem());
		bioResult=BIO_write(cert_data, cert_text, certSize);
		printf("Certificate size: %d\n", bioResult);
		X509* cert=PEM_read_bio_X509(cert_data, NULL, 0, NULL);
		if(cert==NULL){
			cout << "Read X509 failed" << endl;
			return -1;
		}
		int addResult=X509_STORE_add_cert(certStore, cert);
		printf("Add result: %d\n", addResult);
		int setResult=X509_STORE_set_purpose(certStore, X509_PURPOSE_ANY);
		printf("Set result: %d\n", setResult);
		useCerts=true;
		if(argc>=4){
			certs=sk_X509_new_null();
			for(i=3; i<argc; i++){
				ifstream certFile(argv[i]);
				certFile.seekg(0, ios_base::end);
				int certSize=certFile.tellg();
				certFile.seekg(0, ios_base::beg);
				char cert_text[certSize];
				certFile.read(cert_text, certSize);
				BIO* cert_data=BIO_new(BIO_s_mem());
				bioResult=BIO_write(cert_data, cert_text, certSize);
				printf("Certificate size: %d\n", bioResult);
				X509* chain=PEM_read_bio_X509(cert_data, NULL, 0, NULL);
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
	}

	ERR_print_errors_fp(stderr);
	int verifyResult;
	verifyResult=CMS_verify(signature, NULL, NULL, file_data, NULL, CMS_BINARY | CMS_NO_SIGNER_CERT_VERIFY);
	printf("Verify no cert, no cert_verify: %d\n", verifyResult);
	ERR_print_errors_fp(stderr);
	verifyResult=CMS_verify(signature, NULL, NULL, file_data, NULL, CMS_BINARY);
	printf("Verify no cert, cert_verify: %d\n", verifyResult);
	ERR_print_errors_fp(stderr);
	if(useCerts){
		verifyResult=CMS_verify(signature, certs, certStore, file_data, NULL, CMS_BINARY);
		printf("Verify cert, cert_verify: %d\n", verifyResult);
		ERR_print_errors_fp(stderr);
	}

	STACK_OF(X509)* signers_sk=CMS_get0_signers(signature);
	int numSigners=sk_X509_num(signers_sk);
	printf("Number of signers: %d\n", numSigners);
	char signerFile_buffer[128];
	for(i=0; i<numSigners; i++){
		X509* signer=sk_X509_value(signers_sk, i);
		sprintf(signerFile_buffer, "Signer_%d.der", i);
		FILE* signerFile=fopen(signerFile_buffer, "w");
		int exportResult=i2d_X509_fp(signerFile, signer);
		printf("Export result for signer %d: %d\n", i, exportResult);
	}
}
