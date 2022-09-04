// class Encryption

#ifndef INCLUDE_OBJECTS
#include "Objects.hpp"
#define INCLUDE_OBJECTS 1
#endif
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/err.h>

#define PADDING "\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A"

class Encryption{
private:
	Dictionary* encryptDict;
	bool error;
	int V;
	int Length;
	int Length_bytes;
	bool CFexist;
	Dictionary* CF;
	unsigned char* StmF;
	unsigned char* StrF;
	unsigned char* Idn;
	int R;
	uchar* O;
	uchar* U;
	uchar* OE;
	uchar* UE;
	uchar* Perms;
	uchar* FEK;
	bool FEKObtained;
	bool P[32];
	bool encryptMeta;
	uchar* fileEncryptionKey(uchar* pwd);
	uchar* fileEncryptionKey6(uchar* pwd, bool owner);
	uchar* IDs[2];
	uchar* trialU(uchar* fek);
	uchar* trialU6(uchar* pwd);
	uchar* trialO6(uchar* pwd);
	uchar* Hash6(uchar* input, bool owner, int saltLength);
	uchar* RC4EncryptionKey(uchar* pwd);
	uchar* DecryptO(uchar* RC4fek);
	bool ExecDecryption(unsigned char** encrypted, int* elength, unsigned char** decrypted, int* length, unsigned char* CFM, int objNumber, int genNumber);
	bool ExecEncryption(unsigned char** encrypted, int* elength, unsigned char** decrypted, int* length, unsigned char* CFM, int objNumber, int genNumber);
public:
	Encryption(Dictionary* encrypt, Array* ID);
	bool AuthUser(uchar* pwd);
	bool AuthUser();
	bool AuthOwner(uchar* pwd);
	bool AuthOwner();
	bool DecryptStream(Stream* stm);
	bool DecryptString(uchar* str, int objNumber, int genNumber);
	bool EncryptStream(Stream* stm);
	uchar* GetPassword();
};
