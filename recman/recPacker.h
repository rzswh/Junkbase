#pragma once
#include "RID.h"
#include "../utils/type.h"
#include <vector>
using std::vector;

class RecordPacker {
    vector<char> buf;
public:
    RecordPacker(int len = 0);
    void addInt(int a);
    void addNumeric(const Numeric & num); // fixed-point number
    void addChar(const char * str, int len);
    void addChar(const char * str, int len, int maxlen);
    void addBinary(const void * data, int len);
    void addRID(const RID & rid);
    void addDate(const Date & date);
    int length() const { return buf.size(); }
    void unpack(void * buf, int maxlen);
};