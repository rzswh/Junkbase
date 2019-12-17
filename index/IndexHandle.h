#pragma once 

#include "../recman/RID.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "../def.h"
#include "../CompOp.h"
#include <vector>

class BPlusTreeNode;
class BPlusTreeLeafNode;
class FileHandle;

class IndexHandle {

    int file_id;
    BufPageManager * bpman;

    int rootPage; 
    int totalPage;
    int nextEmptyPage;
    void updateHeaderPage();

    BPlusTreeNode * tree_root;
    FileHandle * varSource;
    std::vector<BPlusTreeNode *> pool;
    bool duplicate;
    const int keyNum;
public:
    const AttrType attrType;
    const int attrLen;
    int* attrLenArr;
    int getFileId() const { return file_id; }
    BufPageManager * getBpman() const { return bpman; }

    IndexHandle(int fid, BufPageManager * pm, 
            AttrType attrType, int* attrLens, 
            int rootPage, int totalPage, 
            int nextEmptyPage, 
            FileHandle * varRefFH,
            bool unique = true);
    int insertEntry(const char * d_ptr, const RID & rid_ret);
    int deleteEntry(const char * d_ptr, const RID & rid_ret);
    // void flush();
    static IndexHandle * createFromFile(int fid, BufPageManager * pm, FileHandle * fh);

    ~IndexHandle();

    void debug();

private:
    /**
     * The ONLY safe way to convert a pageID to a node.
     */ 
    BPlusTreeNode * getNode(int pos);
    /**
     * Any node created outside the class should add that node by invoking this method.
     */ 
    void addNode(BPlusTreeNode * n);
    char * getPage(int pos, int &index) { 
        return (char*)bpman->getPage(file_id, pos, index); 
    }
    /**
     * Create a new empty page, by adding 1 to totalPage
     */ 
    int allocatePage();
    /**
     * Mark a page unused. It will update the list of empty page, and be removed from
     * the node pool. If you own a pointer to a node, you have to manually delete it 
     * after/before recyclePage is invoked.
     */ 
    void recyclePage(int pid);
    friend class BPlusTreeNode;
    friend class BPlusTreeInnerNode;
    friend class BPlusTreeLeafNode;
    // Scan
public:
    class iterator {
        iterator(
            IndexHandle * ih, 
            BPlusTreeLeafNode * node, 
            int pos, 
            Operation compOp, void *value,
            bool oneAfter = false);
        BPlusTreeLeafNode * leaf;
        int position;
        IndexHandle * ih;
        Operation comp_op;
        void * value;
        bool checkEnd() const;
        void nextPos();
    public:
        RID operator*() const;
        bool end() const;
        iterator& operator++();
        iterator operator++(int) {
            iterator ret = *this;
            this->operator++();
            return ret;
        }
        bool operator==(const iterator &it) const;
        bool operator!=(const iterator &it) const { return !operator==(it); }
        friend class IndexHandle;
    };
    IndexHandle::iterator findEntry(Operation compOp, void * val);
};