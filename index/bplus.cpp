#include <cstring>
#include <cstdio>
#include "bplus.h"
#include "../filesystem/utils/pagedef.h"
#include "IndexHandle.h"
#include "../errors.h"
#include "../CompOp.h"

BPlusTreeNode::BPlusTreeNode(int fa, int pid, int cap, IndexHandle * ih) 
        : fa(fa), pageID(pid), size(0), Capacity(cap), ih(ih) 
{
    attrVals = new char[ih->attrLen * cap];
}

BPlusTreeNode::~BPlusTreeNode() {
    delete [] attrVals;
}

int BPlusTreeNode::getValue(int i, char * buf) const {
    if (i < 0 || i >= Capacity - 1)
        return OUT_OF_BOUND;
    memcpy(buf, attrVals + i * ih->attrLen, ih->attrLen);
    return 0;
}

int BPlusTreeNode::setValue(int i, const char * buf) {
    if (i < 0 || i >= Capacity - 1)
        return OUT_OF_BOUND;
    memcpy(attrVals + i * ih->attrLen, buf, ih->attrLen);
    return 0;
}

const char * BPlusTreeNode::getValueAddr(int i) const {
    return attrVals + i * ih->attrLen;
}

int BPlusTreeNode::find(const char * val) const {
    for (int i = 0; i < size - 1; i++) {
        if (!Operation(Operation::LESS, ih->attrType).check(attrVals + i * ih->attrLen, val, ih->attrLen))
            return i;
    }
    return size - 1;
}

void BPlusTreeNode::split(BPlusTreeNode *& newnode, char * attrVal) {
    if (size <= Capacity) return;
    BPlusTreeNode * nn = _newNode();
    int ls = size / 2, rs = size - ls;
    // copy its 'attrVal' field
    memcpy(attrVal, attrVals + (ls - 1) * ih->attrLen, ih->attrLen);
    transfer(nn, 0, 0, ls);
    transfer(this, ls, 0, rs);
    clear(rs, ls);
    nn->size = ls;
    size = rs;
    newnode = nn;
}

void BPlusTreeNode::flush()  {
    int cache_id;
    toBytes(ih->getPage(pageID, cache_id));
    ih->getBpman()->markDirty(cache_id);
}
BPlusTreeNode * BPlusTreeNode::createFromBytes(IndexHandle * ih, int pid, char * buf) {
    if (buf == nullptr) {
        int __id;
        buf = ih->getPage(pid, __id);
    }
    unsigned int fa;
    fa = ((int*)buf)[0];
    if (fa >> 31) {
        fa = fa ^ (fa >> 31 << 31);
        BPlusTreeInnerNode * node = new BPlusTreeInnerNode(fa, pid, ih);
        int c = node->Capacity;
        memcpy(node->nodeIndex, buf + 4, 4 * c);
        memcpy(node->attrVals, buf + 4 + 4 * c, ih->attrLen * (c - 1));
        node->size = 0;
        while (node->child(node->size)) node->size ++;
        return node;
    }
    else {
        BPlusTreeLeafNode * node = new BPlusTreeLeafNode(fa, pid, ih);
        int c = node->Capacity;
        node->nextPageID = ((int*)buf)[1];
        memcpy(node->dataPtr, buf + 8, sizeof(RID) * c);
        memcpy(node->attrVals, buf + 8 + sizeof(RID) * c, ih->attrLen * (c - 1));
        node->size = 0;
        while (!(node->dataPtr[node->size] == RID(0, 0)) ) node->size ++;
        return node;
    }
}

int BPlusTreeNode::search(char * data, RID & rid, int & pid, int & pos) {
    BPlusTreeLeafNode * leaf;
    int code = search(data, rid, leaf, pos);
    pid = leaf->pageID;
    return code;
}

///////////////////////////////////////////////////////

BPlusTreeInnerNode::BPlusTreeInnerNode(int fa, int pid, IndexHandle * ih)
        : BPlusTreeNode(fa, pid, (PAGE_SIZE - 4) / (ih->attrLen + 4), ih) 
{
    nodeIndex = new int[Capacity + 1];
    child_ptr = new BPlusTreeNode*[Capacity + 1];
}

BPlusTreeInnerNode::~BPlusTreeInnerNode() {
    delete [] nodeIndex;
    delete [] child_ptr;
}

BPlusTreeNode * BPlusTreeInnerNode::_newNode() { 
    auto ret = new BPlusTreeInnerNode(fa, ih->allocatePage(), ih); 
    ih->addNode(ret);
    return ret;
}

void BPlusTreeInnerNode::toBytes(char * buf) {
    ((unsigned int*) buf)[0] = fa | (1u << 31);
    memcpy(buf + 4, nodeIndex, 4 * Capacity);
    memcpy(buf + 4 + 4 * Capacity, attrVals, ih->attrLen * (Capacity - 1));
}

int & BPlusTreeInnerNode::child(int i) {
    return nodeIndex[i];
}

BPlusTreeLeafNode * BPlusTreeInnerNode::begin() {
    return getNodePtr(0)->begin();
}

BPlusTreeNode * BPlusTreeInnerNode::getNodePtr(int pos) {
    if (pos < 0 || pos >= size) return nullptr;
    if (child_ptr[pos]) return child_ptr[pos];
    return child_ptr[pos] = ih->getNode(nodeIndex[pos]);
}

void BPlusTreeInnerNode::allocate(int pos) {
    const int ptrs = sizeof(BPlusTreeNode*);
    int L = ih->attrLen;
    memmove(nodeIndex + (pos + 1),    nodeIndex + pos,    4 * (size - pos));
    memmove(attrVals  + (pos + 1) * L,attrVals  + pos * L,L * (size - pos));
    memmove(child_ptr + (pos + 1),    child_ptr + pos, ptrs * (size - pos));
    size ++;
}

void BPlusTreeInnerNode::shrink(int pos) {
    const int ptrs = sizeof(BPlusTreeNode*);
    int L = ih->attrLen;
    memmove(nodeIndex + pos,    nodeIndex + (pos + 1),    4 * (size - pos));
    memmove(attrVals  + pos * L,attrVals  + (pos + 1) * L,L * (size - pos));
    memmove(child_ptr + pos,    child_ptr + (pos + 1), ptrs * (size - pos));
    size --;
    nodeIndex[size] = 0;
    child_ptr[size] = nullptr;
}

int BPlusTreeInnerNode::transfer(BPlusTreeNode * _dest, int st, int st_d, int len) {
    BPlusTreeInnerNode * dest = dynamic_cast<BPlusTreeInnerNode *>(_dest);
    if (dest == nullptr) return -1;
    int L = ih->attrLen;
    memmove(dest->attrVals + st_d * L, attrVals + st * L, (len - 1) * L);
    memmove(dest->nodeIndex + st_d, nodeIndex + st, len * sizeof(int));
    memmove(dest->child_ptr + st_d, child_ptr + st, len * sizeof(BPlusTreeNode *));
    return 0;
}

int BPlusTreeInnerNode::clear(int st, int len)  {
    memset(nodeIndex + st, 0, sizeof(int) * len);
    memset(child_ptr + st, 0, sizeof(BPlusTreeInnerNode*) * len);
}

void BPlusTreeInnerNode::rotate(int pos, int direction) {
    BPlusTreeNode * mid = getNodePtr(pos);
    int L = ih->attrLen;
    char buf[8];
    if (direction == 0) // rotate left 
    {
        BPlusTreeNode * left = getNodePtr(pos - 1);
        mid->allocate(0);
        mid->setValue(0, attrVals + (pos - 1) * L);
        setValue(pos-1, left->getValueAddr(left->size - 1));
        left->getEntry(left->getSize() - 1, buf);
        mid->setEntry(0, buf);
        left->shrink(left->getSize() - 1);
    } else {
        BPlusTreeNode * right = getNodePtr(pos + 1);
        mid->allocate(mid->getSize());
        mid->setValue(mid->getSize() - 1, attrVals + pos * L);
        setValue(pos, right->getValueAddr(0));
        right->getEntry(0, buf);
        mid->setEntry(mid->getSize() - 1, buf);
        right->shrink(0);
    }
}

void BPlusTreeInnerNode::mergeChild(int pos) {
    BPlusTreeNode * left = getNodePtr(pos), * right = getNodePtr(pos + 1);
    const int L = ih->attrLen;
    left->setValue(left->getSize(), attrVals + pos * L);
    right->transfer(left, 0, left->getSize(), right->getSize());
    left->size += right->getSize();
    shrink(pos + 1);
    if (dynamic_cast<BPlusTreeLeafNode*>(left) && dynamic_cast<BPlusTreeLeafNode *>(right)) {
        dynamic_cast<BPlusTreeLeafNode*>(left)->nextPageID = dynamic_cast<BPlusTreeLeafNode *>(right)->nextPageID;
    }
    ih->recyclePage(right->pageID);
    delete right;
}

int BPlusTreeInnerNode::search(char * data, RID & rid, BPlusTreeLeafNode *& leaf, int & pos) {
    int p = find(data);
    BPlusTreeNode * ch = getNodePtr(p);
    return ch->search(data, rid, leaf, pos);
}

bool BPlusTreeInnerNode::insert(BPlusTreeNode *& newnode, char * attrVal, const RID & rid) {
    int pos = find(attrVal);
    if (getNodePtr(pos)->insert(newnode, attrVal, rid)) return true;
    allocate(pos);
    memcpy(attrVals + pos * ih->attrLen, attrVal, ih->attrLen);
    nodeIndex[pos] = newnode->pageID;
    child_ptr[pos] = newnode;
    // mark dirty
    flush();
    // if the node overflows, split it up
    if (size <= Capacity) return true;
    split(newnode, attrVal);
    // mark dirty newnode
    newnode->flush();
    return false;
}

int BPlusTreeInnerNode::remove(const char * attrVal) {
    int pos = find(attrVal);
    BPlusTreeNode * mid = getNodePtr(pos);
    int code = mid->remove(attrVal);
    if (code >= 0) return code;
    if (mid->getSize() >= (mid->Capacity + 1) / 2) 
        return 0; // no underflow on this node
    // solve underflow
    BPlusTreeNode * left = pos ? getNodePtr(pos - 1) : nullptr;
    BPlusTreeNode * right = pos + 1 < size ? getNodePtr(pos + 1) : nullptr;
    if (left && left->getSize() > (1 + left->Capacity) / 2) {
        rotate(pos, 0);
        left->flush();
    }
    else if (right && right->getSize() > (1 + right->Capacity) / 2) {
        rotate(pos, 1);
        right->flush();
    }
    else {
        // assume left or right exists
        // always hold except root node
        mergeChild(pos - (left != nullptr));
        flush();
        getNodePtr(pos - (left != nullptr))->flush();
        return -1;
    }
    mid->flush();
    flush();
    return 0; // size of this node stays unchanged
}

void BPlusTreeInnerNode::recycleWhole () {
    for (int i = 0; i <= Capacity; i++) 
        if (child_ptr[i] != nullptr) {
            if (dynamic_cast<BPlusTreeInnerNode*>(child_ptr[i]) != nullptr)
                dynamic_cast<BPlusTreeInnerNode*>(child_ptr[i])->recycleWhole();
            delete child_ptr[i];
        }
}

///////////////////////////////////////////////////////

BPlusTreeLeafNode::BPlusTreeLeafNode(int fa, int pid, IndexHandle * ih)
        : BPlusTreeNode(fa, pid, (PAGE_SIZE - 8) / (ih->attrLen + sizeof(RID)), ih) 
{
    dataPtr = new RID[Capacity + 1];
}

BPlusTreeLeafNode::~BPlusTreeLeafNode() {
    delete [] dataPtr;
}

BPlusTreeNode * BPlusTreeLeafNode::_newNode() { 
    auto ret = new BPlusTreeLeafNode(fa, ih->allocatePage(), ih); 
    ih->addNode(ret);
    return ret;
}

void BPlusTreeLeafNode::toBytes(char * buf) {
    ((int*) buf)[0] = fa;
    ((int*) buf)[1] = nextPageID;
    memcpy(buf + 8, dataPtr, sizeof(RID) * Capacity);
    memcpy(buf + 8 + sizeof(RID) * Capacity, attrVals, ih->attrLen * (Capacity - 1));
}

RID& BPlusTreeLeafNode::data(int i) {
    return dataPtr[i];
}

BPlusTreeLeafNode * BPlusTreeLeafNode::begin() {
    return this;
}

BPlusTreeLeafNode * BPlusTreeLeafNode::next() {
    return nextPageID ? dynamic_cast<BPlusTreeLeafNode*>(ih->getNode(nextPageID)) : nullptr;
}

void BPlusTreeLeafNode::allocate(int pos) {
    const int RS = sizeof(RID);
    int L = ih->attrLen;
    memmove(dataPtr + (pos + 1), dataPtr + pos, RS * (size - pos));
    memmove(attrVals + (pos + 1) * L, attrVals + pos * L, L * (size - pos));
    size ++;
}

void BPlusTreeLeafNode::shrink(int pos) {
    const int RS = sizeof(RID);
    int L = ih->attrLen;
    memmove(dataPtr + pos, dataPtr + (pos + 1), RS * (size - pos));
    memmove(attrVals + pos * L, attrVals + (pos + 1) * L, L * (size - pos));
    size --;
}

int BPlusTreeLeafNode::transfer(BPlusTreeNode * _dest, int st, int st_d, int len) {
    BPlusTreeLeafNode * dest = dynamic_cast<BPlusTreeLeafNode *>(_dest);
    if (dest == nullptr) return -1;
    int L = ih->attrLen;
    memcpy(dest->attrVals + st_d * L, attrVals + st * L, (len - 1) * L);
    L = sizeof(RID);
    memcpy(dest->dataPtr + st_d, dataPtr + st, len * L);
    return 0;
}

int BPlusTreeLeafNode::clear(int st, int len)  {
    memset(dataPtr + st, 0, sizeof(RID) * len);
}

int BPlusTreeLeafNode::search(char * data, RID & rid, BPlusTreeLeafNode *& leaf, int & pos) {
    int p = find(data);
    const int L = ih->attrLen;
    rid = this->data(p);
    leaf = this;
    pos = p;
    return 0;
}

bool BPlusTreeLeafNode::insert(BPlusTreeNode *& newnode, char * attrVal, const RID & rid) {
    int pos = find(attrVal);
    allocate(pos);
    memcpy(attrVals + pos * ih->attrLen, attrVal, ih->attrLen);
    this->data(pos) = rid;
    if (size <= Capacity)  {
        flush();
        return true;
    }
    split(newnode, attrVal);
    dynamic_cast<BPlusTreeLeafNode*> (newnode)->nextPageID = pageID;
    flush();
    newnode->flush();
    return false;
}

int BPlusTreeLeafNode::remove(const char * attrVal) {
    if (size == 1)
        return NO_ENTRY;
    const int L = ih->attrLen;
    int pos = find(attrVal);
    if (!Operation(Operation::EQUAL, ih->attrType).check(attrVal, attrVals + pos * L, L))
        return BPlusTreeNode::SEARCH_NOT_FOUND;
    shrink(pos);
    flush();
    return size >= (Capacity + 1) / 2 ? 0 : -1;
}
