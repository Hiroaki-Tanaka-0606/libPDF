// read PDF executable

#include <fstream>
#include <iostream>
#include "PDFParser.hpp"

using namespace std;
int main(int argc, char** argv){
	cout << "Read PDF file" << endl;
	if(argc<2){
		printf("Usage: %s input\n", argv[0]);
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
			return -1;
		}
	}else{
		return -1;
	}
	cout << endl;
	
	// AcroForm
	cout << "AcroForm dictionary" << endl;
	int acroFormType;
	void* acroFormValue;
	Dictionary* acroForm;
	Indirect* acroFormRef;
	if(dCatalog->Read((unsigned char*)"AcroForm", (void**)&acroFormValue, &acroFormType)){
		if(acroFormType==Type::Dict){
			acroForm=(Dictionary*)acroFormValue;
		}else if(acroFormType==Type::Indirect){
			acroFormRef=(Indirect*)acroFormValue;
			if(PP.readRefObj(acroFormRef, (void**)&acroFormValue, &acroFormType) && acroFormType==Type::Dict){
				acroForm=(Dictionary*)acroFormValue;
			}else{
				cout << "Error in reading AcroForm ref" << endl;
				return -1;
			}
		}else{
			cout << "Error in AcroForm type" << endl;
			return -1;
		}
		acroForm->Print();
		if(acroForm->Search((unsigned char*)"Fields")>=0){
			Array* acroFormFields;
			void* acroFormFieldsValue;
			int acroFormFieldsType;
			if(acroForm->Read((unsigned char*)"Fields", (void**)&acroFormFieldsValue, &acroFormFieldsType) && acroFormFieldsType==Type::Array){
				acroFormFields=(Array*)acroFormFieldsValue;
				int numFields=acroFormFields->getSize();
				for(int i=0; i<numFields; i++){
					Indirect* acroFormFieldRef;
					void* acroFormFieldRefValue;
					int acroFormFieldType;
					if(acroFormFields->Read(i, (void**)&acroFormFieldRefValue, &acroFormFieldType) && acroFormFieldType==Type::Indirect){
						acroFormFieldRef=(Indirect*)acroFormFieldRefValue;
						Dictionary* acroFormField;
						if(PP.readRefObj(acroFormFieldRef, (void**)&acroFormFieldRefValue, &acroFormFieldType) && acroFormFieldType==Type::Dict){
							acroFormField=(Dictionary*)acroFormFieldRefValue;
							acroFormField->Print();
						}
					}
				}
			}
			
		}
		cout << endl;
	}else{
		cout << "No AcroFrom" << endl;
	}
	cout << endl;

	// Annots and size in each page
	cout << "Page information" << endl;
	int i;
	for(i=0; i<PP.PagesSize; i++){
		printf("----    Page %d    ----\n", (i+1));
		Page* p=PP.Pages[i];
		void* UserUnitValue;
		int UserUnitType;
		double UserUnit;
		if(PP.readPage(i, (unsigned char*)"UserUnit", (void**)&UserUnitValue, &UserUnitType, true)&& (UserUnitType==Type::Real || UserUnitType==Type::Int)){
			if(UserUnitType==Type::Real){
				UserUnit=*((double*)UserUnitValue);
			}else{
				UserUnit=(double)(*((int*)UserUnitValue));
			}
		}else{
			UserUnit=1.0;
		}
		void* MediaBoxValue;
		int MediaBoxType;
		Array* MediaBox;
		if(PP.readPage(i, (unsigned char*)"MediaBox", (void**)&MediaBoxValue, &MediaBoxType, true) && MediaBoxType==Type::Array){
			MediaBox=(Array*)MediaBoxValue;
			cout << "MediaBox:" << endl;
			MediaBox->Print();
		}
		void* AnnotsValue;
		int AnnotsType;
		Array* Annots;
		if(PP.readPage(i, (unsigned char*)"Annots", (void**)&AnnotsValue, &AnnotsType, true)){
			if(AnnotsType==Type::Array){
				Annots=(Array*)AnnotsValue;
				cout << "Annots(Direct):" << endl;
				Annots->Print();
			}else if(AnnotsType==Type::Indirect){
				Indirect* AnnotsRef=(Indirect*)AnnotsValue;
				if(PP.readRefObj(AnnotsRef, (void**)&AnnotsValue, &AnnotsType) && AnnotsType==Type::Array){
					Annots=(Array*)AnnotsValue;
					cout << "Annots(Indirect):" << endl;
				  int annotsSize=Annots->getSize();
					int j;
					for(j=0; j<annotsSize; j++){
						printf("  --    Annot %d    --\n", (j+1));
						void* AnnotValue;
						int AnnotType;
						Dictionary* Annot;
						if(Annots->Read(j, (void**)&AnnotValue, &AnnotType)){
							if(AnnotType==Type::Indirect){
								Indirect* AnnotRef=(Indirect*)AnnotValue;
								if(PP.readRefObj(AnnotRef, (void**)&AnnotValue, &AnnotType) && AnnotType==Type::Dict){
									Annot=(Dictionary*)AnnotValue;
								}else{
									cout << "Error in read Annot reference" << endl;
									return -1;
								}
							}else if(AnnotType==Type::Dict){
								Annot=(Dictionary*)AnnotValue;
							}else{
								cout << "Annot is not a dictionary" << endl;
								return -1;
							}
							cout << "  ----" << endl;
							Annot->Print();
							cout << endl;
						}
					}
				}
			}
		}else{
			cout << "No Annots" << endl;
		}
		
		void* contentsValue;
		int contentsType;
		Indirect* contentsRef;
		if(PP.readPage(i, (unsigned char*)"Contents", (void**)&contentsValue, &contentsType, false)){
			if(contentsType==Type::Array){
				Array* contentsArray=(Array*)contentsValue;
				int contentsSize=contentsArray->getSize();
				int j;
				for(j=0; j<contentsSize; j++){
					void* contentValue;
					int contentType;
					Indirect* contentRef;
					Stream* content;
					if(contentsArray->Read(j, (void**)&contentValue, &contentType) && contentType==Type::Indirect){
						contentRef=(Indirect*)contentValue;
						if(PP.readRefObj(contentRef, (void**)&contentValue, &contentType) && contentType==Type::Stream){
							Stream* content=(Stream*)contentValue;
							printf("----\n%s\n----\n", content->decoded);
						}
					}
				}
			}else if(contentsType==Type::Indirect){
				contentsRef=(Indirect*)contentsValue;
				if(PP.readRefObj(contentsRef, (void**)&contentsValue, &contentsType)){
					if(contentsType==Type::Stream){
						Stream* content=(Stream*)contentsValue;
						printf("----\n%s\n----\n", content->decoded);
					}else if(contentsType==Type::Array){
						Array* contentsArray=(Array*)contentsValue;
						int contentsSize=contentsArray->getSize();
						int j;
						for(j=0; j<contentsSize; j++){
							void* contentValue;
							int contentType;
							Indirect* contentRef;
							Stream* content;
							if(contentsArray->Read(j, (void**)&contentValue, &contentType) && contentType==Type::Indirect){
								contentRef=(Indirect*)contentValue;
								if(PP.readRefObj(contentRef, (void**)&contentValue, &contentType) && contentType==Type::Stream){
									Stream* content=(Stream*)contentValue;
									printf("----\n%s\n----\n", content->decoded);
								}
							}
						}
					}
				}
			}
			
			}
		cout << endl;
	}
	
	return 0;
	
}
