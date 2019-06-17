union Data
{
	unsigned long long llval;
	double dval;
	unsigned int lval;
	unsigned char bytes[8];
};

typedef union Data PDP1data;
typedef union Data IEEE754data;

void QueueGPUrequest(unsigned char, unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);
void StartGPUQueue();
void StopGPUqueue();
void ReportQueue();
