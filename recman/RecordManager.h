#pragma once

#include "RID.h"
#include "CompOp.h"
#include "../filesystem/bufmanager/BufPageManager.h"

const int CREATE_FILE_FAILURE = 101;
const int OPEN_FILE_FAILURE = 102;
const int WRITE_PAGE_FAILURE = 103;

class MRecord {
public:
    char * d_ptr;
    RID rid;
    MRecord();
};

struct HeaderPage{
    int record_size;
    int next_empty_page;
};

class FileHandle {
    BufPageManager * bpman;
    // stay constant
    int fileId;
    int recordSize;
    int slotBitmapSizePerPage;
    int slotNumPerPage;

    int firstEmptyPage;
    int inline getSlotBitmapSizePerPage() { return (PAGE_SIZE - 4) / (1 + recordSize * 8); } // header size
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