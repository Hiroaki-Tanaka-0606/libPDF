// class Encryption

#include <iostream>
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
 */

using namespace std;
Encryption::Encryption(Dictionary* encrypt, Array* ID):
	// member initializer
	encryptDict(encrypt),
	error(false),
	Idn((unsigned char*)"Identity"),
	CFexist(false),
	encryptMeta(true)
{
	OSSL_PROVIDER *prov_leg=OSSL_PROVIDER_load(NULL, "legacy");
	OSSL_PROVIDER *prov_def=OSSL_PROVIDER_load(NULL, "default");
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
	if(V==6){
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

	
	AuthUser();
}

bool Encryption::AuthUser(){
	uchar* pwd=new uchar();
	pwd->length=32;
	int i;
	pwd->data=new unsigned char[33];
	for(i=0; i<32; i++){
		pwd->data[i]=PADDING[i];
	}
	pwd->data[32]='\0';
	return AuthUser(pwd);
}
bool Encryption::AuthUser(uchar* pwd){
	int i;
	cout << "Trial password: ";
	for(i=0; i<pwd->length; i++){
		printf("%02x ", pwd->data[i]);
	}
	cout << endl;
	
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
	return true;
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

uchar* Encryption::fileEncryptionKey(uchar* pwd){
	if(error){
		return NULL;
	}
	int i, j;
	if(R<=4){
		// MD5 (16 bytes) version
		int fromPwd=max(32, pwd->length);
		int fromPad=32-fromPwd;
		unsigned char paddedPwd[32];
		for(i=0; i<fromPwd; i++){
			paddedPwd[i]=pwd->data[i];
		}
		for(i=0; i<fromPad; i++){
			paddedPwd[i+fromPwd]=PADDING[i];
		}

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
