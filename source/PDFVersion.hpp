// class PDFVersion

class PDFVersion{
private:
	int major;
	int minor;
	bool error;
public:
	PDFVersion();
	bool set(char* label);
	void print();
	char v[4];
};
