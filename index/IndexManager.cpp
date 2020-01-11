#include "IndexManager.h"
#include "../errors.h"
#include "../recman/RecordManager.h"
#include <cstring>
#include <string>
using std::string;

void pageInitialize(IndexHeaderPage &hp);

void pageInitialize(IndexHeaderPage &hp, AttrType at, int al)
{
    hp.attrType = at;
    hp.attrLen = al;
    hp.totalPages = 2;
    hp.rootPage = 1;
    hp.nextEmptyPage = 2;
}

string getRefTableName(const char *file_name, int index_no)
{
    return file_name;
}

IndexManager::IndexManager(BufPageManager *bpm) : bpman(bpm) {}

char *IndexManager::makeKey(int N, void *d_ptr, int *offsets, int *lengths,
                            AttrTypeAtom *types, int &errCode)
{
    int totalLen = 0;
    for (int i = 0; i < N; i++) {
        totalLen += lengths[i];
    }
    char *ret = new char[totalLen];
    int prefLen = 0;
    for (int i = 0; i < N; i++) {
        if (isNull((char *)d_ptr + offsets[i], types[i])) {
            errCode = NULL_VALUE_KEY;
        }
        memcpy(ret + prefLen, (char *)d_ptr + offsets[i], lengths[i]);
        prefLen += lengths[i];
    }
    return ret;
}

int IndexManager::createIndex(const char *file_name, int index_no,
                              AttrType attr_type, vector<int> &attr_len_arr)
{
    int ret_code = 0;
    FileManager *fm = bpman->fileManager;
    char *newFileName = actualIndexFileName(file_name, index_no);
    // compute attr_len
    int attr_len = 0;
    for (int l : attr_len_arr)
        attr_len += l;
    // initialize
    IndexHeaderPage header;
    pageInitialize(header, attr_type, attr_len);
    // copy into buf array
    char buf[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    memcpy(buf, &header, sizeof(header));
    // atrLens array
    char *buf_ptr = buf + sizeof(header);
    for (auto l : attr_len_arr) {
        *((int *)buf_ptr) = l;
        buf_ptr += sizeof(int);
    }
    int fileId;
    do {
        if (!fm->createFile(newFileName)) {
            ret_code = CREATE_FILE_FAILURE;
            break;
        }
        if (!fm->openFile(newFileName, fileId)) {
            ret_code = OPEN_FILE_FAILURE;
            break;
        }
        if (fm->writePage(fileId, 0, (BufType)buf, 0) != 0) {
            ret_code = WRITE_PAGE_FAILURE;
            break;
        }
        fm->closeFile(fileId);
    } while (false);
    delete[] newFileName;
    return ret_code;
}

int IndexManager::openIndex(const char *file_name, int index_no,
                            IndexHandle *&ih)
{
    FileManager *fm = bpman->fileManager;
    int fileId;
    char *newFileName = actualIndexFileName(file_name, index_no);
    if (!fm->openFile(newFileName, fileId)) {
        delete[] newFileName;
        return OPEN_FILE_FAILURE;
    }
    string refTableName = getRefTableName(file_name, index_no);
    FileHandle *fh;
    RecordManager::quickOpen(refTableName.c_str(), fh);
    ih = IndexHandle::createFromFile(fileId, bpman, fh);
    delete[] newFileName;
    return 0;
}

int IndexManager::deleteIndex(const char *file_name, int index_no)
{
    char *newFileName = actualIndexFileName(file_name, index_no);
    int ret = remove(newFileName);
    delete[] newFileName;
    return ret;
}

int IndexManager::closeIndex(IndexHandle &ih)
{
    FileManager *fm = ih.getBpman()->fileManager;
    ih.getBpman()->close();
    fm->closeFile(ih.getFileId());
    return 0;
}