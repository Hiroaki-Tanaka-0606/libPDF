// OpenSSL digest sample

/*
int EVP_Digest(const void *data, size_t count, unsigned char *md,
                unsigned int *size, const EVP_MD *type, ENGINE *impl);
 */
#include <cstring>
#include <iostream>
#include <openssl/evp.h>

using namespace std;
int main(int argc, char** argv){
	const EVP_MD *md=EVP_md5();

  unsigned char input[256];
	unsigned char output[256];

	input[0]='H';
	input[1]='E';
	input[2]='L';
	input[3]='L';
	input[4]='O';
	input[5]='\0';
	int length=5;
	
	cout << "Input: " << input << endl;
	unsigned int length2=0;

	int result=EVP_Digest(input, length, output, &length2, md, NULL);
	if(result==1){
		// OK
		cout << "Hash succeeded" << endl;
		cout << "Output: ";
		int i;
		for(i=0; i<length2; i++){
			printf("%02x", (int)output[i]);
		}
		cout << endl;
		cout << "Length: " << length2 << endl;
	}else{
		// not
		cout << "Hash failed" << endl;
	}
}
