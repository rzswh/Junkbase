#include "IndexHandle.h"
#include "../recman/FileHandle.h"
#include "IndexManager.h"
#include "bplus.h"
#include <cassert>

int getSum(const int *st, const int *en)
{
    int tot = 0;
    for (int i = 0; st + i < en; i++)
        tot += st[i];
    return tot;
}

IndexHandle *IndexHandle::createFromFile(int fid, BufPageManager *pm,
                                         FileHandle *fh)
{
    IndexHeaderPage header;
    int __index;
    char *buf = (char *)(pm->getPage(fid, 0, __index));
    memcpy(&header, buf, sizeof(header));
    const int keyNum = AttrTypeHelper::countKey(header.attrType);
    int *attrLens = new int[keyNum];
    memcpy(attrLens, buf + sizeof(IndexHeaderPage), sizeof(int) * keyNum);
    IndexHandle *ret =
        new IndexHandle(fid, pm, header.attrType, attrLens, header.rootPage,
                        header.totalPages, header.nextEmptyPage, fh, true);
    return ret;
}

IndexHandle::IndexHandle(int fid, BufPageManager *pm, AttrType attrType,
                         int *attrLens, int rootPage, int totalPage,
                         int nextEmptyPage, FileHandle *varRefFH, bool unique)
    : file_id(fid), bpman(pm),
      duplicate(AttrTypeHelper::getLowestKey(attrType) == TYPE_RID),
      attrType(attrType),
      /* attrType ^ (duplicate ? TYPE_RID << 3 *
         (AttrTypeHelper::countKey(attrType)-1) : 0)*/
      attrLen(getSum(attrLens, attrLens + AttrTypeHelper::countKey(attrType))),
      attrLenArr(attrLens), varSource(varRefFH), rootPage(rootPage),
      totalPage(totalPage), nextEmptyPage(nextEmptyPage),
      keyNum(AttrTypeHelper::countKey(attrType))
{
    int __index;
    char *buf = (char *)pm->getPage(fid, rootPage, __index);
    tree_root = BPlusTreeNode::createFromBytes(this, rootPage, buf);
    pool.push_back(tree_root);
    // new index tree! initialize
    if (tree_root->getSize() == 0) {
        BPlusTreeLeafNode *leaf_root =
            dynamic_cast<BPlusTreeLeafNode *>(tree_root);
        assert(leaf_root != nullptr);
        BPlusTreeLeafNode::initialize(leaf_root);
    }
}

BPlusTreeNode *IndexHandle::getNode(int pos)
{
    if (pos >= pool.size()) pool.resize(pos + 1);
    if (pool[pos] != nullptr) return pool[pos];
    return pool[pos] = BPlusTreeNode::createFromBytes(this, pos);
}

void IndexHandle::addNode(BPlusTreeNode *n)
{
    if (pool.size() <= n->pageID) pool.resize(n->pageID + 1);
    pool[n->pageID] = n;
}

void IndexHandle::updateHeaderPage()
{
    IndexHeaderPage header =
        IndexHeaderPage(attrType, attrLen, totalPage, rootPage, nextEmptyPage);
    int header_index;
    char *buf = (char *)(bpman->getPage(file_id, 0, header_index));
    memcpy(buf, &header, sizeof(header));
    memcpy(buf + sizeof(header), attrLenArr, sizeof(int) * keyNum);
    bpman->markDirty(header_index);
}

int IndexHandle::insertEntry(const char *d_ptr, const RID &rid)
{
    char val[1 + attrLen];
    if (duplicate) {
        memcpy(val, d_ptr, attrLen - sizeof(RID));
        memcpy(val + (attrLen - sizeof(RID)), &rid, sizeof(RID));
    } else
        memcpy(val, d_ptr, attrLen);

    BPlusTreeNode *newNode = nullptr, *oldNode = tree_root;
    BPlusStatusHelper::clearDuplicated();
    if (tree_root->insert(newNode, val, rid))
        return BPlusStatusHelper::getDuplicated();
    // Increase level number
    BPlusTreeInnerNode *new_root =
        new BPlusTreeInnerNode(0, allocatePage(), this);
    pool.push_back(new_root);
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
    return BPlusStatusHelper::getDuplicated();
}

int IndexHandle::deleteEntry(const char *d_ptr, const RID &rid)
{
    char val[1 + attrLen];
    if (duplicate) {
        memcpy(val, d_ptr, attrLen - sizeof(RID));
        memcpy(val + (attrLen - sizeof(RID)), &rid, sizeof(RID));
    } else
        memcpy(val, d_ptr, attrLen);

    // TODO:if root only has one child, remove the topmost node
    int code = tree_root->remove(val);
    if (code >= 0) return code;
    // size of root node decreased to 1
    if (code == -1 && tree_root->getSize() == 1) {
        if (dynamic_cast<BPlusTreeInnerNode *>(tree_root) == nullptr) return 0;
        BPlusTreeInnerNode *root_inner =
            dynamic_cast<BPlusTreeInnerNode *>(tree_root);
        BPlusTreeNode *new_root = getNode(root_inner->child(0));
        new_root->fa = 0;
        rootPage = new_root->pageID;
        recyclePage(tree_root->pageID);
        delete tree_root;
        tree_root = new_root;
    }
}

int IndexHandle::allocatePage()
{
    if (nextEmptyPage == totalPage) {
        nextEmptyPage = ++totalPage;
    } else {
        int _id;
        char *buf = getPage(nextEmptyPage, _id);
        nextEmptyPage = *((int *)buf);
    }
    updateHeaderPage();
    return totalPage - 1;
}

void IndexHandle::recyclePage(int pid)
{
    int _id;
    char *buf = getPage(pid, _id);
    *((int *)buf) = nextEmptyPage;
    bpman->markDirty(_id);
    nextEmptyPage = pid;
    updateHeaderPage();
    pool[pid] = nullptr;
}

IndexHandle::~IndexHandle()
{
    // if (tree_root && dynamic_cast<BPlusTreeInnerNode*> (tree_root))
    //     dynamic_cast<BPlusTreeInnerNode*> (tree_root) -> recycleWhole();
    // delete tree_root;
    delete[] attrLenArr;
    for (auto i : pool)
        if (i != nullptr) delete i;
}
/*
void IndexHandle::flush() {
    bpman->close();
}
*/

IndexHandle::iterator IndexHandle::findEntry(Operation compOp, void *val)
{
    int pos;
    BPlusTreeLeafNode *leaf;
    RID _rid;
    if (compOp.codeOp == Operation::EQUAL ||
        compOp.codeOp == Operation::GREATER) {
        tree_root->search((char *)val, _rid, leaf, pos);
        bool next = compOp.codeOp == Operation::GREATER &&
                    Operation(Operation::EQUAL, attrType, varSource)
                        .check(leaf->getValueAddr(pos), val, attrLenArr);
        return IndexHandle::iterator(this, leaf, pos, compOp, val, next);
    } else {
        return IndexHandle::iterator(this, tree_root->begin(), 0, compOp, val);
    }
}

IndexHandle::iterator::iterator(IndexHandle *ih, BPlusTreeLeafNode *leaf,
                                int pos, Operation compOp, void *value,
                                bool oneAfter)
    : ih(ih), leaf(leaf), position(pos), comp_op(compOp), value(value)
{
    if (oneAfter) nextPos();
}

bool IndexHandle::iterator::end() const { return checkEnd(); }

RID IndexHandle::iterator::operator*() const
{
    RID ret;
    leaf->getEntry(position, (char *)(&ret));
    return ret;
}

bool IndexHandle::iterator::checkEnd() const
{
    return leaf == nullptr ||
           !comp_op.check(leaf->getValueAddr(position), value,
                          ih->attrLenArr) ||
           leaf->next() == nullptr && position == leaf->getSize() - 1;
}

void IndexHandle::iterator::nextPos()
{
    position++;
    if (position == leaf->size) {
        position = 0;
        leaf = leaf->next();
    }
}

IndexHandle::iterator &IndexHandle::iterator::operator++()
{
    nextPos();
    return *this;
}

bool IndexHandle::iterator::operator==(const iterator &it) const
{
    return it.leaf->pageID == leaf->pageID && it.position == position;
}

void IndexHandle::debug() { tree_root->debug(); }