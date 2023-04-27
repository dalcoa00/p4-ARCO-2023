#include "ieee754converter.h"

//IEEE754Converter::IEEE754Converter()
//{

//}

unsigned int IEEE754Converter::floatToIEENumX(float num) {

    union Code a;
    a.numero = num;

    return a.numerox;
}

unsigned int IEEE754Converter::floatToIEESign(float num) {

    union Code a;
    a.numero = num;

    return a.bitfield.sign;
}

unsigned int IEEE754Converter::floatToIEEExp(float num) {
    union Code a;
    a.numero = num;

    return a.bitfield.expo;
}

unsigned int IEEE754Converter::floatToIEEMantissa(float num) {
    union Code a;
    a.numero = num;

    return a.bitfield.partFrac;
}

unsigned int IEEE754Converter::floatToIEE2() {
    union Code a;
    a.numero = 2.1;

    return a.bitfield.partFrac;
}

float IEEE754Converter::IEEtoFloat(int signo, int exponente, int mantissa) {
    union Code a;

    a.bitfield.sign = signo;
    a.bitfield.expo = exponente;
    a.bitfield.partFrac = mantissa;

    return a.numero;
}

float IEEE754Converter::IEEtoFloat2() {
    union Code a;

    a.bitfield.sign = 0;
    a.bitfield.expo = 131;
    a.bitfield.partFrac = 4718592;

    return a.numero;
}
