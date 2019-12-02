#pragma once
#include "../filesystem/utils/pagedef.h"

class MemAllocStrategy {
    int * firstEmpty;
public:
    const int slotBytesNum;
    const int slotNum;
    const int recordSize;
    MemAllocStrategy(int * fe, int recordSize, int slotNum) : 
        firstEmpty(fe),
        slotNum(slotNum), 
        slotBytesNum((7 + slotNum) / 8), 
        recordSize(recordSize)
    {}
    int& firstEmptyPage() { return *firstEmpty; }
    virtual int slotPos(int slot) = 0;
    virtual int& nextEmpty(void *buf) = 0;
    virtual int& previousEmpty(void *buf) = 0;
    virtual char& bitMapByte(void *buf, int pos) = 0;
};
/**
 * Record page:
 * 4 bytes: next empty page (when the page is full, it represents nothing)
 * slotBytesNum bytes: slotBitmap
 * slotNum * recordSize bytes: record
 * last 4 bytes: previous empty page
 */ 
class RecordMemAllocateStrategy : public MemAllocStrategy
{
public:
    RecordMemAllocateStrategy(int recordSize, int * fe) : 
        MemAllocStrategy(
            fe, recordSize, 
            (PAGE_SIZE - 8) / (1 + recordSize * 8) * 8
        ) 
    { }
    int inline slotPos(int slot) override { 
        return 4 + slotBytesNum + slot * recordSize; 
    }
    int & nextEmpty(void * buf) {
        return ((int*)buf)[0];
    }
    int & previousEmpty(void *buf) {
        return *((int*)((char*)buf + PAGE_SIZE - 4));
    }
    char & bitMapByte(void *buf, int pos) override {
        return *((char*)((char*)buf + 4 + pos));
    }
};

/**
 * Variant Data page:
 *  4 bytes: 0
 *  4 bytes: nextPage
 *  4 bytes: previousPage
 * 36 bytes: all 0x11
 * 16 bytes: bitmap
 * 127 * 64 bytes: fixed length segments
 * -  8 bytes: next part RID (0 if end)
 * - 56 bytes: data
 */ 
class VariantMemAllocateStrategy : public MemAllocStrategy
{
public:
    VariantMemAllocateStrategy(int recordSize, int * fe) : 
        MemAllocStrategy(
            fe, 64, (PAGE_SIZE - 48) * 8 / (1 + 64 * 8)
        ) 
    { }
    int inline slotPos(int slot) override { 
        return 64 + slot * recordSize; 
    }
    int & nextEmpty(void * buf) {
        return ((int*)buf)[1];
    }
    int & previousEmpty(void *buf) {
        return ((int*)buf)[2];
    }
    char & bitMapByte(void *buf, int pos) override {
        return *((char*)((char*)buf + 48 + pos));
    }
};