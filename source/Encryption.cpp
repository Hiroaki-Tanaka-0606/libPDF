// class Encryption

#include <iostream>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <gsasl.h>
#include <cstdlib>
#include <ctime>

#include "Encryption.hpp"

/*
OpenSSL
EVP_MD_CTX *EVP_MD_CTX_new(void);
int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl);
int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt);
int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);

int EVP_Digest(const void *data, size_t count, unsigned char *md,
                unsigned int *size, const EVP_MD *type, ENGINE *impl);


EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void);
int EVP_CIPHER_CTX_set_key_length(EVP_CIPHER_CTX *x, int keylen);
int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                        ENGINE *impl, const unsigned char *key, const unsigned char *iv);
int EVP_EncryptInit_ex2(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                         const unsigned char *key, const unsigned char *iv,
                         const OSSL_PARAM params[]);
int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                       int *outl, const unsigned char *in, int inl);
int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);

int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                       int *outl, const unsigned char *in, int inl);
 */

using namespace std;
Encryption::Encryption():
	error(false),
	Idn((unsigned char*)"Identity"),
	CFexist(false),
	encryptMeta(true),
	FEKObtained(false)
{
}
Encryption::Encryption(Encryption* original):
	encryptDict(original->encryptDict),
	V(original->V),
	error(original->error),
	Length(original->Length),
	Length_bytes(original->Length_bytes),
	CFexist(original->CFexist),
	CF(original->CF),
	StmF(original->StmF),
	StrF(original->StrF),
	Idn(original->Idn),
	R(original->R),
	O(original->O),
	U(original->U),
	OE(original->OE),
	UE(original->UE),
	Perms(original->Perms),
	FEK(original->FEK),
	FEKObtained(original->FEKObtained),
	encryptMeta(original->encryptMeta)
{
	int i=0;
	for(i=0; i<32; i++){
		P[i]=original->P[i];
	}
			
}
Encryption::Encryption(Dictionary* encrypt, Array* ID):
	// member initializer
	encryptDict(encrypt),
	error(false),
	Idn((unsigned char*)"Identity"),
	CFexist(false),
	encryptMeta(true),
	FEKObtained(false)
{
	OSSL_PROVIDER *prov_leg=OSSL_PROVIDER_load(NULL, "legacy");
	OSSL_PROVIDER *prov_def=OSSL_PROVIDER_load(NULL, "default");
	// initialize gsasl
	Gsasl *ctx=NULL;
	int rc;
	rc=gsasl_init(&ctx);
	if(rc!=GSASL_OK){
		printf("GSASL initialization error(%d): %s\n", rc, gsasl_strerror(rc));
		error=true;
		return;
	}
	
	int i, j;
	cout << "Read Encrypt information" << endl;
	// Filter: only /Standard can be processed in this program
	int filterType;
	void* filterValue;
	unsigned char* filter;
	if(encryptDict->Read((unsigned char*)"Filter", (void**)&filterValue, &filterType) && filterType==Type::Name){
		filter=(unsigned char*)filterValue;
		if(unsignedstrcmp(filter, (unsigned char*)"Standard")){
			cout << "Filter: Standard" << endl;
		}else{
			printf("Error: Filter %s is not supported\n", filter);
			error=true;
			return;
		}
	}else{
		cout << "Error in read /Filter" << endl;
		error=true;
		return;
	}
	// V: 1-5
	int VType;
	void* VValue;
	if(encryptDict->Read((unsigned char*)"V", (void**)&VValue, &VType) && VType==Type::Int){
		V=*((int*)VValue);
		if(V==1 || V==2 || V==4 || V==5){
			printf("V: %d\n", V);
		}else{
			printf("Error: V %d is invalid\n", V);
			error=true;
			return;
		}
	}else{
		cout << "Error in read /V " << endl;
		error=true;
		return;
	}
	// Length: valid if V==2 (default 40)
	// if V==1, Length=40 bits
	// if V==4, Length=128 bits
	// if V==5, Length=256 bits
	if(V==1){
		Length=40;
	}else if(V==2){
		Length=40;
		if(encryptDict->Search((unsigned char*)"Length")>=0){
			int lengthType;
			void* lengthValue;
			if(encryptDict->Read((unsigned char*)"Length", (void**)&lengthValue, &lengthType) && lengthType==Type::Int){
				Length=*((int*)lengthValue);
				if(Length%8!=0){
					printf("Error: Length %d is invalid\n", Length);
					error=true;
					return;
				}
			}else{
				cout << "Error in read /Length" << endl;
				error=true;
				return;
			}	
		}
	}else if(V==4){
		Length=128;
	}else if(V==5){
		Length=256;
	}
	Length_bytes=Length/8;
  printf("File encryption key length: %d bits = %d bytes\n", Length, Length_bytes);
	
	// CF, StmF, StrF
	if(V==4 || V==5){
		// CF
		if(encryptDict->Search((unsigned char*)"CF")>=0){
			int CFType;
			void* CFValue;
			CFexist=true;
			if(encryptDict->Read((unsigned char*)"CF", (void**)&CFValue, &CFType) && CFType==Type::Dict){
				CF=(Dictionary*)CFValue;
			}else{
				cout << "Error in read /CF" << endl;
				error=true;
				return;
			}
			printf("CF: %d entries\n", CF->getSize());
			CF->Print();
		}else{
			cout << "No CF" << endl;
		}
		// StmF
		if(encryptDict->Search((unsigned char*)"StmF")>=0){
			int StmFType;
			void* StmFValue;
			if(encryptDict->Read((unsigned char*)"StmF", (void**)&StmFValue, &StmFType) && StmFType==Type::Name){
				StmF=(unsigned char*)StmFValue;
			}else{
				cout << "Error in read /StmF" << endl;
				error=true;
				return;
			}
		}else{
			StmF=new unsigned char[unsignedstrlen(Idn)];
			unsignedstrcpy(StmF, Idn);
		}
		if(unsignedstrcmp(StmF, Idn) || (CFexist && CF->Search(StmF)>=0)){
			printf("StmF: %s\n", StmF);
		}else{
			printf("Error: StmF %s not found\n", StmF);
			error=true;
			return;
		}
		// StrF
		if(encryptDict->Search((unsigned char*)"StrF")>=0){
			int StrFType;
			void* StrFValue;
			if(encryptDict->Read((unsigned char*)"StrF", (void**)&StrFValue, &StrFType) && StrFType==Type::Name){
				StrF=(unsigned char*)StrFValue;
			}else{
				cout << "Error in read /StrF" << endl;
				error=true;
				return;
			}
		}else{
			StrF=new unsigned char[unsignedstrlen(Idn)];
			unsignedstrcpy(StrF, Idn);
		}
		if(unsignedstrcmp(StrF, Idn) || (CFexist && CF->Search(StrF)>=0)){
			printf("StrF: %s\n", StrF);
		}else{
			printf("Error: StrF %s not found\n", StrF);
			error=true;
			return;
		}
	}

	// R
	int RType;
	void* RValue;
	if(encryptDict->Read((unsigned char*)"R", (void**)&RValue, &RType) && RType==Type::Int){
		R=*((int*)RValue);
		if(2<=R && R<=6){
			printf("R: %d\n", R);
		}else{
			printf("Error: R %d is invalid\n", R);
			error=true;
			return;
		}
	}
	// O and U
	int OUType;
	void* OUValue;
	if(encryptDict->Read((unsigned char*)"O", (void**)&OUValue, &OUType) && OUType==Type::String){
		O=(uchar*)OUValue;
	}else{
		cout << "Error in read /O" << endl;
		error=true;
		return;
	}
	if(encryptDict->Read((unsigned char*)"U", (void**)&OUValue, &OUType) && OUType==Type::String){
		U=(uchar*)OUValue;
	}else{
		cout << "Error in read /U" << endl;
		error=true;
		return;
	}
	int size;
	size=O->length;
	cout << "O: ";
	for(i=0; i<size; i++){
		printf("%02x ", O->data[i]);
	}
	cout << endl;
	size=U->length;
	cout << "U: ";
	for(i=0; i<size; i++){
		printf("%02x ", U->data[i]);
	}
	cout << endl;
	// OE and UE
	if(R==6){
		if(encryptDict->Read((unsigned char*)"OE", (void**)&OUValue, &OUType) && OUType==Type::String){
			OE=(uchar*)OUValue;
		}else{
			cout << "Error in read /OE" << endl;
			error=true;
			return;
		}
		if(encryptDict->Read((unsigned char*)"UE", (void**)&OUValue, &OUType) && OUType==Type::String){
			UE=(uchar*)OUValue;
		}else{
			cout << "Error in read /UE" << endl;
			error=true;
			return;
		}
		int size;
		size=OE->length;
		cout << "OE: ";
		for(i=0; i<size; i++){
			printf("%02x ", OE->data[i]);
		}
		cout << endl;
		size=UE->length;
		cout << "UE: ";
		for(i=0; i<size; i++){
			printf("%02x ", U->data[i]);
		}
		cout << endl;
	}
	// P, bit position is from low-order to high-order
	int PType;
	void* PValue;
	unsigned int P_i;
	if(encryptDict->Read((unsigned char*)"P", (void**)&PValue, &PType) && PType==Type::Int){
		P_i=*((unsigned int*)PValue);
		for(i=0; i<32; i++){
			P[i]=((P_i>>i & 1)==1);
		}
	}else{
		cout << "Error in read P" << endl;
	}
	cout << "P (1->32): ";
	for(i=0; i<32; i++){
		printf("%1d", P[i] ? 1 : 0);
	}
	cout << endl;
	// Perms
	if(R==6){
		void* permsValue;
		int permsType;
		
		if(encryptDict->Read((unsigned char*)"Perms", (void**)&permsValue, &permsType) && permsType==Type::String){
			Perms=(uchar*)permsValue;
		}else{
			cout << "Error in read /Perms" << endl;
			error=true;
			return;
		}
		int size;
		size=Perms->length;
		cout << "Perms: ";
		for(i=0; i<size; i++){
			printf("%02x ", Perms->data[i]);
		}
		cout << endl;
	}
	// EncryptMetadata
	if(V==4 || V==5){
		if(encryptDict->Search((unsigned char*)"EncryptMetadata")>=0){
			void* encryptMetaValue;
			int encryptMetaType;
			if(encryptDict->Read((unsigned char*)"EncryptMetadata", (void**)&encryptMetaValue, &encryptMetaType) && encryptMetaType==Type::Bool){
				encryptMeta=*((bool*)encryptMetaValue);
			}
		}
		printf("EncryptMetadata: %s\n", encryptMeta ? "true": "false");
	}
	// ID
	void* IDValue;
	int IDType;
	for(i=0; i<2; i++){
		if(ID->Read(i, (void**)&IDValue, &IDType) && IDType==Type::String){
			IDs[i]=(uchar*)IDValue;
		}else{
			cout << "Error in read ID elements" << endl;
			error=true;
			return;
		}
		printf("ID[%d]: ", i);
		for(j=0; j<IDs[i]->length; j++){
			printf("%02x ", IDs[i]->data[j]);
		}
		cout << endl;
	}

	// Authentication test (user)
	cout << "Trial user authentication with no password" << endl;
	if(!AuthUser()){
		cout << "User authentication with no password failed" << endl;
		cout << "Enter user password" << endl;
		uchar* pwd_input=GetPassword();
		AuthUser(pwd_input);
	}

	// Authentication test (owner)
	cout << "Trial owner authentication with no password" << endl;
	if(!AuthOwner()){
		cout << "Owner authentication with no password failed" << endl;
		cout << "Enter owner password; enter something arbitrary to skip" << endl;
		uchar* pwd_input_o=GetPassword();
		if(pwd_input_o->length>0){
			AuthOwner(pwd_input_o);
		}else{
			cout << "Owner authentication skipped" << endl;
		}
	}
	
	if(R==6 && FEKObtained){
		// decrypt Perms
		int aescount;
		int result;
		EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
		unsigned char iv[16];
		uchar* Perms_decoded=new uchar();
		Perms_decoded->data=new unsigned char[16];
		for(i=0; i<16; i++){
			iv[i]='\0';
		}
		result=EVP_DecryptInit_ex2(aesctx, EVP_aes_256_cbc(), &(FEK->data[0]), &iv[0], NULL);
		if(result!=1){
			cout << "EVP_DecryptInit failed " << endl;
			error=true;
			return;
		}
		result=EVP_CIPHER_CTX_set_padding(aesctx, 0);
		if(result!=1){
			cout << "EVP_CIPHER_CTX_set_padding failed" << endl;
			error=true;
			return;
		}
		result=EVP_DecryptUpdate(aesctx, &(Perms_decoded->data[0]), &aescount, &(Perms->data[0]), 16);
		if(result!=1){
			cout << "EVP_DecryptUpdate failed" << endl;
		  error=true;
			return;
		}
		int aesfinalCount;
		result=EVP_DecryptFinal_ex(aesctx, &(Perms_decoded->data[aescount]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_DecryptFinal failed" << endl;
			ERR_print_errors_fp(stderr);
			error=true;
			return;
		}
		aescount+=aesfinalCount;

		bool P2[32];
		for(i=0; i<4; i++){
			for(j=0; j<8; j++){
				P2[i*8+j]=((Perms_decoded->data[i])>>(j))&1==1 ? true: false;
			}
		}
		
		cout << "Decoded Perms: ";
		for(i=0; i<32; i++){
			printf("%01d", P2[i]?1:0);
		}
		cout << endl;
		for(i=0; i<32; i++){
			if(P2[i]!=P[i]){
				cout << "Mismatch found in Ps" << endl;
				error=true;
				return;
			}
		}
		if(Perms_decoded->data[8]=='T'){
			if(encryptMeta){
				// ok
			}else{
				cout << "Mismatch found in encryptMeta" << endl;
				error=true;
				return;
			}
		}else if(Perms_decoded->data[8]=='F'){
			if(!encryptMeta){
				// ok
			}else{
				cout << "Mismatch found in encryptMeta" << endl;
				error=true;
				return;
			}
		}else{
			cout << "EncryptMetadata not ok" << endl;
			error=true;
			return;
		}
		cout << "EncryptMetadata ok" << endl;
		if(Perms_decoded->data[9]=='a' && Perms_decoded->data[10]=='d' && Perms_decoded->data[11]=='b'){
			cout << "'adb' ok" << endl;
		}else{
			cout << "'adb' not ok " << endl;
			error=true;
			return;
		}
	}
		
}

bool Encryption::DecryptString(uchar* str, int objNumber, int genNumber){
	if(!FEKObtained){
		cout << "Not yet authenticated" << endl;
		return false;
	}
	if(str->decrypted){
		// already decrypted
		return true;
	}
	// copy the original data to encrypted
	str->encrypted=new unsigned char[str->length+1];
	int i;
	for(i=0; i<str->length; i++){
		str->encrypted[i]=str->data[i];
	}
	str->encrypted[str->length]='\0';
	str->elength=str->length;

	if(unsignedstrcmp(StrF, Idn)){
		// Identity filter: do nothing
		str->decrypted=true;
		return true;
	}
	// use StrF
	Dictionary* filter;
	int filterType;
	void* filterValue;
	if(CF->Read(StrF, (void**)&filterValue, &filterType) && filterType==Type::Dict){
		filter=(Dictionary*)filterValue;
	}else{
		cout << "Error in read StrF value" << endl;
		return false;
	}

	unsigned char* CFM;
	int CFMType;
	void* CFMValue;
	if(filter->Read((unsigned char*)"CFM", (void**)&CFMValue, &CFMType) && CFMType==Type::Name){
		CFM=(unsigned char*)CFMValue;
	}else{
		cout << "Error in read CFM" << endl;
		return false;
	}


	if(ExecDecryption(&(str->encrypted), &(str->elength), &(str->data), &(str->length), CFM, objNumber, genNumber)){
		str->decrypted=true;
		return true;
	}else{
		cout << "Error in ExecDecryption" << endl;
		return false;
	}
} // end of DecryptString


bool Encryption::DecryptStream(Stream* stm){
	if(!FEKObtained){
		cout << "Not yet authenticated" << endl;
		return false;
	}
	if(stm->decrypted){
		// already decrypted
		return true;
	}
	unsigned char* type;
	int typeType;
	if(stm->StmDict.Read((unsigned char*)"Type", (void**)&type, &typeType) && typeType==Type::Name){
		// check whether the type is XRef
		if(unsignedstrcmp(type, (unsigned char*)"XRef")){
			// XRef is not encrypted
			stm->decrypted=true;
			return true;
		}
	}
	// copy the original data to encrypted
	stm->encrypted=new unsigned char[stm->length+1];
	int i;
	for(i=0; i<stm->length; i++){
		stm->encrypted[i]=stm->data[i];
	}
	stm->encrypted[stm->length]='\0';
	stm->elength=stm->length;

	if(unsignedstrcmp(StmF, Idn)){
		// Identity filter: do nothing
		stm->decrypted=true;
		return true;
	}
	// use StmF
	Dictionary* filter;
	int filterType;
	void* filterValue;
	if(CF->Read(StmF, (void**)&filterValue, &filterType) && filterType==Type::Dict){
		filter=(Dictionary*)filterValue;
	}else{
		cout << "Error in read StmF value" << endl;
		return false;
	}
	// filter->Print();

	unsigned char* CFM;
	int CFMType;
	void* CFMValue;
	if(filter->Read((unsigned char*)"CFM", (void**)&CFMValue, &CFMType) && CFMType==Type::Name){
		CFM=(unsigned char*)CFMValue;
		// printf("CFM: %s\n", CFM);
	}else{
		cout << "Error in read CFM" << endl;
		return false;
	}

	if(ExecDecryption(&(stm->encrypted), &(stm->elength), &(stm->data), &(stm->length), CFM, stm->objNumber, stm->genNumber)){
		stm->decrypted=true;
		return true;
	}else{
		cout << "Error in ExecDecryption" << endl;
		return false;
	}
}

bool Encryption::EncryptStream(Stream* stm){
	if(!FEKObtained){
		cout << "Not yet authenticated!" << endl;
		return false;
	}
	unsigned char* type;
	int typeType;
	// prepare buffer for encrypted data
	// slightly longer than original due to padding and iv (16)
	int padLength=16-(stm->length%16);
	stm->encrypted=new unsigned char[stm->length+padLength+17];
	int i;

	if(unsignedstrcmp(StmF, Idn)){
		// Identity filter: do nothing
		for(i=0; i<stm->length; i++){
			stm->encrypted[i]=stm->data[i];
		}
		stm->encrypted[stm->length]='\0';
		stm->elength=stm->length;
		return true;
	}
	// use StmF
	Dictionary* filter;
	int filterType;
	void* filterValue;
	if(CF->Read(StmF, (void**)&filterValue, &filterType) && filterType==Type::Dict){
		filter=(Dictionary*)filterValue;
	}else{
		cout << "Error in read StmF value" << endl;
		return false;
	}
	// filter->Print();

	unsigned char* CFM;
	int CFMType;
	void* CFMValue;
	if(filter->Read((unsigned char*)"CFM", (void**)&CFMValue, &CFMType) && CFMType==Type::Name){
		CFM=(unsigned char*)CFMValue;
		// printf("CFM: %s\n", CFM);
	}else{
		cout << "Error in read CFM" << endl;
		return false;
	}

	
	if(ExecEncryption(&(stm->encrypted), &(stm->elength), &(stm->data), &(stm->length), CFM, stm->objNumber, stm->genNumber)){
		return true;
	}else{
		cout << "Error in ExecEncryption" << endl;
		return false;
	}
	return true;
}


bool Encryption::EncryptString(uchar* str, int objNumber, int genNumber){
	if(!FEKObtained){
		cout << "Not yet authenticated?" << endl;
		return false;
	}
	// prepare buffer for encrypted data
	// slightly longer than original due to padding and iv (16)
	int padLength=16-(str->length%16);
	str->encrypted=new unsigned char[str->length+padLength+17];
	int i;

	if(unsignedstrcmp(StrF, Idn)){
		// Identity filter: do nothing
		for(i=0; i<str->length; i++){
			str->encrypted[i]=str->data[i];
		}
		str->encrypted[str->length]='\0';
		str->elength=str->length;
		return true;
	}
	// use StrF
	Dictionary* filter;
	int filterType;
	void* filterValue;
	if(CF->Read(StrF, (void**)&filterValue, &filterType) && filterType==Type::Dict){
		filter=(Dictionary*)filterValue;
	}else{
		cout << "Error in read StrF value" << endl;
		return false;
	}

	unsigned char* CFM;
	int CFMType;
	void* CFMValue;
	if(filter->Read((unsigned char*)"CFM", (void**)&CFMValue, &CFMType) && CFMType==Type::Name){
		CFM=(unsigned char*)CFMValue;
	}else{
		cout << "Error in read CFM" << endl;
		return false;
	}


	if(ExecEncryption(&(str->encrypted), &(str->elength), &(str->data), &(str->length), CFM, objNumber, genNumber)){
		str->decrypted=true;
		return true;
	}else{
		cout << "Error in ExecEncryption" << endl;
		return false;
	}
} // end of EncryptString

bool Encryption::ExecDecryption(unsigned char** encrypted, int* elength, unsigned char** decrypted, int* length, unsigned char* CFM, int objNumber, int genNumber){
	int i;
	if(!FEKObtained){
		cout << "Not yet authenticated" << endl;
		return false;
	}

	if(unsignedstrcmp(CFM, (unsigned char*)"V2")){
		unsigned char hashed_md5[16];
		int result;
		EVP_MD_CTX* md5ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(md5ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return false;
		}
		// FEK
		result=EVP_DigestUpdate(md5ctx, &(FEK->data[0]), Length_bytes);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in FEK" << endl;
			return false;
		}
		// object number (low-order byte first)
		int objNumber2=objNumber;
		unsigned char objNumber_c[3];
		for(i=0; i<3; i++){
			objNumber_c[i]=objNumber2%256;
			objNumber2=(objNumber2-objNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &objNumber_c[0], 3);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in objNumber" << endl;
			return false;
		}
		// gen number
		int genNumber2=genNumber;
		unsigned char genNumber_c[2];
		for(i=0; i<2; i++){
			genNumber_c[i]=genNumber2%256;
			genNumber2=(genNumber2-genNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &genNumber_c[0], 2);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in genNumber" << endl;
			return false;
		}
		// close the hash
		unsigned int count;
		result=EVP_DigestFinal_ex(md5ctx, &hashed_md5[0], &count);
		
		// key
		int key_length=max(Length_bytes+5, 16);
		unsigned char key[key_length];
		for(i=0; i<key_length; i++){
			key[i]=hashed_md5[i];
		}

		//decryption
		int rc4count;
		int rc4finalCount;
		EVP_CIPHER_CTX *rc4ctx=EVP_CIPHER_CTX_new();
		EVP_CIPHER* rc4=EVP_CIPHER_fetch(NULL, "RC4", "provider=legacy");
		result=EVP_DecryptInit_ex2(rc4ctx, rc4, key, NULL, NULL);
		if(result!=1){
			cout << "EVP_DecryptInit failed" << endl;
			return NULL;
		}
		result=EVP_CIPHER_CTX_set_key_length(rc4ctx, key_length*8);
		if(result!=1){
			cout << "EVP_CIPHER_CTX_set_key_length failed" << endl;
			return NULL;
		}
		result=EVP_DecryptUpdate(rc4ctx, &((*decrypted)[0]), &rc4count, &((*encrypted)[0]), *elength);
		if(result!=1){
			cout << "EVP_DecryptUpdate failed" << endl;
			return NULL;
		}
		result=EVP_DecryptFinal_ex(rc4ctx, &((*decrypted)[rc4count]), &rc4finalCount);
		if(result!=1){
			cout << "EVP_DecryptFinal failed" << endl;
			return NULL;
		}
		rc4count+=rc4finalCount;
			
		*length=rc4count;
	}else if(unsignedstrcmp(CFM, (unsigned char*)"AESV2")){
		unsigned char hashed_md5[16];
		int result;
		EVP_MD_CTX* md5ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(md5ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return false;
		}
		// FEK
		result=EVP_DigestUpdate(md5ctx, &(FEK->data[0]), Length_bytes);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in FEK" << endl;
			return false;
		}
		// object number (low-order byte first)
		int objNumber2=objNumber;
		unsigned char objNumber_c[3];
		for(i=0; i<3; i++){
			objNumber_c[i]=objNumber2%256;
			objNumber2=(objNumber2-objNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &objNumber_c[0], 3);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in objNumber" << endl;
			return false;
		}
		// gen number
		int genNumber2=genNumber;
		unsigned char genNumber_c[2];
		for(i=0; i<2; i++){
			genNumber_c[i]=genNumber2%256;
			genNumber2=(genNumber2-genNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &genNumber_c[0], 2);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in genNumber" << endl;
			return false;
		}
		// 'sAIT'
		unsigned char sAIT[4]={0x73, 0x41, 0x6c, 0x54};
		result=EVP_DigestUpdate(md5ctx, &sAIT[0], 4);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in sAIT" << endl;
			return false;
		}
		// close the hash
		unsigned int count;
		result=EVP_DigestFinal_ex(md5ctx, &hashed_md5[0], &count);

		// key and init vector
		unsigned char key[16];
		for(i=0; i<16; i++){
			key[i]=hashed_md5[i];
		}
		unsigned char iv[16];
		for(i=0; i<16; i++){
			iv[i]=(*encrypted)[i];
		}

		// AES, CBC mode decrypt
		int aescount;
		EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
		result=EVP_DecryptInit_ex2(aesctx, EVP_aes_128_cbc(), key, iv, NULL);
		if(result!=1){
			cout << "EVP_DecryptInit failed " << endl;
			return false;
		}
		result=EVP_DecryptUpdate(aesctx, &((*decrypted)[0]), &aescount, &((*encrypted)[16]), (*elength-16));
		if(result!=1){
			cout << "EVP_DecryptUpdate failed" << endl;
			return false;
		}
		int aesfinalCount;
		result=EVP_DecryptFinal_ex(aesctx, &((*decrypted)[aescount]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_DecryptFinal failed" << endl;
			ERR_print_errors_fp(stderr);
			// cout << aescount << endl;
			// cout << stm->length << endl;
			//return false;
		}
		aescount+=aesfinalCount;
		*length=aescount;
	}else if(unsignedstrcmp(CFM, (unsigned char*)"AESV3")){
		unsigned char iv[16];
		for(i=0; i<16; i++){
			iv[i]=(*encrypted)[i];
		}

		// AES, CBC mode decrypt
		int aescount;
		int result;
		EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
		result=EVP_DecryptInit_ex2(aesctx, EVP_aes_256_cbc(), &(FEK->data[0]), iv, NULL);
		if(result!=1){
			cout << "EVP_DecryptInit failed " << endl;
			return false;
		}
		result=EVP_DecryptUpdate(aesctx, &((*decrypted)[0]), &aescount, &((*encrypted)[16]), *elength-16);
		if(result!=1){
			cout << "EVP_DecryptUpdate failed" << endl;
			return false;
		}
		// cout << aescount << endl;
		int aesfinalCount;
		result=EVP_DecryptFinal_ex(aesctx, &((*decrypted)[aescount]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_DecryptFinal failed" << endl;
			ERR_print_errors_fp(stderr);
			//return false;
		}
		aescount+=aesfinalCount;
		*length=aescount;
		// printf("AESV3 length: %d\n", *length);
		// printf("Last character: %02x\n", (*decrypted)[aescount-1]);
	}
	return true;
}


bool Encryption::ExecEncryption(unsigned char** encrypted, int* elength, unsigned char** decrypted, int* length, unsigned char* CFM, int objNumber, int genNumber){
	// in case of AESV2 and AESV3, encrypted[0-15] includes the iv
	int i;
	if(!FEKObtained){
		cout << "Not yet authenticated" << endl;
		return false;
	}
	// note: padding is automatially added for AESV2 and AESV3
	if(unsignedstrcmp(CFM, (unsigned char*)"V2")){
		unsigned char hashed_md5[16];
		int result;
		EVP_MD_CTX* md5ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(md5ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return false;
		}
		// FEK
		result=EVP_DigestUpdate(md5ctx, &(FEK->data[0]), Length_bytes);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in FEK" << endl;
			return false;
		}
		// object number (low-order byte first)
		int objNumber2=objNumber;
		unsigned char objNumber_c[3];
		for(i=0; i<3; i++){
			objNumber_c[i]=objNumber2%256;
			objNumber2=(objNumber2-objNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &objNumber_c[0], 3);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in objNumber" << endl;
			return false;
		}
		// gen number
		int genNumber2=genNumber;
		unsigned char genNumber_c[2];
		for(i=0; i<2; i++){
			genNumber_c[i]=genNumber2%256;
			genNumber2=(genNumber2-genNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &genNumber_c[0], 2);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in genNumber" << endl;
			return false;
		}
		// close the hash
		unsigned int count;
		result=EVP_DigestFinal_ex(md5ctx, &hashed_md5[0], &count);
		
		// key
		int key_length=max(Length_bytes+5, 16);
		unsigned char key[key_length];
		for(i=0; i<key_length; i++){
			key[i]=hashed_md5[i];
		}

		//encryption
		int rc4count;
		int rc4finalCount;
		EVP_CIPHER_CTX *rc4ctx=EVP_CIPHER_CTX_new();
		EVP_CIPHER* rc4=EVP_CIPHER_fetch(NULL, "RC4", "provider=legacy");
		result=EVP_EncryptInit_ex2(rc4ctx, rc4, key, NULL, NULL);
		if(result!=1){
			cout << "EVP_EncryptInit failed" << endl;
			return NULL;
		}
		result=EVP_CIPHER_CTX_set_key_length(rc4ctx, key_length*8);
		if(result!=1){
			cout << "EVP_CIPHER_CTX_set_key_length failed" << endl;
			return NULL;
		}
		result=EVP_EncryptUpdate(rc4ctx, &((*encrypted)[0]), &rc4count, &((*decrypted)[0]), *length);
		if(result!=1){
			cout << "EVP_EncryptUpdate failed" << endl;
			return NULL;
		}
		result=EVP_EncryptFinal_ex(rc4ctx, &((*encrypted)[rc4count]), &rc4finalCount);
		if(result!=1){
			cout << "EVP_EncryptFinal failed" << endl;
			return NULL;
		}
		rc4count+=rc4finalCount;
			
		*elength=rc4count;
	}else if(unsignedstrcmp(CFM, (unsigned char*)"AESV2")){
		unsigned char hashed_md5[16];
		int result;
		EVP_MD_CTX* md5ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(md5ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return false;
		}
		// FEK
		result=EVP_DigestUpdate(md5ctx, &(FEK->data[0]), Length_bytes);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in FEK" << endl;
			return false;
		}
		// object number (low-order byte first)
		int objNumber2=objNumber;
		unsigned char objNumber_c[3];
		for(i=0; i<3; i++){
			objNumber_c[i]=objNumber2%256;
			objNumber2=(objNumber2-objNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &objNumber_c[0], 3);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in objNumber" << endl;
			return false;
		}
		// gen number
		int genNumber2=genNumber;
		unsigned char genNumber_c[2];
		for(i=0; i<2; i++){
			genNumber_c[i]=genNumber2%256;
			genNumber2=(genNumber2-genNumber2%256)/256;
		}
		result=EVP_DigestUpdate(md5ctx, &genNumber_c[0], 2);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in genNumber" << endl;
			return false;
		}
		// 'sAIT'
		unsigned char sAIT[4]={0x73, 0x41, 0x6c, 0x54};
		result=EVP_DigestUpdate(md5ctx, &sAIT[0], 4);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in sAIT" << endl;
			return false;
		}
		// close the hash
		unsigned int count;
		result=EVP_DigestFinal_ex(md5ctx, &hashed_md5[0], &count);

		// key and init vector
		unsigned char key[16];
		for(i=0; i<16; i++){
			key[i]=hashed_md5[i];
		}
		unsigned char iv[16];
		prepareIV(iv);
		for(i=0; i<16; i++){
			(*encrypted)[i]=iv[i];
		}

		// AES, CBC mode encrypt
		int aescount;
		EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
		result=EVP_EncryptInit_ex2(aesctx, EVP_aes_128_cbc(), key, iv, NULL);
		if(result!=1){
			cout << "EVP_EncryptInit failed " << endl;
			return false;
		}
		
		result=EVP_EncryptUpdate(aesctx, &((*encrypted)[16]), &aescount, &((*decrypted)[0]), *length);
		if(result!=1){
			cout << "EVP_EncryptUpdate failed" << endl;
			return false;
		}
		int aesfinalCount;
		result=EVP_EncryptFinal_ex(aesctx, &((*encrypted)[aescount+16]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_EncryptFinal failed" << endl;
			ERR_print_errors_fp(stderr);
			// cout << aescount << endl;
			// cout << stm->length << endl;
			//return false;
		}
		aescount+=aesfinalCount;
		*elength=aescount+16;
	}else if(unsignedstrcmp(CFM, (unsigned char*)"AESV3")){
		unsigned char iv[16];
		prepareIV(iv);
		for(i=0; i<16; i++){
			(*encrypted)[i]=iv[i];
		}

		// AES, CBC mode decrypt
		int aescount;
		int result;
		EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
		result=EVP_EncryptInit_ex2(aesctx, EVP_aes_256_cbc(), &(FEK->data[0]), iv, NULL);
		if(result!=1){
			cout << "EVP_EncryptInit failed " << endl;
			return false;
		}
		result=EVP_EncryptUpdate(aesctx, &((*encrypted)[16]), &aescount, &((*decrypted)[0]), *length);
		if(result!=1){
			cout << "EVP_EncryptUpdate failed" << endl;
			return false;
		}
		// cout << aescount << endl;
		int aesfinalCount;
		result=EVP_EncryptFinal_ex(aesctx, &((*encrypted)[aescount+16]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_EncryptFinal failed" << endl;
			ERR_print_errors_fp(stderr);
			//return false;
		}
		aescount+=aesfinalCount;
		*elength=aescount+16;
	}
	return true;
}

bool Encryption::AuthOwner(){
	if(R<=4){
		uchar* pwd=new uchar();
		pwd->length=32;
		int i;
		pwd->data=new unsigned char[33];
		for(i=0; i<32; i++){
			pwd->data[i]=PADDING[i];
		}
		pwd->data[32]='\0';
		return AuthOwner(pwd);
	}else if(R==6){
		uchar* pwd=new uchar();
		pwd->length=0;
		int i;
		pwd->data=new unsigned char[1];
		pwd->data[0]='\0';
		return AuthOwner(pwd);
	}
	return false;
}

bool Encryption::AuthOwner(uchar* pwd){
	int i;
	cout << "Trial password: ";
	for(i=0; i<pwd->length; i++){
		printf("%02x ", pwd->data[i]);
	}
	cout << endl;

	if(R<=4){
		uchar* RC4fek=RC4EncryptionKey(pwd);
		cout << "RC4 file encryption key: ";
		for(i=0; i<Length_bytes; i++){
			printf("%02x ", RC4fek->data[i]);
		}
		cout << endl;

		uchar* trialUserPassword=DecryptO(RC4fek);
		cout << "Trial user password (padded): ";
		for(i=0; i<trialUserPassword->length; i++){
			printf("%02x ", trialUserPassword->data[i]);
		}
		cout << endl;
		return AuthUser(trialUserPassword);
	}else if(R==6){
		uchar* tO=trialO6(pwd);
		cout << "Trial O: ";
		for(i=0; i<tO->length; i++){
			printf("%02x ", tO->data[i]);
		}
		cout << endl;
		for(i=0; i<tO->length; i++){
			if(tO->data[i]!=O->data[i]){
				cout << "Mismatch found in O and trial O" << endl;
				return false;
			}
		}
		cout << "O and trial O match" << endl;
		uchar* fek=fileEncryptionKey6(pwd, true);
		cout << "FEK: ";
		for(i=0; i<fek->length; i++){
			printf("%02x ", fek->data[i]);
		}
		cout << endl;
		FEK=fek;
		FEKObtained=true;
		return true;
	}
	return false;
}

bool Encryption::AuthUser(){
	if(R<=4){
		uchar* pwd=new uchar();
		pwd->length=32;
		int i;
		pwd->data=new unsigned char[33];
		for(i=0; i<32; i++){
			pwd->data[i]=PADDING[i];
		}
		pwd->data[32]='\0';
		return AuthUser(pwd);
	}else if(R==6){
		uchar* pwd=new uchar();
		pwd->length=0;
		int i;
		pwd->data=new unsigned char[1];
		pwd->data[0]='\0';
		return AuthUser(pwd);
	}
	return false;
}

bool Encryption::AuthUser(uchar* pwd){
	int i;
	cout << "Trial password: ";
	for(i=0; i<pwd->length; i++){
		printf("%02x ", pwd->data[i]);
	}
	cout << endl;
	if(R<=4){
		uchar* fek=fileEncryptionKey(pwd);
		cout << "Trial file encryption key: ";
		for(i=0; i<fek->length; i++){
			printf("%02x ", fek->data[i]);
		}
		cout << endl;
  
		uchar* tU=trialU(fek);
		cout << "Trial U: ";
		for(i=0; i<tU->length; i++){
			printf("%02x ", tU->data[i]);
		}
		cout << endl;

		for(i=0; i<tU->length; i++){
			if(tU->data[i]!=U->data[i]){
				cout << "Mismatch found in U and trial U" << endl;
				return false;
			}
		}
		cout << "U and trial U match" << endl;
		// save fileEncryptionKey
		FEK=fek;
		FEKObtained=true;
		return true;
	}else if(R==6){
		uchar* tU=trialU6(pwd);
		cout << "Trial U: ";
		for(i=0; i<tU->length; i++){
			printf("%02x ", tU->data[i]);
		}
		cout << endl;
		for(i=0; i<tU->length; i++){
			if(tU->data[i]!=U->data[i]){
				cout << "Mismatch found in U and trial U" << endl;
				return false;
			}
		}
		cout << "U and trial U match" << endl;
		uchar* fek=fileEncryptionKey6(pwd, false);
		cout << "FEK: ";
		for(i=0; i<fek->length; i++){
			printf("%02x ", fek->data[i]);
		}
		cout << endl;
		FEK=fek;
		FEKObtained=true;
		return true;
	}
	return false;
}

uchar* Encryption::trialU(uchar* fek){
	int i,j;
	if(R==2){
		
	}else if(R==3 || R==4){
		unsigned char hashed_md5[16];
		int result;
		EVP_MD_CTX* md5ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(md5ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return NULL;
		}
		// PADDING
		result=EVP_DigestUpdate(md5ctx, &PADDING[0], 32);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in PADDING" << endl;
			return NULL;
		}
		// ID[0]
		result=EVP_DigestUpdate(md5ctx, &(IDs[0]->data[0]), IDs[0]->length);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in ID[0]" << endl;
			return NULL;
		}
		// close
		unsigned int count;
		result=EVP_DigestFinal_ex(md5ctx, &hashed_md5[0], &count);
		
		// encrypt the hash with fek
		int rc4count;
		unsigned char encrypted_rc4[16];
		EVP_CIPHER_CTX *rc4ctx=EVP_CIPHER_CTX_new();
		EVP_CIPHER* rc4=EVP_CIPHER_fetch(NULL, "RC4", "provider=legacy");
		result=EVP_EncryptInit_ex2(rc4ctx, rc4, fek->data, NULL, NULL);
		if(result!=1){
			cout << "EVP_EncryptInit failed" << endl;
			return NULL;
		}
		result=EVP_CIPHER_CTX_set_key_length(rc4ctx, Length);
		if(result!=1){
			cout << "EVP_CIPHER_CTX_set_key_length failed" << endl;
			return NULL;
		}
		result=EVP_EncryptUpdate(rc4ctx, &encrypted_rc4[0], &rc4count, &hashed_md5[0], 16);
		if(result!=1){
			cout << "EVP_EncryptUpdate failed" << endl;
			return NULL;
		}
		int rc4finalCount;
		result=EVP_EncryptFinal_ex(rc4ctx, &(encrypted_rc4[rc4count]), &rc4finalCount);
		if(result!=1){
			cout << "EVP_EncryptFinal failed" << endl;
			return NULL;
		}
		rc4count+=rc4finalCount;
		printf("RC4 encrypted %d bytes: ", rc4count);
		for(i=0; i<rc4count; i++){
			printf("%02x ", encrypted_rc4[i]);
		}
		cout << endl;
		for(i=1; i<=19; i++){
			unsigned char unencrypted[16];
			unsigned char fek_i[Length_bytes];
			for(j=0; j<16; j++){
				unencrypted[j]=encrypted_rc4[j];
			}
			for(j=0; j<Length_bytes; j++){
				fek_i[j]=(fek->data[j])^((unsigned char)i);
			}
			/*
			printf("RC4 encryption key: ");
			for(j=0; j<Length_bytes; j++){
				printf("%02x ", fek_i[j]);
			}
			cout << endl;*/
			result=EVP_CIPHER_CTX_reset(rc4ctx);
			if(result!=1){
				cout << "EVP_CIPHER_CTX_reset failed" << endl;
				return NULL;
			}
			result=EVP_EncryptInit_ex2(rc4ctx, rc4, &fek_i[0], NULL, NULL);
			if(result!=1){
				cout << "EVP_EncryptInit failed" << endl;
				return NULL;
			}
			result=EVP_CIPHER_CTX_set_key_length(rc4ctx, Length);
			if(result!=1){
				cout << "EVP_CIPHER_CTX_set_key_length failed" << endl;
				return NULL;
			}
			result=EVP_EncryptUpdate(rc4ctx, &encrypted_rc4[0], &rc4count, &unencrypted[0], 16);
			if(result!=1){
				cout << "EVP_EncryptUpdate failed" << endl;
				return NULL;
			}
			rc4finalCount;
			result=EVP_EncryptFinal_ex(rc4ctx, &(encrypted_rc4[rc4count]), &rc4finalCount);
			if(result!=1){
				cout << "EVP_EncryptFinal failed" << endl;
				return NULL;
			}
			rc4count+=rc4finalCount;
			/*
			printf("RC4 encrypted %d bytes: ", rc4count);
			for(j=0; j<rc4count; j++){
				printf("%02x ", encrypted_rc4[j]);
			}
			cout << endl;*/
		}
		uchar* tU=new uchar();
		tU->data=new unsigned char[rc4count+1];
		for(i=0; i<rc4count; i++){
			tU->data[i]=encrypted_rc4[i];
		}
		tU->data[rc4count]='\0';
		tU->length=rc4count;
		return tU;
	}
	return NULL;
}


uchar* Encryption::trialU6(uchar* pwd){
	// saslprep
	int result;
	char* in=new char[pwd->length+1];
	int i;
	for(i=0; i<pwd->length; i++){
		in[i]=pwd->data[i];
	}
	in[pwd->length]='\0';
	char* out;
	int stringpreprc;
	result=gsasl_saslprep(&in[0], GSASL_ALLOW_UNASSIGNED, &out, &stringpreprc);
	if(result!=GSASL_OK){
		printf("GSASL SASLprep error(%d): %s\n", stringpreprc, gsasl_strerror(stringpreprc));
		return NULL;
	}
	/*
	cout << "SASLprep" << endl;
	cout << out << endl;
	i=0;
	cout << "SASLprep" << endl;
	while(true){
		if(out[i]!='\0'){
			printf("%02x ", out[i]);
			i++;
		}else{
			cout << endl;
			break;
		}
		}*/

	// concatenate pwd with User Validation Salt
	uchar* input=new uchar();
	input->length=strlen(out)+8;
	input->data=new unsigned char[input->length];
	for(i=0; i<strlen(out); i++){
		input->data[i]=out[i];
	}
	for(i=0; i<8; i++){
		input->data[strlen(out)+i]=U->data[32+i];
	}
	cout << "Input + User Validation Salt: ";
	for(i=0; i<input->length; i++){
		printf("%02x ", input->data[i]);
	}
	cout << endl;
		
	
	return Hash6(input, false, 8);
}

uchar* Encryption::Hash6(uchar* input, bool owner, int saltLength){
	uchar* K=new uchar();
	// K can be the hash of 32, 48, and 64 bytes
	K->data=new unsigned char[64];
	
	int result;
	unsigned int count;
	// K preparation (SHA-256)
	EVP_MD_CTX* ctx=EVP_MD_CTX_new();
	result=EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
	if(result!=1){
		cout << "EVP_DigestInit failed" << endl;
		return NULL;
	}
	result=EVP_DigestUpdate(ctx, &(input->data[0]), input->length);
	if(result!=1){
		cout << "EVP_DigestUpdate failed" << endl;
		return NULL;
	}
	result=EVP_DigestFinal_ex(ctx, &(K->data[0]), &count);
	if(result!=1){
		cout << "EVP_DigestFinal failed" << endl;
		return NULL;
	}
	K->length=32;

	int maxK1Length=input->length-saltLength+64;
	if(owner){
		maxK1Length+=48;
	}
	uchar* K1=new uchar();
	K1->data=new unsigned char[maxK1Length*64];
	uchar* E=new uchar();
	E->data=new unsigned char[maxK1Length*64];

	int round=0;
	bool okFlag=false;
	int i,j, K1Length;
	EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
	while(round<64 || okFlag==false){
		K1Length=input->length-saltLength+K->length;
		if(owner){
			K1Length+=48;
		}
		K1->length=K1Length*64;
		for(i=0; i<64; i++){
			for(j=0; j<input->length-saltLength; j++){
				K1->data[i*K1Length+j]=input->data[j];
			}
			for(j=0; j<K->length; j++){
				K1->data[i*K1Length+input->length-saltLength+j]=K->data[j];
			}
			if(owner){
				for(j=0; j<48; j++){
					K1->data[i*K1Length+input->length-saltLength+K->length+j]=U->data[j];
				}
			}
		}
		/*cout << "K1 length: " << K1->length << endl;
		
		for(i=0; i<K1->length; i++){
			printf("%02x ", K1->data[i]);
		}
		cout << endl;*/
		// AES, CBC mode encrypt
		int aescount;
		result=EVP_CIPHER_CTX_reset(aesctx);
		result=EVP_EncryptInit_ex2(aesctx, EVP_aes_128_cbc(), &(K->data[0]), &(K->data[16]), NULL);
		if(result!=1){
			cout << "EVP_EncryptInit failed " << endl;
			return NULL;
		}
		result=EVP_CIPHER_CTX_set_padding(aesctx, 0);
		if(result!=1){
			cout << "EVP_CIPHER_CTX_set_padding failed" << endl;
			return NULL;
		}
		result=EVP_EncryptUpdate(aesctx, &(E->data[0]), &aescount, &(K1->data[0]), K1->length);
		if(result!=1){
			cout << "EVP_EncryptUpdate failed" << endl;
			return NULL;
		}
		// cout << aescount << endl;
		int aesfinalCount;
		result=EVP_EncryptFinal_ex(aesctx, &(E->data[aescount]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_EncryptFinal failed" << endl;
			//return false;
		}
		aescount+=aesfinalCount;
		E->length=aescount;
		/*
		cout << "E length: " << E->length << endl;
		for(i=0; i<E->length; i++){
			printf("%02x ", (unsigned int)E->data[i]);
		}
		cout << endl;*/

		int remainder=0;
		for(i=0; i<16; i++){
			remainder*=256;
			remainder+=(unsigned int)E->data[i];
			remainder%=3;
		}
	  
		result=EVP_MD_CTX_reset(ctx);
		if(remainder==0){
			result=EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
			K->length=32;
		}else if(remainder==1){
			result=EVP_DigestInit_ex(ctx, EVP_sha384(), NULL);
			K->length=48;
		}else if(remainder==2){
			result=EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
			K->length=64;
		}else{
			cout << "Modulo error" << endl;
			return NULL;
		}
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return NULL;
		}
		result=EVP_DigestUpdate(ctx, &(E->data[0]), E->length);
		if(result!=1){
			cout << "EVP_DigestUpdate failed" << endl;
			return NULL;
		}
		result=EVP_DigestFinal_ex(ctx, &(K->data[0]), &count);
		if(result!=1){
			cout << "EVP_DigestFinal failed" << endl;
			return NULL;
		}
		// cout << count << endl;
		round++;
		/*
		printf("K in round %3d: ", round);
		for(i=0; i<K->length; i++){
			printf("%02x ", K->data[i]);
		}
		cout << endl;*/
		
		// cout << "E " << (unsigned int) E->data[E->length-1] << endl;
		if((unsigned int)E->data[E->length-1]>(round-32)){
			okFlag=false;
		}else{
			okFlag=true;
		}
	}
	K->length=32;							
	return K;
}



uchar* Encryption::trialO6(uchar* pwd){
	// saslprep
	int result;
	char* in=new char[pwd->length+1];
	int i;
	for(i=0; i<pwd->length; i++){
		in[i]=pwd->data[i];
	}
	in[pwd->length]='\0';
	char* out;
	int stringpreprc;
	result=gsasl_saslprep(&in[0], GSASL_ALLOW_UNASSIGNED, &out, &stringpreprc);
	if(result!=GSASL_OK){
		printf("GSASL SASLprep error(%d): %s\n", stringpreprc, gsasl_strerror(stringpreprc));
		return NULL;
	}
	/*
	cout << "SASLprep" << endl;
	cout << out << endl;
	i=0;
	cout << "SASLprep" << endl;
	while(true){
		if(out[i]!='\0'){
			printf("%02x ", out[i]);
			i++;
		}else{
			cout << endl;
			break;
		}
		}*/

	// concatenate pwd with Owner Validation Salt and U (48 bytes)
	uchar* input=new uchar();
	input->length=strlen(out)+56;
	input->data=new unsigned char[input->length];
	for(i=0; i<strlen(out); i++){
		input->data[i]=out[i];
	}
	for(i=0; i<8; i++){
		input->data[strlen(out)+i]=O->data[32+i];
	}
	for(i=0; i<48; i++){
		input->data[strlen(out)+8+i]=U->data[i];
	}
	cout << "Input + Owner Validation Salt + U: ";
	for(i=0; i<input->length; i++){
		printf("%02x ", input->data[i]);
	}
	cout << endl;
		
	
	return Hash6(input, true, 56);
}


uchar* Encryption::fileEncryptionKey(uchar* pwd){
	if(error){
		return NULL;
	}
	int i, j;
	if(R<=4){
		// MD5 (16 bytes) version
		int fromPwd=min(32, pwd->length);
		int fromPad=32-fromPwd;
		unsigned char paddedPwd[32];
		for(i=0; i<fromPwd; i++){
			paddedPwd[i]=pwd->data[i];
		}
		for(i=0; i<fromPad; i++){
			paddedPwd[i+fromPwd]=PADDING[i];
		}
		cout << "Padded password: ";
		for(i=0; i<32; i++){
			printf("%02x ", paddedPwd[i]);
		}
		cout << endl;

		unsigned char hashed_md5[16];
		int result;

		EVP_MD_CTX* ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return NULL;
		}
		// Padded password
		result=EVP_DigestUpdate(ctx, &paddedPwd[0], 32);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in padded password" << endl;
			return NULL;
		}
		// O
		result=EVP_DigestUpdate(ctx, &(O->data[0]), O->length);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in O" << endl;
			return NULL;
		}
		// P
		unsigned char p_c[4];
		p_c[0]=(P[7]?128:0)+(P[6]?64:0)+(P[5]?32:0)+(P[4]?16:0)+(P[3]?8:0)+(P[2]?4:0)+(P[1]?2:0)+(P[0]?1:0);
		p_c[1]=(P[15]?128:0)+(P[14]?64:0)+(P[13]?32:0)+(P[12]?16:0)+(P[11]?8:0)+(P[10]?4:0)+(P[9]?2:0)+(P[8]?1:0);
		p_c[2]=(P[23]?128:0)+(P[22]?64:0)+(P[21]?32:0)+(P[20]?16:0)+(P[19]?8:0)+(P[18]?4:0)+(P[17]?2:0)+(P[16]?1:0);
		p_c[3]=(P[31]?128:0)+(P[30]?64:0)+(P[29]?32:0)+(P[28]?16:0)+(P[27]?8:0)+(P[26]?4:0)+(P[25]?2:0)+(P[24]?1:0);
		// printf("%02x %02x %02x %02x\n", p_c[0], p_c[1], p_c[2], p_c[3]);
		result=EVP_DigestUpdate(ctx, &p_c[0], 4);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in P" << endl;
			return NULL;
		}
		// ID[0]
		result=EVP_DigestUpdate(ctx, &(IDs[0]->data[0]), IDs[0]->length);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in ID[0]" << endl;
			return NULL;
		}
		// 0xffffffff
		if(R>=4 && !encryptMeta){
			unsigned char meta[4]={255, 255, 255, 255};
			result=EVP_DigestUpdate(ctx, &meta[0], 4);
			if(result!=1){
				cout << "EVP_DigestUpdate failed in metadata" << endl;
				return NULL;
			}
		}
		// close the hash
		unsigned int count;
		result=EVP_DigestFinal_ex(ctx, &hashed_md5[0], &count);
		if(result!=1){
			cout << "EVP_DigestFinal failed" << endl;
			return NULL;
		}
		/*
		printf("%d bytes, ", count);
		for(i=0; i<count; i++){
			printf("%02x ", hashed_md5[i]);
		}
		cout << endl;*/
		if(R>=3){
			unsigned char hash_input[Length_bytes];
			result=EVP_MD_CTX_reset(ctx);
			if(result!=1){
				cout << "EVP_MD_CTX_reset failed" << endl;
				return NULL;
			}
			result=EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
			if(result!=1){
				cout << "EVP_DigestInit failed" << endl;
				return NULL;
			}
			for(i=0; i<50; i++){
				for(j=0; j<Length_bytes; j++){
					hash_input[j]=hashed_md5[j];
				}
				result=EVP_MD_CTX_reset(ctx);
				if(result!=1){
					cout << "EVP_MD_CTX_reset failed" << endl;
					return NULL;
				}
				result=EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
				if(result!=1){
					cout << "EVP_DigestInit failed" << endl;
					return NULL;
				}
				result=EVP_DigestUpdate(ctx, &hash_input[0], Length_bytes);
				if(result!=1){
					cout << "EVP_DigestUpdate failed in loop" << endl;
					return NULL;
				}
				result=EVP_DigestFinal_ex(ctx, &hashed_md5[0], &count);
				if(result!=1){
					cout << "EVP_DigestFinal failed in loop" << endl;
					return NULL;
				}
				/*
				printf("#%d: %d bytes, ", i, count);
				for(j=0; j<count; j++){
					printf("%02x ", hashed_md5[j]);
				}
				cout << endl;*/
			}
		}
		uchar* fek=new uchar();
		fek->data=new unsigned char[Length_bytes+1];
		for(i=0; i<Length_bytes; i++){
			fek->data[i]=hashed_md5[i];
		}
		fek->data[Length_bytes+1]='\0';
		fek->length=Length_bytes;
		return fek;
	}else if(R==6){
		
	}else{
		return NULL;
	}

	return NULL;
}

uchar* Encryption::fileEncryptionKey6(uchar* pwd, bool owner){
	// saslprep
	int result;
	char* in=new char[pwd->length+1];
	int i;
	for(i=0; i<pwd->length; i++){
		in[i]=pwd->data[i];
	}
	in[pwd->length]='\0';
	char* out;
	int stringpreprc;
	result=gsasl_saslprep(&in[0], GSASL_ALLOW_UNASSIGNED, &out, &stringpreprc);
	if(result!=GSASL_OK){
		printf("GSASL SASLprep error(%d): %s\n", stringpreprc, gsasl_strerror(stringpreprc));
		return NULL;
	}

	// user (owner=false): (pwd)(User key salt) -> hash -> decrypt UE
	// owner (owner=true): (pwd)(Owner key salt)(U) -> hash -> decrypt OE
	int inLength=strlen(out)+8;
	int saltLength=8;
	if(owner){
		inLength+=48;
		saltLength+=48;
	}
	uchar* input=new uchar();
	input->length=inLength;
	input->data=new unsigned char[inLength];
	for(i=0; i<strlen(out); i++){
		input->data[i]=out[i];
	}
	if(owner){
		for(i=0; i<8; i++){
			input->data[strlen(out)+i]=O->data[40+i];
		}
		for(i=0; i<48; i++){
			input->data[strlen(out)+8+i]=U->data[i];
		}
	}else{
		for(i=0; i<8; i++){
			input->data[strlen(out)+i]=U->data[40+i];
		}
	}
	uchar* key=Hash6(input, owner, saltLength);
	uchar* fek=new uchar();
	fek->length=32;
	fek->data=new unsigned char[32];
	// decrypt UE/OE AES-256 no padding, zero vector as iv
	unsigned char iv[16];
	for(i=0; i<16; i++){
		iv[i]='\0';
	}
	int aescount;
	EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
	result=EVP_DecryptInit_ex2(aesctx, EVP_aes_256_cbc(), &(key->data[0]), &iv[0], NULL);
	if(result!=1){
		cout << "EVP_DecryptInit failed " << endl;
		return NULL;
	}
	result=EVP_CIPHER_CTX_set_padding(aesctx, 0);
	if(result!=1){
		cout << "EVP_CIPHER_CTX_set_padding failed" << endl;
		return NULL;
	}
	if(owner){
		result=EVP_DecryptUpdate(aesctx, &(fek->data[0]), &aescount, &(OE->data[0]), OE->length);
	}else{
		result=EVP_DecryptUpdate(aesctx, &(fek->data[0]), &aescount, &(UE->data[0]), UE->length);
	}
	if(result!=1){
		cout << "EVP_DecryptUpdate failed" << endl;
		return NULL;
	}
	int aesfinalCount;
	result=EVP_DecryptFinal_ex(aesctx, &(fek->data[aescount]), &aesfinalCount);
	if(result!=1){
		cout << "EVP_EncryptFinal failed" << endl;
		//return false;
	}
	aescount+=aesfinalCount;
  
	return fek;
}

uchar* Encryption::encryptFEK6(uchar* pwd, bool owner){
	// saslprep
	int result;
	char* in=new char[pwd->length+1];
	int i;
	for(i=0; i<pwd->length; i++){
		in[i]=pwd->data[i];
	}
	in[pwd->length]='\0';
	char* out;
	int stringpreprc;
	result=gsasl_saslprep(&in[0], GSASL_ALLOW_UNASSIGNED, &out, &stringpreprc);
	if(result!=GSASL_OK){
		printf("GSASL SASLprep error(%d): %s\n", stringpreprc, gsasl_strerror(stringpreprc));
		return NULL;
	}

	// user (owner=false): (pwd)(User key salt) -> hash
	// owner (owner=true): (pwd)(Owner key salt)(U) -> hash
	int inLength=strlen(out)+8;
	int saltLength=8;
	if(owner){
		inLength+=48;
		saltLength+=48;
	}
	uchar* input=new uchar();
	input->length=inLength;
	input->data=new unsigned char[inLength];
	for(i=0; i<strlen(out); i++){
		input->data[i]=out[i];
	}
	if(owner){
		for(i=0; i<8; i++){
			input->data[strlen(out)+i]=O->data[40+i];
		}
		for(i=0; i<48; i++){
			input->data[strlen(out)+8+i]=U->data[i];
		}
	}else{
		for(i=0; i<8; i++){
			input->data[strlen(out)+i]=U->data[40+i];
		}
	}
	uchar* key=Hash6(input, owner, saltLength);
	uchar* encrypted_fek=new uchar();
	encrypted_fek->length=32;
	encrypted_fek->data=new unsigned char[32];
	// encrypt FEK by AES-256 no padding, zero vector as iv
	unsigned char iv[16];
	for(i=0; i<16; i++){
		iv[i]='\0';
	}
	int aescount;
	EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
	result=EVP_EncryptInit_ex2(aesctx, EVP_aes_256_cbc(), &(key->data[0]), &iv[0], NULL);
	if(result!=1){
		cout << "EVP_EncryptInit failed " << endl;
		return NULL;
	}
	result=EVP_CIPHER_CTX_set_padding(aesctx, 0);
	if(result!=1){
		cout << "EVP_CIPHER_CTX_set_padding failed" << endl;
		return NULL;
	}
	result=EVP_EncryptUpdate(aesctx, &(encrypted_fek->data[0]), &aescount, &(FEK->data[0]), FEK->length);
	if(result!=1){
		cout << "EVP_DecryptUpdate failed" << endl;
		return NULL;
	}
	int aesfinalCount;
	result=EVP_EncryptFinal_ex(aesctx, &(encrypted_fek->data[aescount]), &aesfinalCount);
	if(result!=1){
		cout << "EVP_EncryptFinal failed" << endl;
		//return false;
	}
	aescount+=aesfinalCount;

	return encrypted_fek;
}

uchar* Encryption::RC4EncryptionKey(uchar* pwd){
	if(error){
		return NULL;
	}
	int i, j;
	if(R<=4){
		// MD5 (16 bytes) version
		int fromPwd=min(32, pwd->length);
		int fromPad=32-fromPwd;
		unsigned char paddedPwd[32];
		for(i=0; i<fromPwd; i++){
			paddedPwd[i]=pwd->data[i];
		}
		for(i=0; i<fromPad; i++){
			paddedPwd[i+fromPwd]=PADDING[i];
		}
		cout << "Padded password: ";
		for(i=0; i<32; i++){
			printf("%02x ", paddedPwd[i]);
		}
		cout << endl;

		unsigned char hashed_md5[16];
		int result;

		EVP_MD_CTX* ctx=EVP_MD_CTX_new();
		result=EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
		if(result!=1){
			cout << "EVP_DigestInit failed" << endl;
			return NULL;
		}
		// Padded password
		result=EVP_DigestUpdate(ctx, &paddedPwd[0], 32);
		if(result!=1){
			cout << "EVP_DigestUpdate failed in padded password" << endl;
			return NULL;
		}

		// close the hash
		unsigned int count;
		result=EVP_DigestFinal_ex(ctx, &hashed_md5[0], &count);
		if(result!=1){
			cout << "EVP_DigestFinal failed" << endl;
			return NULL;
		}

		if(R>=3){
			unsigned char hash_input[16];
			result=EVP_MD_CTX_reset(ctx);
			if(result!=1){
				cout << "EVP_MD_CTX_reset failed" << endl;
				return NULL;
			}
			result=EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
			if(result!=1){
				cout << "EVP_DigestInit failed" << endl;
				return NULL;
			}
			for(i=0; i<50; i++){
				for(j=0; j<16; j++){
					hash_input[j]=hashed_md5[j];
				}
				result=EVP_MD_CTX_reset(ctx);
				if(result!=1){
					cout << "EVP_MD_CTX_reset failed" << endl;
					return NULL;
				}
				result=EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
				if(result!=1){
					cout << "EVP_DigestInit failed" << endl;
					return NULL;
				}
				result=EVP_DigestUpdate(ctx, &hash_input[0], 16);
				if(result!=1){
					cout << "EVP_DigestUpdate failed in loop" << endl;
					return NULL;
				}
				result=EVP_DigestFinal_ex(ctx, &hashed_md5[0], &count);
				if(result!=1){
					cout << "EVP_DigestFinal failed in loop" << endl;
					return NULL;
				}
				/*
				printf("#%d: %d bytes, ", i, count);
				for(j=0; j<count; j++){
					printf("%02x ", hashed_md5[j]);
				}
				cout << endl;*/
			}
		}
		uchar* fek=new uchar();
		fek->data=new unsigned char[Length_bytes+1];
		for(i=0; i<Length_bytes; i++){
			fek->data[i]=hashed_md5[i];
		}
		fek->data[Length_bytes+1]='\0';
		fek->length=Length_bytes;
		return fek;
	}else if(R==6){
		
	}else{
		return NULL;
	}

	return NULL;
}


uchar* Encryption::DecryptO(uchar* RC4fek){
	int i,j;
	if(R==2){
		
	}else if(R==3 || R==4){
		unsigned char encrypted_rc4[32];
		unsigned char unencrypted[32];
		for(i=0; i<32; i++){
			unencrypted[i]=O->data[i];
		}
		int result;
		int rc4count;
		int rc4finalCount;
		EVP_CIPHER_CTX *rc4ctx=EVP_CIPHER_CTX_new();
		EVP_CIPHER* rc4=EVP_CIPHER_fetch(NULL, "RC4", "provider=legacy");
		result=EVP_DecryptInit_ex2(rc4ctx, rc4, RC4fek->data, NULL, NULL);
		for(i=19; i>=0; i--){
			unsigned char fek_i[Length_bytes];
			for(j=0; j<32; j++){
				encrypted_rc4[j]=unencrypted[j];
			}
			for(j=0; j<Length_bytes; j++){
				fek_i[j]=(RC4fek->data[j])^((unsigned char)i);
			}
			/*
			printf("RC4 encryption key: ");
			for(j=0; j<Length_bytes; j++){
				printf("%02x ", fek_i[j]);
			}
			cout << endl;*/
			result=EVP_CIPHER_CTX_reset(rc4ctx);
			if(result!=1){
				cout << "EVP_CIPHER_CTX_reset failed" << endl;
				return NULL;
			}
			result=EVP_DecryptInit_ex2(rc4ctx, rc4, &fek_i[0], NULL, NULL);
			if(result!=1){
				cout << "EVP_DecryptInit failed" << endl;
				return NULL;
			}
			result=EVP_CIPHER_CTX_set_key_length(rc4ctx, Length);
			if(result!=1){
				cout << "EVP_CIPHER_CTX_set_key_length failed" << endl;
				return NULL;
			}
			result=EVP_DecryptUpdate(rc4ctx, &unencrypted[0], &rc4count, &encrypted_rc4[0], 32);
			if(result!=1){
				cout << "EVP_DecryptUpdate failed" << endl;
				return NULL;
			}
			result=EVP_DecryptFinal_ex(rc4ctx, &(unencrypted[rc4count]), &rc4finalCount);
			if(result!=1){
				cout << "EVP_DecryptFinal failed" << endl;
				return NULL;
			}
			rc4count+=rc4finalCount;
			/*
			printf("RC4 decrypted %d bytes: ", rc4count);
			for(j=0; j<rc4count; j++){
				printf("%02x ", unencrypted[j]);
			}
			cout << endl;*/
		}
		uchar* tUser=new uchar();
		tUser->data=new unsigned char[rc4count+1];
		for(i=0; i<rc4count; i++){
			tUser->data[i]=unencrypted[i];
		}
		tUser->data[rc4count]='\0';
		tUser->length=rc4count;
		return tUser;
	}
	return NULL;
}

void Encryption::EncryptO(unsigned char* paddedUserPwd, uchar* RC4fek){
	int i,j;
	if(R==2){
		
	}else if(R==3 || R==4){
		unsigned char encrypted_rc4[32];
		unsigned char unencrypted[32];
		for(i=0; i<32; i++){
			encrypted_rc4[i]=paddedUserPwd[i];
		}
		int result;
		int rc4count;
		int rc4finalCount;
		EVP_CIPHER_CTX *rc4ctx=EVP_CIPHER_CTX_new();
		EVP_CIPHER* rc4=EVP_CIPHER_fetch(NULL, "RC4", "provider=legacy");
		result=EVP_EncryptInit_ex2(rc4ctx, rc4, RC4fek->data, NULL, NULL);
		for(i=0; i<=19; i++){
			unsigned char fek_i[Length_bytes];
			for(j=0; j<32; j++){
				unencrypted[j]=encrypted_rc4[j];
			}
			for(j=0; j<Length_bytes; j++){
				fek_i[j]=(RC4fek->data[j])^((unsigned char)i);
			}
			result=EVP_CIPHER_CTX_reset(rc4ctx);
			if(result!=1){
				cout << "EVP_CIPHER_CTX_reset failed" << endl;
				return;
			}
			result=EVP_EncryptInit_ex2(rc4ctx, rc4, &fek_i[0], NULL, NULL);
			if(result!=1){
				cout << "EVP_EncryptInit failed" << endl;
				return;
			}
			result=EVP_CIPHER_CTX_set_key_length(rc4ctx, Length);
			if(result!=1){
				cout << "EVP_CIPHER_CTX_set_key_length failed" << endl;
				return;
			}
			result=EVP_EncryptUpdate(rc4ctx, &encrypted_rc4[0], &rc4count, &unencrypted[0], 32);
			if(result!=1){
				cout << "EVP_EncryptUpdate failed" << endl;
				return;
			}
			result=EVP_EncryptFinal_ex(rc4ctx, &(encrypted_rc4[rc4count]), &rc4finalCount);
			if(result!=1){
				cout << "EVP_EncryptFinal failed" << endl;
				return;
			}
			rc4count+=rc4finalCount;
		}
	  O=new uchar();
		O->data=new unsigned char[rc4count+1];
		for(i=0; i<rc4count; i++){
			O->data[i]=encrypted_rc4[i];
		}
		O->data[rc4count]='\0';
		O->length=rc4count;
	}
}



uchar* Encryption::GetPassword(){
	char pwd[64];
	struct termios term;
	struct termios save;
	tcgetattr(STDIN_FILENO, &term);
	save=term;
	// turn off echo
	term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	// password
	cin >> pwd;
	// turn off echo
	tcsetattr(STDIN_FILENO, TCSANOW, &save);

	uchar* pwdObj=new uchar();
	pwdObj->data=new unsigned char[strlen(pwd)+1];
	int i;
	for(i=0; i<strlen(pwd); i++){
		pwdObj->data[i]=pwd[i];
	}
	pwdObj->data[strlen(pwd)]='\0';
	pwdObj->length=strlen(pwd);
	return pwdObj;
}

void Encryption::prepareIV(unsigned char* iv){
	srand((unsigned int)time(NULL));
	int i;
	for(i=0; i<16; i++){
		int r=rand();
		iv[i]=(unsigned char)r%256;
	}
}

int Encryption::getV(){
	return V;
}


// (V=1 5 bytes, R=2)
// V=2 >5 bytes (set to 16 bytes), R=3
// V=4 16 bytes, R=4
// V=5 32 bytes, R=6
void Encryption::setV(int V_new){
	switch(V_new){
	case 1:
		V=1;
		R=2;
		Length=40;
		Length_bytes=5;
		break;
	case 2:
		V=2;
		R=3;
		Length=128;
		Length_bytes=16;
		break;
	case 4:
		V=4;
		R=4;
		Length=128;
		Length_bytes=16;
		break;
	case 5:
		V=5;
		R=6;
		Length=256;
		Length_bytes=32;
		break;
	default:
		cout << "setV error" << endl;
	}
}

void Encryption::setCFM(char* CFM_new){
	// set the CFM of StdCF to the given value
	CF=new Dictionary(CF);
	int StdCFIndex=CF->Search((unsigned char*)"StdCF");
	if(StdCFIndex>=0){
		CF->Delete(StdCFIndex);
	}
	Dictionary* StdCF=new Dictionary();
	StdCF->Append((unsigned char*)"CFM", (unsigned char*)CFM_new, Type::Name);
	StdCF->Append((unsigned char*)"Length", &Length, Type::Int);
	CF->Append((unsigned char*)"StdCF", StdCF, Type::Dict);
	
	// set "StdCF" to StmF and StrF
	StmF=(unsigned char*)"StdCF";
	StrF=(unsigned char*)"StdCF";
}


void Encryption::setPwd(Array* ID, uchar* userPwd, uchar* ownerPwd){
	// copy ID to IDs
	uchar* idValue;
	int idType;
	int i;
	int j;
	for(i=0; i<2; i++){
		if(ID->Read(i, (void**)&idValue, &idType) && idType==Type::String){
			IDs[i]=new uchar();
			IDs[i]->length=16;
			IDs[i]->data=new unsigned char[16];
			for(j=0; j<16; j++){
				IDs[i]->data[j]=idValue->data[j];
			}
		}
	}
	if(R<=4){
		// ownerPwd -> RC4fek
		uchar* RC4fek=RC4EncryptionKey(ownerPwd);

		// userPwd -> padded userPwd
		int fromPwd=min(32, userPwd->length);
		int fromPad=32-fromPwd;
		unsigned char paddedUserPwd[32];
		for(i=0; i<fromPwd; i++){
			paddedUserPwd[i]=userPwd->data[i];
		}
		for(i=0; i<fromPad; i++){
			paddedUserPwd[i+fromPwd]=PADDING[i];
		}

		// RC4fek, paddedUserPwd -> O
		EncryptO(paddedUserPwd, RC4fek);
		cout << "O: ";
		for(i=0; i<O->length; i++){
			printf("%02x", O->data[i]);
		}
		cout << endl;
		
		// userPwd -> FEK
		FEK=fileEncryptionKey(userPwd);
		cout << "FEK: ";
		for(i=0; i<FEK->length; i++){
			printf("%02x", FEK->data[i]);
		}
		cout << endl;

		// FEK -> U_short (16) + arbitrary (16) -> U
		uchar* U_short=trialU(FEK);
		U=new uchar();
		U->length=32;
		U->data=new unsigned char[32];
		for(i=0; i<U_short->length; i++){
			U->data[i]=U_short->data[i];
		}
		for(i=U_short->length; i<32; i++){
			U->data[i]=0;
		}
		cout << "U: ";
		for(i=0; i<U->length; i++){
			printf("%02x", U->data[i]);
		}
		cout << endl;
	}else if(R==6){
		// prepare FEK (32 bytes, random)
		FEK=new uchar();
		FEK->length=32;
		FEK->data=new unsigned char[32];
		srand((unsigned int)time(NULL));
		int i;
		for(i=0; i<32; i++){
			int r=rand();
			FEK->data[i]=(unsigned char)r%256;
		}

		// prepare salt parts of U and O
		U=new uchar();
		U->length=48;
		U->data=new unsigned char[48];
		O=new uchar();
		O->length=48;
		O->data=new unsigned char[48];
		for(i=0; i<16; i++){
			int r=rand();
			U->data[32+i]=(unsigned char)r%256;
			r=rand();
			O->data[32+i]=(unsigned char)r%256;
		}

		// U and UE
		// because hash for O and OE uses U
		uchar* tU=trialU6(userPwd);
		for(i=0; i<tU->length; i++){
			U->data[i]=tU->data[i];
		}

		UE=encryptFEK6(userPwd, false);

		cout << "U: ";
		for(i=0; i<U->length; i++){
			printf("%02x", U->data[i]);
		}
		cout << endl;
		cout << "UE: ";
		for(i=0; i<UE->length; i++){
			printf("%02x", UE->data[i]);
		}
		cout << endl;

		// O and OE
		uchar* tO=trialO6(ownerPwd);
		for(i=0; i<tO->length; i++){
			O->data[i]=tO->data[i];
		}
		OE=encryptFEK6(ownerPwd, true);
		
		cout << "O: ";
		for(i=0; i<O->length; i++){
			printf("%02x", O->data[i]);
		}
		cout << endl;
		cout << "OE: ";
		for(i=0; i<OE->length; i++){
			printf("%02x", OE->data[i]);
		}
		cout << endl;

		// Perms
		int aescount;
		int result;
		EVP_CIPHER_CTX *aesctx=EVP_CIPHER_CTX_new();
		unsigned char iv[16];
		uchar* Perms_decoded=new uchar();
		Perms_decoded->data=new unsigned char[16];
		Perms_decoded->length=16;
		for(i=0; i<16; i++){
			iv[i]='\0';
			Perms_decoded->data[i]='\0';
		}
		Perms=new uchar();
		Perms->data=new unsigned char[16];
		Perms->length=16;
		// prepare Perms_decoded
		// 0-3: from P
		int j;
		for(i=0; i<4; i++){
			for(j=0; j<8; j++){
				if(P[i*8+j]){
					Perms_decoded->data[i]+=(1<<j);
				}
			}
		}
		// 4-7: FF
		for(i=4; i<8; i++){
			Perms_decoded->data[i]=0xff;
		}
		// 8: encryptMeta
		Perms_decoded->data[8]=encryptMeta?'T':'F';
		// 9-11: adb
		Perms_decoded->data[9]='a';
		Perms_decoded->data[10]='d';
		Perms_decoded->data[11]='b';
		
		result=EVP_EncryptInit_ex2(aesctx, EVP_aes_256_cbc(), &(FEK->data[0]), &iv[0], NULL);
		if(result!=1){
			cout << "EVP_EncryptInit failed " << endl;
			error=true;
			return;
		}
		result=EVP_CIPHER_CTX_set_padding(aesctx, 0);
		if(result!=1){
			cout << "EVP_CIPHER_CTX_set_padding failed" << endl;
			error=true;
			return;
		}
		result=EVP_EncryptUpdate(aesctx, &(Perms->data[0]), &aescount, &(Perms_decoded->data[0]), 16);
		if(result!=1){
			cout << "EVP_EncryptUpdate failed" << endl;
		  error=true;
			return;
		}
		int aesfinalCount;
		result=EVP_EncryptFinal_ex(aesctx, &(Perms->data[aescount]), &aesfinalCount);
		if(result!=1){
			cout << "EVP_EncryptFinal failed" << endl;
			ERR_print_errors_fp(stderr);
			error=true;
			return;
		}
		aescount+=aesfinalCount;

		cout << "Perms (before encryption): ";
		for(i=0; i<Perms_decoded->length; i++){
			printf("%02x", Perms_decoded->data[i]);
		}
		cout << endl;
		cout << "Perms (encrypted): ";
		for(i=0; i<Perms->length; i++){
			printf("%02x", Perms->data[i]);
		}
		cout << endl;
	}
}

void Encryption::setP(bool* P_new){
	P[0]=false;
	P[1]=false;
	P[2]=P_new[0];
	P[3]=P_new[1];
	P[4]=P_new[2];
	P[5]=P_new[3];
	P[6]=true;
	P[7]=true;
	P[8]=P_new[4];
	P[9]=true;
	P[10]=P_new[5];
	P[11]=P_new[6];
	int i;
	for(i=12; i<32; i++){
		P[i]=true;
	}
}

Dictionary* Encryption::exportDict(){
	Dictionary* ret=new Dictionary();
	O->hex=true;
	U->hex=true;
	if(R==6){
		OE->hex=true;
		UE->hex=true;
		Perms->hex=true;
	}
	ret->Append((unsigned char*)"R", &R, Type::Int);
	ret->Append((unsigned char*)"O", O, Type::String);
	ret->Append((unsigned char*)"U", U, Type::String);
	if(R==6){
		ret->Append((unsigned char*)"OE", OE, Type::String);
		ret->Append((unsigned char*)"UE", UE, Type::String);
	}
	int* P_i=new int();
	*P_i=-1;
  int i;
	for(i=0; i<32; i++){
		if(P[i]==false){
			*P_i-=1<<i;
		}
	}
	ret->Append((unsigned char*)"P", P_i, Type::Int);
	if(R==6){
		ret->Append((unsigned char*)"Perms", Perms, Type::String);
	}
	ret->Append((unsigned char*)"EncryptMetaData", &encryptMeta, Type::Bool);
	ret->Append((unsigned char*)"Filter", (unsigned char*)"Standard", Type::Name);
	ret->Append((unsigned char*)"V", &V, Type::Int);
	ret->Append((unsigned char*)"Length", &Length, Type::Int);
	ret->Append((unsigned char*)"CF", CF, Type::Dict);
	ret->Append((unsigned char*)"StmF", StmF, Type::Name);
	ret->Append((unsigned char*)"StrF", StrF, Type::Name);
	return ret;
}

