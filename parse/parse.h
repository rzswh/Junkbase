#pragma once

#include <cstring>

struct ValueHolder {
    char * buf;
    int len;
    ValueHolder(int x) {
        len = sizeof(int);
        buf = new char[len];
        memcpy(buf, &x, len);
    }
    ValueHolder(double x) {
        len = sizeof(double);
        buf = new char[len];
        memcpy(buf, &x, len);
    }
    ValueHolder(const char * str) {
        len = strlen(str);
        buf = new char[len];
        memcpy(buf, const_cast<char*>(str), len);
    }
    ValueHolder() { // NULL
        len = 0;
        buf = nullptr;
    }
    ValueHolder(const ValueHolder & vh) {
        buf = vh.len == 0 ? nullptr : new char[vh.len];
        len = vh.len;
        memcpy(buf, vh.buf, len);
    }
    ValueHolder(ValueHolder && vh) {
        buf = vh.buf; vh.buf = nullptr;
        len = vh.len;
    }
    ValueHolder & operator=(const ValueHolder & vh) {
        if (&vh == this) return *this;
        len = vh.len;
        delete [] buf;
        buf = new char[len];
        memcpy(buf, vh.buf, len);
    }
    ValueHolder & operator=(ValueHolder && vh) {
        if (&vh == this) return *this;
        len = vh.len;
        if (buf) delete [] buf;
        buf = vh.buf;
        vh.buf = nullptr;
    }
    ~ValueHolder() {
        delete [] buf;
    }
};