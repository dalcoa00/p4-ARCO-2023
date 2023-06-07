#ifndef IEEE754CONVERTER_H
#define IEEE754CONVERTER_H


class IEEE754Converter
{
public:
    IEEE754Converter();
    static unsigned long floattoIEESign (float num);
    static unsigned long floattoIEEMantisa (float num);
    static unsigned long floattoIEE2 ();
    static unsigned long floattoIEEExp (float num);
    static float IEEtofloat (int signo, int exponente, int mantisa);
    static float IEEtofloat2 ();
    static unsigned long floattoIEENumex (float num);

private:
    union Code {

        struct{
            unsigned long partFrac : 23;
            unsigned long expo : 8;
            unsigned long sign : 1;
        }bitfield;

        float numero;
        unsigned long numerox;

    };
};

#endif // IEEE754CONVERTER_H
