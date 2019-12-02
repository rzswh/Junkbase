#include "recPacker.h"
#include <cstring>

RecordPacker::RecordPacker(int len) {
    buf.reserve(len);
}

void RecordPacker::addChar(const char * s, int len) {
    for (int i = 0; i < len; i++) 
        buf.push_back(s[i]);
}

void RecordPacker::addChar(const char * s, int len, int maxlen) {
    for (int i = 0; i < len; i++) 
        buf.push_back(s[i]);
    for (int i = len; i < maxlen; i++)
        buf.push_back(0);
}

void RecordPacker::addBinary(const void * d, int len) {
    addChar((char*)d, len);
}

void RecordPacker::addInt(int a) {
    addChar((char*)(&a), 4);
}

void RecordPacker::addRID(const RID & rid) {
    addChar((char*)(&rid), sizeof(RID));
}

void RecordPacker::addNumeric(const Numeric & num) {
    char buf[32];
    int len = num.toBinary(buf);
    addChar(buf, len);
}

void RecordPacker::addDate(const Date & dat) {
    addChar((char*)(&dat), sizeof(Date));
}

void RecordPacker::unpack(void * _data, int maxlen) {
    char * buf = (char *)_data;
    int len = maxlen > this->buf.size() ? this->buf.size() : maxlen;
    memcpy(buf, this->buf.data(), len);
}