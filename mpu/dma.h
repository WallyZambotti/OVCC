union _Data
{
	unsigned long long llval;
	double dval;
	unsigned int lval;
	unsigned char bytes[8];
    unsigned short words[4];
    unsigned int dwords[2];
};

typedef union _Data Data;
typedef union _Data PDP1data;
typedef union _Data IEEE754data;

unsigned short int ReadCoCoInt(unsigned short int);
void WriteCoCoInt(unsigned short int, unsigned short int);
int ReadCoCo4byte(unsigned short int);
void WriteCoCo4bytes(unsigned short int, PDP1data);
PDP1data ReadCoCo8bytes(unsigned short int);
void WriteCoCo8bytes(unsigned short int, PDP1data);
