#pragma once
#include "../def.h"
#include "../recman/RID.h"
#include <cstring>

class IndexHandle;
class BPlusTreeLeafNode;

/**
 * Memory Mapping:
 * |----------------------------------+----------------------|
 * | type(0 = leaf, 1 = inner, 1 bit) | father node(31 bits) |
 * |----------------------------------+----------------------|
 * |    pointers to leaf ((Capacity * 4) bytes) for inner    |
 * | or pointers to record ((Capacity * 8) bytes) for leaf   |
 * |                          ...                            |
 * |---------------------------------------------------------|
 * |    attribute values ((Capacity - 1) * attrLen bytes)    |
 * |                          ...                            |
 * |---------------------------------------------------------|
 */
class BPlusTreeNode
{
public:
    const int Capacity;
    int pageID;
    static const int SEARCH_NOT_FOUND = 1;

protected:
    int fa;
    int size;
    char *attrVals;
    IndexHandle *ih;

protected:
    virtual BPlusTreeNode *_newNode() = 0;

public:
    /**
     * @param   pos     Attribute position.
     * @param   mode    Indicates the position of the node to be removed.
     *                  110 indicates remove attr[pos] & node[pos]
     *                  011 indicates remove attr[pos] & node[pos+1]
     */
    virtual void allocate(int pos, int mode = 110) = 0;
    /**
     * Remove one node and its adjacent attribute value.
     * @param   pos     Attribute position.
     * @param   mode    Indicates the position of the node to be removed.
     *                  110 indicates remove attr[pos] & node[pos]
     *                  011 indicates remove attr[pos] & node[pos+1]
     */
    virtual void shrink(int pos, int mode = 110) = 0;
    /**
     * Move data from this[st, st + len) to dest[st_d, st_d + len).
     * Invoke this between objects of same subtype. -1 is returned if different
     * subtypes.
     */
    virtual int transfer(BPlusTreeNode *dest, int st, int st_d, int len) = 0;
    virtual int clear(int st, int len) = 0;
    virtual void split(BPlusTreeNode *&newnode, char *attrVal);

public:
    BPlusTreeNode(int fa, int pid, int cap, IndexHandle *ih);
    int getSize() const { return size; }
    int getValue(int i, char *buf) const; // copy attrVals[i] -> buf
    const char *getValueAddr(int i) const;
    int setValue(int i, const char *buf);
    virtual int getEntry(int i, char *buf) const = 0;
    virtual int setEntry(int i, const char *buf) = 0;

    /**
     * Return the first position that attrValues[pos] >= val.
     */
    int find(const char *val) const;
    virtual int search(char *data, RID &rid, BPlusTreeLeafNode *&leaf,
                       int &pos) = 0;
    int search(char *data, RID &rid, int &pid, int &pos);
    virtual BPlusTreeLeafNode *begin() = 0;

    bool full() const { return size == Capacity; }
    void flush();

    virtual bool insert(BPlusTreeNode *&newnode, char *attrVal,
                        const RID &rid) = 0;
    virtual int remove(const char *attrVal) = 0;
    virtual void toBytes(char *buf) = 0;
    virtual ~BPlusTreeNode() = 0;
    // insert
    static BPlusTreeNode *createFromBytes(IndexHandle *ih, int pos,
                                          char *buf = nullptr);

    friend class IndexHandle;
    friend class BPlusTreeInnerNode;

    virtual void debug() = 0;
};

class BPlusTreeInnerNode : public BPlusTreeNode
{
    int *nodeIndex;
    BPlusTreeNode **child_ptr;

protected:
    BPlusTreeNode *_newNode() override;

public:
    void allocate(int pos, int mode = 110) override;
    void shrink(int pos, int mode = 110) override;
    int transfer(BPlusTreeNode *dest, int st, int st_d, int len) override;
    int clear(int st, int len) override;

public:
    BPlusTreeInnerNode(int fa, int pid, IndexHandle *ih);

    int getEntry(int i, char *buf) const override
    {
        *((int *)buf) = nodeIndex[i];
    }
    int setEntry(int i, const char *buf) override
    {
        nodeIndex[i] = *((int *)buf);
    }
    int &child(int i);
    /**
     * Add an element to child[pos]. The element is taken from its neighbor.
     */
    void rotate(int pos, int direction);
    /**
     * Merge child[pos] and child[pos+1] into a single node.
     * Make sure child[pos+1] exist and child[pos]->size + child[pos+1]->size <
     * Capacity before invoking.
     */
    void mergeChild(int pos);

    /**
     * Return 1(SEARCH_NOT_FOUND) if not found.
     */
    int search(char *data, RID &rid, BPlusTreeLeafNode *&leaf,
               int &pos) override;
    bool insert(BPlusTreeNode *&newnode, char *attrVal,
                const RID &rid) override;
    int remove(const char *attrVal) override;
    void toBytes(char *buf) override;
    BPlusTreeNode *getNodePtr(int pos);
    BPlusTreeLeafNode *begin() override;

    void recycleWhole();
    ~BPlusTreeInnerNode();

    friend class BPlusTreeNode;
    friend class IndexHandle;

    virtual void debug();
};

class BPlusTreeLeafNode : public BPlusTreeNode
{
    RID *dataPtr;
    int nextPageID;

protected:
    BPlusTreeNode *_newNode() override;

public:
    void allocate(int pos, int mode = 110) override;
    void shrink(int pos, int mode = 110) override;
    int transfer(BPlusTreeNode *dest, int st, int st_d, int len) override;
    int clear(int st, int len) override;

public:
    BPlusTreeLeafNode(int fa, int pid, IndexHandle *ih);
    RID &data(int i);
    BPlusTreeLeafNode *begin() override;
    BPlusTreeLeafNode *next();
    int getEntry(int i, char *buf) const override
    {
        memcpy(buf, dataPtr + i, sizeof(RID));
    }
    int setEntry(int i, const char *buf) override
    {
        memcpy(dataPtr + i, buf, sizeof(RID));
    }
    int search(char *data, RID &rid, BPlusTreeLeafNode *&leaf,
               int &pos) override;
    bool insert(BPlusTreeNode *&newnode, char *attrVal,
                const RID &rid) override;
    int remove(const char *attrVal) override;
    void toBytes(char *buf) override;
    ~BPlusTreeLeafNode();

    static void initialize(BPlusTreeLeafNode *node)
    {
        node->dataPtr[0] = RID(-1, -1);
        node->size++;
        node->flush();
    }

    friend class BPlusTreeNode;
    friend void BPlusTreeInnerNode::mergeChild(int);

    virtual void debug();
};

class BPlusStatusHelper
{
    static bool duplicatedFlag;

public:
    static void setDuplicated() { duplicatedFlag = true; }
    static void clearDuplicated() { duplicatedFlag = false; }
    static bool getDuplicated() { return duplicatedFlag; }
};
