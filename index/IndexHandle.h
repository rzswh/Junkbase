#pragma once 

#include "../recman/RID.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "../def.h"

class BPlusTreeNode;

class IndexHandle {

    int file_id;
    BufPageManager * bpman;

    int rootPage; 
    int totalPage;
    int nextEmptyPage;
    void updateHeaderPage();

    BPlusTreeNode * tree_root;
public:
    const AttrType attrType;
    const int attrLen;

    IndexHandle(int fid, BufPageManager * pm, AttrType attrType, int attrLen, int rootPage, int totalPage, int nextEmptyPage);
    int insertEntry(const char * d_ptr, const RID & rid_ret);
    int deleteEntry(const char * d_ptr, const RID & rid_ret);
    // void flush();

    char * getPage(int pos, int &index) { 
        return (char*)bpman->getPage(file_id, pos, index); 
    }
    int getFileId() const { return file_id; }
    BufPageManager * getBpman() const { return bpman; }
    int allocatePage();
    void recyclePage(int pid);

    ~IndexHandle();

    static IndexHandle * createFromFile(int fid, BufPageManager * pm);
};