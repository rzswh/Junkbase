#pragma once

#include "../def.h"
#include "../utils/type.h"
#include <cstring>

struct ValueHolder {
    char *buf;
    int len;
    AttrTypeAtom attrType;
    ValueHolder(int x)
    {
        len = sizeof(int);
        buf = new char[len];
        memcpy(buf, &x, len);
        attrType = TYPE_INT;
    }
    ValueHolder(Numeric x)
    {
        len = sizeof(Numeric);
        buf = new char[len];
        memcpy(buf, &x, len);
        attrType = TYPE_NUMERIC;
    }
    ValueHolder(Date x)
    {
        len = sizeof(Date);
        buf = new char[len];
        memcpy(buf, &x, len);
        attrType = TYPE_DATE;
    }
    ValueHolder(const char *str)
    {
        len = strlen(str) + 1;
        buf = new char[len];
        memcpy(buf, const_cast<char *>(str), len);
        attrType = TYPE_CHAR;
    }
    ValueHolder()
    { // Nothing! Not NULL, just nothing
        len = 0;
        buf = nullptr;
        attrType = TYPE_CHAR; // TODO: ???
    }
    ValueHolder(AttrTypeAtom type, void *x, int l)
    {
        buf = new char[l];
        len = l;
        if (x) memcpy(buf, x, len);
        attrType = type;
    }
    ValueHolder(const ValueHolder &vh)
    {
        buf = vh.len == 0 ? nullptr : new char[vh.len];
        len = vh.len;
        if (buf) memcpy(buf, vh.buf, len);
        attrType = vh.attrType;
    }
    ValueHolder(ValueHolder &&vh)
    {
        buf = vh.buf;
        vh.buf = nullptr;
        len = vh.len;
        attrType = vh.attrType;
    }
    ValueHolder &operator=(const ValueHolder &vh)
    {
        if (&vh == this) return *this;
        len = vh.len;
        delete[] buf;
        buf = vh.len == 0 ? nullptr : new char[len];
        attrType = vh.attrType;
        if (buf) memcpy(buf, vh.buf, len);
    }
    ValueHolder &operator=(ValueHolder &&vh)
    {
        if (&vh == this) return *this;
        len = vh.len;
        if (buf) delete[] buf;
        buf = vh.buf;
        attrType = vh.attrType;
        vh.buf = nullptr;
    }
    ~ValueHolder() { delete[] buf; }

    bool isNull() const { return ::isNull(buf, attrType); }
    static ValueHolder makeNull(AttrTypeAtom type = TYPE_CHAR)
    {
        ValueHolder ret;
        if (type == TYPE_INT) {
            ret.len = 4;
            ret.buf = new char[4];
            ((unsigned int *)ret.buf)[0] = 0x80000000;
            ret.attrType = TYPE_INT;
        } else {
            ret.len = 1;
            ret.buf = new char[1];
            ret.buf[0] = 0x80;
            ret.attrType = type;
        }
        return ret;
    }
};

int convertType(ValueHolder &val, AttrTypeAtom dstType);
