#ifndef IEEE754CONVERTER_H
#define IEEE754CONVERTER_H

#include <stdio.h>
#include <unistd.h>


class IEEE754Converter
{
public:
    IEEE754Converter();
    static unsigned int floatToIEENumX(float num);
    static unsigned int floatToIEESign(float num);
    static unsigned int floatToIEEExp(float num);
    static unsigned int floatToIEEMantissa(float num);
    static unsigned int floatToIEE2();
    static float IEEtoFloat(int signo, int exponente, int mantissa);
    static float IEEtoFloat2();


private:

    union Code {

        struct {
            unsigned int partFrac : 23;
            unsigned int expo : 8;
            unsigned int sign : 1;
        } bitfield;

        float numero;
        unsigned int numerox;
    };
};

#endif // IEEE754CONVERTER_H
