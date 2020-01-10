#pragma once
#include <iostream>
#include <string>
using std::ostream;
using std::string;
struct Numeric {
    /**
     * M: # of significant digits (M <= 65)
     * D: # of digits following the decimal point
     */
    uint8_t M, D, SIGN;
    uint32_t digits[8]; // 9 digits in one int
    Numeric() : M(0), D(0), SIGN(0) {}
    Numeric(const char *str);
    void debug() const
    {
        printf("/* Numeric(%d, %d):", M, D);
        for (int i = (M - 1) / 9; i >= 0; i--)
            printf(" %d", digits[i]);
        printf("*/");
    }
    /**
     * Length is returned.
     * little-endian machine only
     */
    int toBinary(void *buf) const
    {
        char *cbuf = (char *)buf;
        char *cdig = (char *)digits;
        int i = 0;
        for (; i < M && cdig[i]; i++) {
            cbuf[i] = cdig[i];
        }
        return i == 0 ? 1 : i;
    }
    // length is returned
    int toString(char buf[], int maxlen) const;
    string toString() const;
    bool operator==(const Numeric &n2) const;
    bool operator<(const Numeric &n2) const;
    friend ostream &operator<<(ostream &out, const Numeric &num);
};

struct Date {
    unsigned char month, day;
    unsigned char year; // 0000, 1901, 1902, ..., 2155
public:
    Date() : year(0), month(0), day(0) {}
    Date(const char *str);
    bool checkValid() const;
    bool operator<(const Date &date) const;
    bool operator==(const Date &date) const;
    string toString() const;
};