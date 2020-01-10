#include "../def.h"
#include "../errors.h"
#include "../queryman/ValueHolder.h"
#include "../recman/RID.h"
#include "helper.h"
#include "type.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
using std::cout;
using std::endl;
using std::ostream;
using std::string;

int RecordHelper::getOffset(void *d_ptr)
{
    return *(int *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN);
}
int RecordHelper::getLength(void *d_ptr)
{
    return *(int *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN +
                    sizeof(int));
}
AttrTypeAtom RecordHelper::getType(void *d_ptr)
{
    return (AttrTypeAtom) * (uint8_t *)((char *)d_ptr + MAX_TABLE_NAME_LEN +
                                        MAX_ATTR_NAME_LEN + sizeof(int) * 2);
}
bool RecordHelper::getNotNull(void *d_ptr)
{
    return (AttrTypeAtom) *
           (uint8_t *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN +
                       sizeof(int) * 2 + 1);
}
int RecordHelper::getIndex(void *d_ptr)
{
    return *(int *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN +
                    sizeof(int) * 2 + 2);
}
RID RecordHelper::getDefaultValueRID(void *d_ptr)
{
    return *(RID *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN +
                    sizeof(int) * 3 + 2);
}
RID RecordHelper::getRefTableNameRID(void *d_ptr)
{
    return *(RID *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN +
                    sizeof(int) * 3 + 2 + sizeof(RID));
}
RID RecordHelper::getRefColumnNameRID(void *d_ptr)
{
    return *(RID *)((char *)d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN +
                    sizeof(int) * 3 + 2 + sizeof(RID) * 2);
}
string RecordHelper::getAttrName(void *d_ptr)
{
    return string((char *)d_ptr + MAX_TABLE_NAME_LEN)
        .substr(0, MAX_ATTR_NAME_LEN);
}

int AttrTypeHelper::getRawLength(int len, AttrTypeAtom atom)
{
    if (atom == TYPE_VARCHAR)
        return sizeof(RID);
    else if (atom == TYPE_NUMERIC)
        return sizeof(Numeric);
    else if (atom == TYPE_DATE)
        return sizeof(Date);
    else if (atom == TYPE_RID)
        return sizeof(RID);
    else
        return len;
}

Numeric::Numeric(const char *_s)
{
    char *str = const_cast<char *>(_s);
    // remove leading and trailing zeros
    if (str[0] == '-') SIGN = 1;
    if (str[0] == '-' || str[0] == '+') str++;
    while (str[0] == '0')
        str++;
    int L = strlen(str);
    while (str[L - 1] == '0')
        L--;
    int dotPos = 0;
    while (dotPos < L && str[dotPos] != '.')
        dotPos++;
    M = L - (dotPos != L);
    D = M - dotPos;
    assert(M <= 65);
    memset(digits, 0, sizeof(digits));
    for (int i = L - 1, P = 1, j = 0; i >= 0; i--) {
        if (str[i] == '.') continue;
        digits[j] += (str[i] - '0') * P;
        P *= 10;
        if (P == 1000000000) {
            P = 1;
            j++;
        }
    }
}

bool Numeric::operator==(const Numeric &n2) const
{
    if (n2.SIGN != SIGN) return false; // -0 < +0
    char buf1[70], buf2[70];
    int SGN = SIGN;
    const_cast<Numeric &>(n2).SIGN = const_cast<Numeric *>(this)->SIGN = 0;
    int len1 = toString(buf1, 70);
    int len2 = n2.toString(buf2, 70);
    const_cast<Numeric &>(n2).SIGN = const_cast<Numeric *>(this)->SIGN = SGN;
    int dot1 = 0, dot2 = 0;
    while (buf1[dot1] != '.')
        dot1++;
    while (buf2[dot2] != '.')
        dot2++;
    int i = 1;
    for (; i + dot1 < len1 && i + dot2 < len2; i++)
        if (buf1[i + dot1] != buf2[i + dot2]) return false;
    for (; i + dot1 < len1; i++)
        if (buf1[i + dot1] != '0') return false;
    for (; i + dot2 < len2; i++)
        if (buf2[i + dot2] != '0') return false;
    i = 1;
    for (; 0 <= dot1 - i && 0 <= dot2 - i; i++)
        if (buf1[dot1 - i] != buf2[dot2 - i]) return false;
    for (; dot1 - i >= 0; i++)
        if (buf1[dot1 - i] != '0') return false;
    for (; dot2 - i >= 0; i++)
        if (buf2[dot2 - i] != '0') return false;
    return true;
}
bool Numeric::operator<(const Numeric &n2) const
{
    if (SIGN != n2.SIGN) return SIGN > n2.SIGN; // -0 < +0
    char buf1[70], buf2[70];
    int SGN = SIGN;
    const_cast<Numeric &>(n2).SIGN = const_cast<Numeric *>(this)->SIGN = 0;
    int len1 = toString(buf1, 70);
    int len2 = n2.toString(buf2, 70);
    const_cast<Numeric &>(n2).SIGN = const_cast<Numeric *>(this)->SIGN = SGN;
    int dot1 = 0, dot2 = 0;
    while (buf1[dot1] != '.')
        dot1++;
    while (buf2[dot2] != '.')
        dot2++;
    int i1 = 0, i2 = 0;
    for (; dot1 - i1 > dot2; i1++)
        if (buf1[i1] != '0') return SIGN == 1; // |n1| > |n2|
    for (; dot2 - i2 > dot1; i2++)
        if (buf2[i2] != '0') return SIGN == 0; // |n1| < |n2|
    for (; i1 < len1 && i2 < len2; i1++, i2++) {
        if (buf1[i1] < buf2[i2])
            return SIGN == 0;
        else if (buf1[i1] > buf2[i2])
            return SIGN == 1;
    }
    for (; i1 < len1; i1++)
        if (buf1[i1] != '0') return SIGN == 1;
    for (; i2 < len2; i2++)
        if (buf2[i2] != '0') return SIGN == 0;
    return false;
}

int Numeric::toString(char _buf[], int maxlen) const
{
    assert(maxlen >= 68);
    char *buf = _buf;
    if (SIGN == 1) *(buf++) = '-';
    for (int i = M, j = digits[0], k = 0, l = 9; i >= 0; i--) {
        if (i == M - D) {
            buf[i] = '.';
            continue;
        }
        buf[i] = j % 10 + '0';
        l--;
        if (l == 0) {
            j = digits[++k];
            l = 9;
        } else {
            j /= 10;
        }
    }
    buf[M + 1] = '\0';
    return M + 1 + (buf - _buf);
}

string Numeric::toString() const
{
    char buf[70];
    toString(buf, 70);
    return string(buf);
}

ostream &operator<<(ostream &out, const Numeric &num)
{
    char buf[70];
    num.debug();
    num.toString(buf, sizeof(buf));
    out << buf;
}

Date::Date(const char *str)
{
    //
    char *ptr = const_cast<char *>(str);
    int y = atoi(ptr);
    if (y > 0) y -= 1900;
    year = y;
    while (*ptr && *ptr != '-')
        ptr++;
    month = *ptr ? atoi(++ptr) : 0;
    while (*ptr && *ptr != '-')
        ptr++;
    day = *ptr ? atoi(++ptr) : 0;
}

bool Date::checkValid() const
{
    if (year == 0) return month == 0 && day == 0;
    const int DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int year = this->year + 1900;
    if (month > 12 || month == 0) return false;
    if (month == 2)
        return day >= 1 && day <= 28 + (year % 4 == 0 &&
                                        (year % 100 != 0 || year % 400 == 0));
    else
        return day >= 1 && day <= DAYS[month - 1];
}

bool Date::operator<(const Date &date) const
{
    if (year != date.year) return year < date.year;
    if (month != date.month) return month < date.month;
    return day < date.day;
}

bool Date::operator==(const Date &date) const
{
    return year == date.year && month == date.month && day == date.day;
}

string Date::toString() const
{
    string ret;
    if (year == 0)
        ret = "0000-";
    else
        ret = std::to_string(year + 1900) + "-";
    return ret += std::to_string(month) + "-" + std::to_string(day);
}

// see in ValueHolder.h
int convertType(ValueHolder &val, AttrTypeAtom dstType)
{
    if (dstType == TYPE_DATE && val.attrType == TYPE_CHAR) {
        if (val.isNull()) {
            val = ValueHolder::makeNull(TYPE_DATE);
            return 0;
        }
        auto date = Date(val.buf);
        if (date.checkValid())
            val = ValueHolder(date);
        else {
            return DATE_FORMAT_INVALID;
        }
    }
    return 0;
}

// see in def.h
bool isNull(const void *buf, AttrTypeAtom type)
{
    if (!buf) return false;
    return type == TYPE_INT ? ((unsigned *)buf)[0] == 0x80000000
                            : (((unsigned char *)buf)[0] & 0x80) == 0x80;
}

// #define TEST
#ifdef TEST

int main()
{
    // testing
    cout << Numeric("3.5") << endl;
    cout << Numeric("0.123456789") << endl;
    cout << Numeric("123456789") << endl;
    cout << Numeric("123456789.0") << endl;
    cout << Numeric("12345678999.99987654321") << endl;
    cout << (Numeric("12345678999.99987654321") == Numeric("123456789.0"))
         << endl;
    cout << (Numeric("012345.6789") == Numeric("12345.67890")) << endl;
    cout << (Numeric("0.123456789") == Numeric("123456789.0")) << endl;
    cout << (Numeric("12345678999.99987654321") < Numeric("123456789.0"))
         << endl;
    cout << (Numeric("0123456789") < Numeric("123456789.1")) << endl;
    cout << (Numeric("0.123456789") < Numeric("123456789.0")) << endl;
    return 0;
}

#endif