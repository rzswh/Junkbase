#pragma once

class Numeric {
    /**
     * M: # of significant digits (M <= 65)
     * D: # of digits following the decimal point
     */
    int M, D;
    int digits[8]; // 9 digits in one int
public:
    /**
     * Length is returned.
     * little-endian machine only
     */ 
    int toBinary(void * buf) const {
        char * cbuf = (char*) buf;
        char * cdig = (char*) digits;
        int i = 0;
        for (; i < M && cdig[i]; i ++) {
            cbuf[i] = cdig[i];
        }
        return i == 0 ? 1 : i;
    }
};

class Date {
    unsigned char year; // 0000, 1901, 1902, ..., 2155
    unsigned char month, day;
};