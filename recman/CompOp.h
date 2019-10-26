#pragma once
#include <cstring>

class CompOp {
public:
    virtual bool check(void *, void *) = 0;
};

class CompOpBin : public CompOp {};

class Every : public CompOpBin {
public:
    virtual bool check(void *, void *) {
        return true;
    }
};

template<typename T>
class Equal : public CompOpBin {
public:
    virtual bool check(void *a, void *b) {
        return memcmp(a, b, sizeof(T)) == 0;
    }
};

template<typename T>
class NotEqual : public CompOpBin {
public:
    virtual bool check(void *a, void *b) {
        int N = sizeof(T);
        return memcmp(a, b, N) != 0;
    }
};

class Operation {
private:
    int codeOp;
public:
    enum OpKind {EVERY, EQUAL_INT, NEQUAL_INT};
    Operation (int op) : codeOp(op) {}
    bool check(void *a, void *b) {
        switch (codeOp)
        {
        case EVERY: return Every().check(a, b);
        case EQUAL_INT: return Equal<int>().check(a, b);
        case NEQUAL_INT: return NotEqual<int>().check(a, b);
        default: return false;
        }
    }
};
