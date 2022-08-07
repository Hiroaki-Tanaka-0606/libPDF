// class PDFVersion

class PDFVersion{
private:
	int major;
	int minor;
	bool error;
	bool valid;
public:
	PDFVersion();
	bool set(char* label);
	bool isValid();
	void print();
	char v[4];
};
