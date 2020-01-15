#pragma once

#include "../def.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "IndexHandle.h"
#include <cstring>
#include <vector>

using std::vector;

class IndexManager
{
    BufPageManager *bpman;

public:
    IndexManager(BufPageManager *man);
    int createIndex(const char *file_name, int index_no, AttrType attr_type,
                    vector<int> &attr_len_arr);
    int openIndex(const char *file_name, int index_no, IndexHandle *&ih,
                  FileHandle *&fh_t);
    static int closeIndex(IndexHandle &ih);
    int deleteIndex(const char *file_name, int index_no);
    static int quickOpen(const char *fileName, int index_no, IndexHandle *&ih,
                         FileHandle *&rfh)
    {
        IndexManager *im = quickManager();
        if (im->openIndex(fileName, index_no, ih, rfh) != 0) {
            delete im->bpman->fileManager;
            delete im->bpman;
            delete im;
            return 1;
        }
        delete im;
        return 0;
    }
    static IndexManager *quickManager()
    {
        auto fm = new FileManager();
        auto bpm = new BufPageManager(fm);
        return new IndexManager(bpm);
    }
    static void quickRecycleManager(IndexManager *rm)
    {
        delete rm->bpman->fileManager;
        delete rm->bpman;
        delete rm;
    }
    static int quickClose(IndexHandle *fh)
    {
        IndexManager::closeIndex(*fh);
        BufPageManager *bpm = fh->getBpman();
        FileManager *fm = bpm->fileManager;
        delete fh;
        delete bpm;
        delete fm;
        return 0;
    }
    static char *makeKey(int N, void *dptr, int *offsets, int *lengths,
                         AttrTypeAtom *types, const RID &rid, int &errCode);

private:
    char *actualIndexFileName(const char *file_name, int index_no)
    {
        char *newFileName = new char[strlen(file_name) + 9];
        // index no range: [0, 65536) i.e. < 16^4
        sprintf(newFileName, "%s.%04x.i", file_name, index_no);
        return newFileName;
    }
};

struct IndexHeaderPage {
    AttrType attrType;
    int attrLen;
    int totalPages;
    int rootPage;
    int nextEmptyPage;
    IndexHeaderPage()
        : attrType(TYPE_CHAR), attrLen(0), totalPages(0), rootPage(0),
          nextEmptyPage(0)
    {
    }
    IndexHeaderPage(AttrType at, int al, int tp, int rp, int nep)
        : attrType(at), attrLen(al), totalPages(tp), rootPage(rp),
          nextEmptyPage(nep)
    {
    }
};
