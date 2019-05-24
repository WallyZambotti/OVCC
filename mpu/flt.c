#include <stdio.h>

union Data
{
    double dval;
    unsigned long long llval;
    unsigned char bytes[8];
};

void prtdbl(double val)
{  
    union Data data;
    int i;

    data.dval = val;
    
    printf("%f : %llx ", data.dval, data.llval);

    for (i = 0 ; i < 8 ; i++)
    {
        printf("%02x", data.bytes[i]);
    }
    
    printf("\n");
}

main(int agrc, char *argv[])
{
    prtdbl(0.0);
    prtdbl(4.0);
    prtdbl(2.0);
    prtdbl(-2.0);
    prtdbl(1.0);
    prtdbl(-1.0);
    prtdbl(0.5);
    prtdbl(-0.5);
    prtdbl(0.25);
    prtdbl(-0.25);
    prtdbl(0.1);
}