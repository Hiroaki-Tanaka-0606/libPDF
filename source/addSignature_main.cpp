// read PDF executable

#include <fstream>
#include <iostream>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "PDFParser.hpp"
#include "PDFExporter.hpp"

using namespace std;
int main(int argc, char** argv){
	cout << "Add a digital signature" << endl;
	if(argc<4){
		printf("Usage: %s input private_key.pem output\n", argv[0]);
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

	// export
	PDFExporter PE(&PP);
	if(PE.exportToFile(argv[3],PP.encrypted)){
		cout << "Export succeeded" << endl;
	}else{
		cout << "Export failed" << endl;
	}	
}
