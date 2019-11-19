#include <cstring>
#include "IndexManager.h"
#include "../errors.h"

void pageInitialize(IndexHeaderPage & hp);

void pageInitialize(IndexHeaderPage & hp, AttrType at, int al) {
    hp.attrType = at;
    hp.attrLen = al;
    hp.totalPages = 2;
    hp.rootPage = 1;
    hp.nextEmptyPage = 2;
}

IndexManager::IndexManager(BufPageManager* bpm) : bpman(bpm)
{}

int IndexManager::createIndex(const char * file_name, int index_no, AttrType attr_type, int attr_len)
{
    int ret_code = 0;
    FileManager* fm = bpman->fileManager;
    char * newFileName = actualIndexFileName(file_name, index_no);
    // initialize
    IndexHeaderPage header;
    pageInitialize(header, attr_type, attr_len);
    // copy into buf array
    char * buf[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    memcpy(buf, &header, sizeof(header));
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
    delete [] newFileName;
    return ret_code;
}

int IndexManager::openIndex(const char * file_name, int index_no, IndexHandle *& ih) {
    FileManager * fm = bpman->fileManager;
    int fileId;
    char * newFileName = actualIndexFileName(file_name, index_no);
    if (!fm->openFile(newFileName, fileId)) {
        delete [] newFileName;
        return OPEN_FILE_FAILURE;
    }
    ih = IndexHandle::createFromFile(fileId, bpman);
    delete [] newFileName;
    return 0;
}

int IndexManager::deleteIndex(IndexHandle & ih) {
    FileManager * fm = bpman->fileManager;
    fm->closeFile(ih.getFileId());
    return 0;
}

int IndexManager::closeIndex(IndexHandle & ih) {
    FileManager * fm = bpman->fileManager;
    ih.getBpman()->close();
    fm->closeFile(ih.getFileId());
    return 0;
}