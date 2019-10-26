#include "RecordManager.h"
#include "../filesystem/utils/pagedef.h"
#include <cstring>
#include <cassert>

RecordManager::RecordManager(BufPageManager & bm) : pm(&bm) {
}

int RecordManager::createFile(const char * file_name, int record_size) {
    FileManager * fm = pm->fileManager;
    // initialize
    HeaderPage header;
    header.record_size = record_size;
    header.next_empty_page = 1; // next page
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

int RecordManager::deleteFile(FileHandle & fh) {
    FileManager * fm = pm->fileManager;
    fm->closeFile(fh.fileId);
    return 0;
}

int RecordManager::closeFile(FileHandle & fh) {
    FileManager * fm = pm->fileManager;
    fh.bpman->close();
    fm->closeFile(fh.fileId);
    return 0;
}

MRecord::MRecord() { d_ptr = nullptr; }


FileHandle::FileHandle(int fid, BufPageManager * bm) 
    : fileId(fid), bpman(bm) 
{
    int pg_index;
    char * buf = getPage(fid, 0, pg_index);
    HeaderPage header;
    memcpy(&header, buf, sizeof(header));
    recordSize = header.record_size;
    firstEmptyPage = header.next_empty_page;
    slotBitmapSizePerPage = getSlotBitmapSizePerPage();
    slotNumPerPage = slotBitmapSizePerPage * 8;
}

int FileHandle::getRecord(const RID & rid, MRecord &record) {
    int page = rid.getPageNum();
    int slot = rid.getSlotNum();
    int pg_index;
    char * buf = getPage(fileId, page, pg_index);
    // Usually record is newly created...
    if (record.d_ptr == nullptr) record.d_ptr = new char[recordSize];
    // Assume record size is recordSize
    memcpy(record.d_ptr, buf + slotPos(slot), recordSize);
    record.rid = rid;
    return 0;
}

int FileHandle::updateRecord(const MRecord &record) {
    const RID & rid = record.rid;
    int page = rid.getPageNum();
    int slot = rid.getSlotNum();
    int pg_index;
    char * buf = getPage(fileId, page, pg_index);
    memcpy(buf + slotPos(slot), record.d_ptr, recordSize);
    return 0;
}

int FileHandle::insertRecord(char * d_ptr, RID &rid) {
    int page = firstEmptyPage;
    int pg_index, skip_page_num;
    char* buf = getPage(fileId, page, pg_index);
    for (int i = 0, off = 0; i < 2; i++) {
        while (buf[4+off] == -1 && off < slotBitmapSizePerPage) off++;
        if (slotBitmapSizePerPage == off) {
            memcpy(&page, buf, 4); // 
            // this page is full! update information
            if (i == 0) { // no free slot in this page!
                // robust: find next empty page
                buf = getPage(fileId, page, pg_index);
                off = 0; i = -1;
                continue;
                //return INTERNAL_ERROR; 
            } else {
                firstEmptyPage = page;
                updateHeader();
                return 0;
            }
        }
        if (i == 1) break; // not only empty slot
        //
        int index = 0; unsigned char bm_c = buf[4+off];
        while (bm_c & 1) index ++, bm_c >>= 1; // Big-Endian byte order
        buf[4 + off] |= 1 << index;
        index += off * 8;
        memcpy(buf + slotPos(index), d_ptr, recordSize);
        bpman->markDirty(pg_index);
        rid = RID(page, index);
    }
    return 0; // not full, normal exit
}

// if full -> not full, mark this page as firstEmptyPage and old 'firstEmptyPage' next page 
int FileHandle::removeRecord(const RID & rid) {
    bool full = true; int pg_index;
    char* buf = getPage(fileId, rid.getPageNum(), pg_index);
    for (int i = 0; full && i < slotBitmapSizePerPage; i++) full = buf[4+i] == -1;
    if (full) {
        memcpy(buf, &firstEmptyPage, sizeof(int));
        firstEmptyPage = rid.getPageNum();
        updateHeader();
    }
    int slot = rid.getSlotNum();
    buf[4 + slot / 8] &= ~(1<<slot % 8);
    bpman->markDirty(pg_index);
}

void FileHandle::updateHeader() {
    int header_index;
    char * buf = getPage(fileId, 0, header_index);
    HeaderPage header_page = (HeaderPage){recordSize, firstEmptyPage};
    memcpy(buf, &header_page, sizeof(HeaderPage));
    bpman->markDirty(header_index);
}
char* FileHandle::getPage(int fileID, int pageID, int& index) {
    char * buf = (char*)(bpman->getPage(fileID, pageID, index));
    if (pageID == 0) return buf;
    int header; 
    memcpy(&header, buf, sizeof(int)); // next free page
    if (header == 0) {
        header = pageID + 1;
        memcpy(buf, &header, sizeof(int));
    }
    return buf;
}

FileHandle::~FileHandle() {
}

FileHandle::iterator FileHandle::findRecord(int attrLength, int attrOffset, Operation compOp, void *value) {
    RID rid = nextRecord(RID(1, -1), attrLength, attrOffset, compOp, value);
    return FileHandle::iterator(this, rid, attrLength, attrOffset, compOp, value);
}

bool isEmpty(void * buf) {
    int nextPage;
    memcpy(&nextPage, buf, sizeof(int));
    return nextPage == 0;
}

RID FileHandle::nextRecord(RID prev, int attrLength, int attrOffset, Operation compOp, void *value)
{
    int page = prev.getPageNum(), slot = prev.getSlotNum();
    char * curVal = new char[attrLength];
    RID ret;
    while (true) {
        slot ++;
        int pg_ind;
        unsigned char * buf = (unsigned char*) bpman->getPage(fileId, page, pg_ind);
        if (isEmpty(buf))  { // max page
            ret = RID::end();
            break;
        }

        for (int addr = slotPos(slot); slot < slotNumPerPage; slot++, addr += recordSize) {
            if ((buf[4 + slot/8] & (1<<slot % 8)) == 0) continue;
            memcpy(curVal, buf + addr + attrOffset, attrLength);
            if (compOp.check(curVal, value)) {
                ret = RID(page, slot);
                break;
            }
        }
        if (slot != slotNumPerPage) break; // found!
        slot = 0, page ++; // next page
    }
    delete [] curVal;
    return ret;
}

FileHandle::iterator::iterator(FileHandle * fh, RID rid, int attrLength, 
                               int attrOffset, Operation compOp, void *value) 
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
