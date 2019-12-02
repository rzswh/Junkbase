#pragma once

#include "RID.h"
#include "../CompOp.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "../errors.h"
#include "recMemAllocStrat.h"
#include <cstdio>

class MRecord {
public:
    char * d_ptr;
    RID rid;
    MRecord();
};

struct HeaderPage{
    int record_size;
    int next_empty_page;
    int next_empty_variant_data_page;
    int total_pages;
    HeaderPage():record_size(0), next_empty_page(0), total_pages(0), next_empty_variant_data_page(0){}
    HeaderPage(int recordSize, int nextEmptyPage, int nxtEmpVDPage, int totalPage) {
        record_size = recordSize;
        next_empty_page = nextEmptyPage;
        next_empty_variant_data_page = nxtEmpVDPage;
        total_pages = totalPage;
    }
};

class FileHandle {
    BufPageManager * bpman;
    // stay constant
    int fileId;
    int recordSize;
    // int slotBitmapSizePerPage;
    // int slotNumPerPage;

    int firstEmptyPage;
    int firstEmptyVarDataPage;
    /**
     * Number of data pages (except header page). 
     */ 
    int totalPages;
    RecordMemAllocateStrategy * recordMemAllocStrat;
    VariantMemAllocateStrategy * varMemAllocStrat;

    void updateHeader();
    char* getPage(int fileID, int pageID, int& index, MemAllocStrategy * ms);
    // int inline slotPos(int slot) { return 4 + slotBitmapSizePerPage + slot * recordSize; }
public:
    FileHandle(int fid, BufPageManager * pm);
    ~FileHandle();
    int getRecord(const RID & rid, MRecord &record);
    int updateRecord(const MRecord &record);
    int removeRecord(const RID & rid);
    int insertRecord(char * d_ptr, RID &rid);

    /**
     * Variant length data will be divided into fixed length parts and stored in
     * seperate pages. 
     * Every piece of variant length data will be represented by an RID as well.
     * It is the header of the data sequence. 
     */
    int insertVariant(char * buf, int length, RID & rid);
    int getVariant(const RID& rid, char * buf, int maxLen);
    int removeVariant(const RID & rid);

    int getData(const RID & rid, void * dest, MemAllocStrategy * strat);
    int insertDataAt(void * d_ptr, int length, MemAllocStrategy * strat, RID & rid);
    void usePage(int page, MemAllocStrategy * strat, void * buf);
    void recyclePage(int page, MemAllocStrategy * strat, void * buf);
    int removeData(const RID & rid, MemAllocStrategy * strat);

    // empty page chain operations
    int editNextEmpty(int page, int next, MemAllocStrategy * strat);
    int editPreviousEmpty(int page, int previous, MemAllocStrategy * strat);
    class iterator {
        iterator(
            FileHandle * fh, RID rid, 
            int attrLength, 
            int attrOffset, 
            Operation compOp, 
            const void *value);
        RID rid;
        FileHandle * fh;
        Operation comp_op;
        const void * value;
        int attrLength, attrOffset;
    public:
        MRecord operator*();
        iterator& operator++();
        iterator operator++(int) {
            iterator ret = *this;
            this->operator++();
            return ret;
        }
        bool end() const;
        bool operator==(const iterator &it) const;
        bool operator!=(const iterator &it) const { return !operator==(it); }
        friend class FileHandle;
    };
    iterator findRecord(int attrLength, int attrOffset, Operation compOp, const void *value);
private:
    // for scanning
    RID nextRecord(RID prev, int attrLength, int attrOffset, Operation compOp, const void *value);
    friend class RecordManager;
public:
    void debug();
};

class RecordManager {
    BufPageManager * pm;
public:
    RecordManager(BufPageManager & pm);
    int createFile(const char * file_name, int record_size);
    int deleteFile(const char * file_name);
    int openFile(const char * file_name, FileHandle *& fh);
    int closeFile(FileHandle & fh);
};