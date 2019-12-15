#include "RecordManager.h"
#include "../errors.h"
#include "../filesystem/utils/pagedef.h"
#include <cstring>
#include <cassert>
#include <errno.h>
#include <iostream>

RecordManager::RecordManager(BufPageManager * bm) : pm(bm) {
}

void pageInitialize(HeaderPage &header, int record_size) {
    header.record_size = record_size;
    header.next_empty_page = 1; // next page
    header.next_empty_variant_data_page = 2;
    header.total_pages = 2;
}

int RecordManager::createFile(const char * file_name, int record_size) {
    FileManager * fm = pm->fileManager;
    // initialize
    HeaderPage header;
    pageInitialize(header, record_size);
    // copy into buf array
    char * buf[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    memcpy(buf, &header, sizeof(header));
    if (!fm->createFile(file_name))
        return CREATE_FILE_FAILURE;
    int fileId;
    if (!fm->openFile(file_name, fileId))
        return OPEN_FILE_FAILURE;
    if (fm->writePage(fileId, 0, (BufType)buf, 0) != 0)
        return WRITE_PAGE_FAILURE;
    fm->closeFile(fileId);
    return 0;
}

int RecordManager::openFile(const char * file_name, FileHandle *& fh) {
    FileManager * fm = pm->fileManager;
    int fileId;
    if (!fm->openFile(file_name, fileId))
        return OPEN_FILE_FAILURE;
    fh = new FileHandle(fileId, pm);
    return 0;
}

int RecordManager::deleteFile(const char * file_name) {
    int ret = remove(file_name);
    return ret;
}

int RecordManager::closeFile(FileHandle & fh) {
    FileManager * fm = fh.bpman->fileManager;
    fh.bpman->close();
    // if (errno != 0) 
    //     std::cerr << "IO_ERROR: " << errno << std::endl;
    fm->closeFile(fh.fileId);
    return 0;
}

MRecord::MRecord() { d_ptr = nullptr; }


FileHandle::FileHandle(int fid, BufPageManager * bm) 
    : fileId(fid), bpman(bm) 
{
    int pg_index;
    char * buf = getPage(fid, 0, pg_index, nullptr);
    HeaderPage header;
    memcpy(&header, buf, sizeof(header));
    recordSize = header.record_size;
    firstEmptyPage = header.next_empty_page;
    firstEmptyVarDataPage = header.next_empty_variant_data_page;
    totalPages = header.total_pages;
    recordMemAllocStrat = new RecordMemAllocateStrategy(
        recordSize, &firstEmptyPage);
    varMemAllocStrat = new VariantMemAllocateStrategy(
        VARIANT_SEGMENT_LENGTH, &firstEmptyVarDataPage);
    // slotBitmapSizePerPage = getSlotBitmapSizePerPage();
    // slotNumPerPage = slotBitmapSizePerPage * 8;
}

int FileHandle::getRecord(const RID & rid, MRecord &record) {
    // Usually record is newly created...
    if (record.d_ptr == nullptr) record.d_ptr = new char[recordSize];
    getData(rid, record.d_ptr, recordMemAllocStrat);
    // Assume record size is recordSize
    record.rid = rid;
    return 0;
}

int FileHandle::getData(const RID & rid, void * dest, MemAllocStrategy * strat) {
    int page = rid.getPageNum();
    int slot = rid.getSlotNum();
    int pg_index;
    char * buf = getPage(fileId, page, pg_index, strat);
    memcpy(dest, buf + strat->slotPos(slot), strat->recordSize);
    return 0;
}

int FileHandle::updateRecord(const MRecord &record) {
    const RID & rid = record.rid;
    int page = rid.getPageNum();
    int slot = rid.getSlotNum();
    int pg_index;
    char * buf = getPage(fileId, page, pg_index, recordMemAllocStrat);
    memcpy(buf + recordMemAllocStrat->slotPos(slot), record.d_ptr, recordSize);
    return 0;
}

int FileHandle::insertDataAt(void * d_ptr, int length, MemAllocStrategy * strat, RID & rid) {
    int page = strat->firstEmptyPage();
    int pg_index, skip_page_num;
    char* buf = getPage(fileId, page, pg_index, strat);
    const int slotBitmapSizePerPage = strat->slotBytesNum;
    for (int i = 0, off = 0; i < 2; i++) {
        while (strat->bitMapByte(buf, off) == -1 && off < slotBitmapSizePerPage) off++;
        if (slotBitmapSizePerPage == off) {
            int nextPage = strat->nextEmpty(buf);
            // this page is full! update information
            if (i == 0) { // no free slot in this page!
                // robust: find next empty page
                buf = getPage(fileId, nextPage, pg_index, strat);
                off = 0; i = -1;
                page = nextPage;
                continue;
                //return INTERNAL_ERROR; 
            } else {
                usePage(page, strat, buf);
                return 0;
            }
        }
        if (i == 1) break; // not only empty slot
        //
        int index = 0; unsigned char bm_c = strat->bitMapByte(buf, off);
        while (bm_c & 1) index ++, bm_c >>= 1; // Big-Endian byte order
        strat->bitMapByte(buf, off) |= 1 << index;
        index += off * 8;
        // copy data into the slot
        memcpy(buf + strat->slotPos(index), d_ptr, length);
        bpman->markDirty(pg_index);
        rid = RID(page, index);
    }
}

int FileHandle::insertRecord(char * d_ptr, RID &rid) {
    return insertDataAt(d_ptr, recordSize, recordMemAllocStrat, rid);
}

int FileHandle::insertVariant(char * data, int length, RID & rid) {
    RID lastRID;
    char buf[VARIANT_SEGMENT_LENGTH];
    memset(buf, 0, sizeof(buf));
    const int SegLen = VARIANT_SEGMENT_LENGTH - sizeof(RID);
    int segNum = (length + SegLen - 1) / SegLen, lastSegLen = length % SegLen;
    lastRID = RID(0, lastSegLen);
    for (int i = segNum - 1; i >= 0; i--) {
        char * ptr = (char*) (data + i * SegLen);
        int l = i == segNum - 1 ? lastSegLen : SegLen;
        memcpy(buf, &lastRID, sizeof(RID));
        memcpy(buf + sizeof(RID), ptr, l);
        insertDataAt(buf, varMemAllocStrat->recordSize, varMemAllocStrat, lastRID);
    }
    rid = lastRID;
    return 0;
}

int FileHandle::getVariant(const RID& rid, char * dest, int maxLen) {
    RID lastRID = rid;
    const int SegLen = VARIANT_SEGMENT_LENGTH - sizeof(RID);
    char buf[VARIANT_SEGMENT_LENGTH];
    int retCode = 0;
    while (maxLen > 0 && lastRID.getPageNum() > 0) {
        getData(lastRID, buf, varMemAllocStrat);
        lastRID = *((RID*)(buf));
        int len = SegLen > maxLen ? maxLen : SegLen; // buf rest space
        if (lastRID.getPageNum() == 0)
            if(len >= lastRID.getSlotNum())
                len = lastRID.getSlotNum(); // length of this
            else retCode = 1; // incomplete variant
        memcpy(dest, buf + sizeof(RID), len);
        maxLen -= len;
        dest += len;
    }
    return retCode;
}

void FileHandle::usePage(int page, MemAllocStrategy * strat, void * buf) {
    if (buf == nullptr) {
        int pg_index;
        buf = getPage(fileId, page, pg_index, strat);
    }
    int& firstEmptyPage = strat->firstEmptyPage();
    int nextPage = strat->nextEmpty(buf);
    // remove this page from the chain
    if (firstEmptyPage == page) {
        firstEmptyPage = nextPage;
        updateHeader();
    }
    int previous = strat->previousEmpty(buf);
    editPreviousEmpty(page, previous, strat);
    editNextEmpty(previous, page, strat);
}

void FileHandle::recyclePage(int page, MemAllocStrategy * strat, void * buf) {
    int firstEmptyPage = strat->firstEmptyPage();
    if (buf == nullptr) {
        int pg_index;
        buf = getPage(fileId, page, pg_index, strat);
    }
    editNextEmpty(page, firstEmptyPage, strat);
    editPreviousEmpty(firstEmptyPage, page, strat);
    strat->firstEmptyPage() = page;
    updateHeader();
}

int FileHandle::removeData(const RID & rid, MemAllocStrategy * strat) {
    bool full = true; int pg_index;
    char* buf = getPage(fileId, rid.getPageNum(), pg_index, strat);
    const int slotBitmapSizePerPage = strat->slotBytesNum;
    for (int i = 0; full && i < slotBitmapSizePerPage; i++) full = strat->bitMapByte(buf, i) == -1;
    if (full) { // full -> not full
        int page = rid.getPageNum();
        recyclePage(page, strat, buf);
    }
    int slot = rid.getSlotNum();
    strat->bitMapByte(buf, slot/8) &= ~(1<<slot % 8);
    bpman->markDirty(pg_index);
}

// if full -> not full, mark this page as firstEmptyPage and old 'firstEmptyPage' next page 
int FileHandle::removeRecord(const RID & rid) {
    if (rid.getPageNum() == 0 && rid.getSlotNum() == 0) return EMPTY_RID;
    removeData(rid, recordMemAllocStrat);
    return 0;
}

int FileHandle::removeVariant(const RID & rid) {
    if (rid.getPageNum() == 0 && rid.getSlotNum() == 0) return EMPTY_RID;
    RID lastRID = rid;
    char buf[VARIANT_SEGMENT_LENGTH];
    while (lastRID.getPageNum() > 0) {
        getData(lastRID, buf, varMemAllocStrat);
        removeData(lastRID, varMemAllocStrat);
        lastRID = *(RID*)buf;
    }
    return 0;
}

void FileHandle::updateHeader() {
    int header_index;
    char * buf = getPage(fileId, 0, header_index, nullptr);
    HeaderPage header_page = HeaderPage(recordSize, firstEmptyPage, firstEmptyVarDataPage, totalPages);
    memcpy(buf, &header_page, sizeof(HeaderPage));
    bpman->markDirty(header_index);
}
char* FileHandle::getPage(int fileID, int pageID, int& index, MemAllocStrategy * ms) {
    char * buf = (char*)(bpman->getPage(fileID, pageID, index));
    if (pageID == 0) return buf;
    int header = ms->nextEmpty(buf); 
    // new page?
    if (header == 0) {
        // allocate a new page
        header = totalPages + 1;
        if (this->totalPages < header) 
            this->totalPages = header;
        ms->nextEmpty(buf) = header;
        ms->firstEmptyPage() = pageID;
        bpman->markDirty(index);
        updateHeader();
    }
    return buf;
}

int FileHandle::editNextEmpty(int page, int next, MemAllocStrategy * strat) {
    int index;
    void * buf = bpman->getPage(fileId, page, index);
    strat->nextEmpty(buf) = next;
    bpman->markDirty(index);
}
int FileHandle::editPreviousEmpty(int page, int previous, MemAllocStrategy * strat) {
    int index;
    void * buf = bpman->getPage(fileId, page, index);
    strat->previousEmpty(buf) = previous;
    bpman->markDirty(index);
}

FileHandle::~FileHandle() {
    delete recordMemAllocStrat;
    delete varMemAllocStrat;
}

FileHandle::iterator FileHandle::findRecord(int attrLength, int attrOffset, Operation compOp, const void *value) {
    RID rid = nextRecord(RID(1, -1), attrLength, attrOffset, compOp, value);
    return FileHandle::iterator(this, rid, attrLength, attrOffset, compOp, value);
}

bool isEmpty(void * buf) {
    int nextPage;
    memcpy(&nextPage, buf, sizeof(int));
    return nextPage == 0;
}

RID FileHandle::nextRecord(RID prev, int attrLength, int attrOffset, Operation compOp, const void *value)
{
    MemAllocStrategy * s = recordMemAllocStrat;
    int page = prev.getPageNum(), slot = prev.getSlotNum();
    char * curVal = new char[attrLength];
    RID ret;
    while (true) {
        slot ++;
        int pg_ind;
        unsigned char * buf = (unsigned char*) bpman->getPage(fileId, page, pg_ind);
        if (recordMemAllocStrat->nextEmpty(buf) == 0)  { // max page / variant data page
            if (varMemAllocStrat->nextEmpty(buf) == 0) { // max page
                ret = RID::end();
                break;
            }
        }

        for (int addr = s->slotPos(slot); slot < s->slotNum; slot++, addr += recordSize) {
            if ((buf[4 + slot/8] & (1<<slot % 8)) == 0) continue;
            memcpy(curVal, buf + addr + attrOffset, attrLength);
            if (compOp.check(curVal, value, attrLength)) {
                ret = RID(page, slot);
                break;
            }
        }
        if (slot != s->slotNum) break; // found!
        slot = 0, page ++; // next page
    }
    delete [] curVal;
    return ret;
}

void FileHandle::debug() {
    printf("record size: %d\n", this->recordSize);
    printf("first empty page: %d\n", this->firstEmptyPage);
    printf("first empty var page: %d\n", this->firstEmptyVarDataPage);
    printf("total Pages: %d\n", this->totalPages);
    MemAllocStrategy* s = recordMemAllocStrat;
    for (int i = 1; i <= totalPages; i++) {
        int _ind;
        unsigned char * buf = (unsigned char *)bpman->getPage(fileId, i, _ind);
        printf("********************\n");
        s = recordMemAllocStrat;
        int next = recordMemAllocStrat->nextEmpty(buf), prev = recordMemAllocStrat->previousEmpty(buf);
        if (next == 0 && varMemAllocStrat->nextEmpty(buf) != 0) {
            next = varMemAllocStrat->nextEmpty(buf);
            prev = varMemAllocStrat->previousEmpty(buf);
            s = varMemAllocStrat;
        }
        printf("PAGE_ID=%d next page: %d previous page: %d\n", i, next, prev);
        for (int j = 0; j < s->slotNum; j++) {
            printf("#0x%03x: ", j);
            if (s->bitMapByte(buf, j / 8) & (1 << j % 8)) {
                for (int k = s->slotPos(j), k2 = 0; k2 < s->recordSize; k2++)
                    printf("%02hhx", buf[k + k2]);
            }
            printf("\n");
        }
    }
}

FileHandle::iterator::iterator(FileHandle * fh, RID rid, int attrLength, 
                               int attrOffset, Operation compOp, const void *value) 
    : fh(fh), rid(rid), attrLength(attrLength), 
      attrOffset(attrOffset), comp_op(compOp), value(value)
{}

MRecord FileHandle::iterator::operator*() {
    MRecord ret;
    fh->getRecord(rid, ret);
    return ret;
}

bool FileHandle::iterator::operator==(const FileHandle::iterator& iter) const {
    return iter.rid == iter.rid;
}

FileHandle::iterator& FileHandle::iterator::operator++() {
    rid = fh->nextRecord(rid, attrLength, attrOffset, comp_op, value);
    return *this;
}

bool FileHandle::iterator::end() const {
    return RID::end() == rid;
}
