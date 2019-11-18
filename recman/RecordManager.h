#pragma once

#include "RID.h"
#include "../CompOp.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "../errors.h"
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
    int total_pages;
    HeaderPage():record_size(0), next_empty_page(0), total_pages(0){}
    HeaderPage(int recordSize, int nextEmptyPage, int totalPage) {
        record_size = recordSize;
        next_empty_page = nextEmptyPage;
        total_pages = totalPage;
    }
};

class FileHandle {
    BufPageManager * bpman;
    // stay constant
    int fileId;
    int recordSize;
    int slotBitmapSizePerPage;
    int slotNumPerPage;

    int firstEmptyPage;
    /**
     * Number of data pages (except header page). 
     */ 
    int totalPages;
    int inline getSlotBitmapSizePerPage() { return (PAGE_SIZE - 8) / (1 + recordSize * 8); } // header size
    void updateHeader();
    char* getPage(int fileID, int pageID, int& index);
    int inline slotPos(int slot) { return 4 + slotBitmapSizePerPage + slot * recordSize; }
public:
    FileHandle(int fid, BufPageManager * pm);
    ~FileHandle();
    int getRecord(const RID & rid, MRecord &record);
    int updateRecord(const MRecord &record);
    int removeRecord(const RID & rid);
    int insertRecord(char * d_ptr, RID &rid);

    // empty page chain operations
    int& getNextEmpty(void * page) {
        return *((int*)(page));
    }
    int& getPreviousEmpty(void * page) {
        return *((int*)((char*)page + PAGE_SIZE - 4));
    }
    int editNextEmpty(int page, int next);
    int editPreviousEmpty(int page, int previous);
    class iterator {
        iterator(FileHandle * fh, RID rid, int attrLength, int attrOffset, Operation compOp, void *value);
        RID rid;
        FileHandle * fh;
        Operation comp_op;
        void * value;
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
    iterator findRecord(int attrLength, int attrOffset, Operation compOp, void *value);
private:
    // for scanning
    RID nextRecord(RID prev, int attrLength, int attrOffset, Operation compOp, void *value);
    friend class RecordManager;
public:
    void debug();
};

class RecordManager {
    BufPageManager * pm;
public:
    RecordManager(BufPageManager & pm);
    int createFile(const char * file_name, int record_size);
    int deleteFile(FileHandle & fh);
    int openFile(const char * file_name, FileHandle *& fh);
    int closeFile(FileHandle & fh);
};