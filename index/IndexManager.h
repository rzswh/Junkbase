#pragma once

#include "../filesystem/bufmanager/BufPageManager.h"
#include "../def.h"
#include "IndexHandle.h"
#include <cstring>
#include <vector>

using std::vector;

class IndexManager
{
    BufPageManager * bpman;
public:
    IndexManager(BufPageManager * man);
    int createIndex(const char * file_name, int index_no, AttrType attr_type, int attr_len, vector<int> & attr_len_arr);
    int openIndex(const char * file_name, int index_no, IndexHandle *& ih);
    int closeIndex(IndexHandle & ih);
    int deleteIndex(const char * file_name, int index_no);
private:
    char * actualIndexFileName(const char * file_name, int index_no) {
        char * newFileName = new char[strlen(file_name) + 7];
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
    IndexHeaderPage() : attrType(TYPE_CHAR), attrLen(0), totalPages(0), rootPage(0), nextEmptyPage(0) {}
    IndexHeaderPage(AttrType at, int al, int tp, int rp, int nep) 
        : attrType(at), attrLen(al), totalPages(tp), rootPage(rp), nextEmptyPage(nep) {}
};
