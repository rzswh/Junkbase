#include "IndexHandle.h"
#include "IndexManager.h"
#include "bplus.h"
#include <cassert>

IndexHandle * IndexHandle::createFromFile(int fid, BufPageManager * pm) {
    IndexHeaderPage header;
    int __index;
    char * buf = (char*)(pm->getPage(fid, 0, __index));
    memcpy(&header, buf, sizeof(header));
    IndexHandle * ret = new IndexHandle(fid, pm, header.attrType, header.attrLen, header.rootPage, header.totalPages, header.nextEmptyPage);
    return ret;
}

IndexHandle::IndexHandle(int fid, BufPageManager * pm, AttrType attrType, int attrLen, int rootPage, int totalPage, int nextEmptyPage)
        : file_id(fid), bpman(pm), attrType(attrType), attrLen(attrLen), rootPage(rootPage), totalPage(totalPage), nextEmptyPage(nextEmptyPage) 
{
    int __index;
    char * buf = (char*) pm->getPage(fid, rootPage, __index);
    tree_root = BPlusTreeNode::createFromBytes(this, rootPage, buf);
    pool.push_back(tree_root);
    // new index tree! initialize
    if (tree_root->getSize() == 0) { 
        BPlusTreeLeafNode * leaf_root = dynamic_cast<BPlusTreeLeafNode*> (tree_root);
        assert(leaf_root != nullptr);
        BPlusTreeLeafNode::initialize(leaf_root);
    }
}

BPlusTreeNode * IndexHandle::getNode(int pos) {
    if (pos >= pool.size()) pool.resize(pos + 1);
    if (pool[pos] != nullptr) return pool[pos];
    return pool[pos] = BPlusTreeNode::createFromBytes(this, pos);
}

void IndexHandle::addNode(BPlusTreeNode * n) { 
    if (pool.size() <= n->pageID) pool.resize(n->pageID + 1);
    pool[n->pageID] = n; 
}

void IndexHandle::updateHeaderPage() {
    IndexHeaderPage header = IndexHeaderPage(attrType, attrLen, totalPage, rootPage, nextEmptyPage);
    int header_index;
    char * buf = (char*)(bpman->getPage(file_id, 0, header_index));
    memcpy(buf, &header, sizeof(header));
    bpman->markDirty(header_index);
}

int IndexHandle::insertEntry(const char * d_ptr, const RID & rid)
{
    char val[1 + attrLen];
    memcpy(val, d_ptr, attrLen);
    BPlusTreeNode * newNode = nullptr, * oldNode = tree_root;
    if (tree_root->insert(newNode, val, rid)) return 0;
    // Increase level number
    BPlusTreeInnerNode * new_root = new BPlusTreeInnerNode(0, allocatePage(), this);
    new_root->size = 2;
    memcpy(new_root->attrVals, val, attrLen);
    new_root->nodeIndex[1] = oldNode->pageID;
    new_root->nodeIndex[0] = newNode->pageID;
    new_root->child_ptr[1] = oldNode;
    new_root->child_ptr[0] = newNode;
    oldNode->fa = newNode->fa = new_root->pageID;
    tree_root = new_root;
    newNode->flush();
    oldNode->flush();
    new_root->flush();
    tree_root = new_root;
    rootPage = new_root->pageID;
    updateHeaderPage();
    return 0;
}

int IndexHandle::deleteEntry(const char * d_ptr, const RID & rid_ret)
{
    // TODO:if root only has one child, remove the topmost node
    int code = tree_root->remove(d_ptr);
    if (code >= 0) return code;
    // size of root node decreased to 1
    if (code == -1 && tree_root->getSize() == 1) {
        BPlusTreeNode * newRoot;
    }
}

int IndexHandle::allocatePage() {
    if (nextEmptyPage == totalPage) {
        nextEmptyPage = ++ totalPage;
    } else {
        int _id;
        char * buf = getPage(nextEmptyPage, _id);
        nextEmptyPage = *((int*)buf);
    }
    updateHeaderPage();
    return totalPage - 1;
}

void IndexHandle::recyclePage(int pid) {
    int _id;
    char * buf = getPage(pid, _id);
    *((int*)buf) = nextEmptyPage;
    bpman->markDirty(_id);
    nextEmptyPage = pid;
    updateHeaderPage();
}

IndexHandle::~IndexHandle() {
    if (tree_root && dynamic_cast<BPlusTreeInnerNode*> (tree_root)) 
        dynamic_cast<BPlusTreeInnerNode*> (tree_root) -> recycleWhole();
    delete tree_root;
}
/*
void IndexHandle::flush() {
    bpman->close();
}
*/

IndexHandle::iterator IndexHandle::findEntry(Operation compOp, void * val) {
    int pos;
    BPlusTreeLeafNode * leaf;
    RID _rid;
    if (compOp.codeOp == Operation::EQUAL || compOp.codeOp == Operation::GREATER) {
        tree_root->search((char*)val, _rid, leaf, pos);
        bool next = compOp.codeOp == Operation::GREATER &&
            Operation(Operation::EQUAL, attrType).check(leaf->getValueAddr(pos), val, attrLen);
        return IndexHandle::iterator(this, leaf, pos, compOp, val, next);
    } else {
        return IndexHandle::iterator(this, tree_root->begin(), 0, compOp, val);
    }
}


IndexHandle::iterator::iterator(
    IndexHandle * ih, 
    BPlusTreeLeafNode * leaf, 
    int pos, 
    Operation compOp, 
    void *value, 
    bool oneAfter)
        : ih(ih), leaf(leaf), position(pos), comp_op(compOp), value(value) {
    if (oneAfter) nextPos();
}

bool IndexHandle::iterator::end() const {
    return checkEnd();
}

RID IndexHandle::iterator::operator*() const  { 
    RID ret;
    leaf->getEntry(position, (char*)(&ret)); 
    return ret;
}

bool IndexHandle::iterator::checkEnd() const {
    return leaf == nullptr || leaf->next() == nullptr && position == leaf->getSize() - 1
        || !comp_op.check(leaf->getValueAddr(position), value, ih->attrLen);
}

void IndexHandle::iterator::nextPos() {
    position ++;
    if (position == leaf->size) {
        position = 0;
        leaf = leaf->next();
    }
}

IndexHandle::iterator& IndexHandle::iterator::operator++() {
    nextPos();
    return *this;
}

bool IndexHandle::iterator::operator==(const iterator &it) const {
    return it.leaf->pageID == leaf->pageID && it.position == position;
}