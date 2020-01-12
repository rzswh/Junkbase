#pragma once
#include "../def.h"
#include "../index/IndexHandle.h"
#include "../index/IndexManager.h"
#include "../recman/RID.h"

int indexTryInsert(IndexHandle *ih, bool isKey, int N, int *offsets,
                   int *lengths, AttrTypeAtom *types, void *buf, const RID &rid,
                   bool recover);

int indexTryDelete(IndexHandle *ih, bool isKey, int N, int *offsets,
                   int *lengths, AttrTypeAtom *types, void *buf, const RID &rid,
                   bool recover);

int indexCheckPresent(int refIndex, IndexHandle *ih, int N, int *offsets,
                      int *lengths, AttrTypeAtom *types, void *buf,
                      const RID &rid);

int indexCheckAbsent(int refIndex, IndexHandle *ih, int N, int *offsets,
                     int *lengths, AttrTypeAtom *types, void *buf,
                     const RID &rid);

typedef decltype(indexTryInsert) IdxOps;
typedef decltype(indexCheckPresent) IdxChk;

class IndexPreprocessingData
{
private:
    IndexManager *indman;
    vector<IndexHandle *> ihs;
    vector<bool> isKey;
    vector<int> refIndexNo;
    vector<IndexHandle *> refIndexHandle;
    vector<FileHandle *> refFileHandle;
    vector<int> count;
    vector<int *> selOffsets;
    vector<int *> selLens;
    vector<AttrTypeAtom *> selAttrTypes;

public:
    IndexPreprocessingData() {}
    ~IndexPreprocessingData();
    void add(IndexHandle *ih, bool ik, int refIndNo, IndexHandle *rfih,
             FileHandle *rffh, int c, int *off, int *len, AttrTypeAtom *type)
    {
        ihs.push_back(ih);
        isKey.push_back(ik);
        refIndexNo.push_back(refIndNo);
        refIndexHandle.push_back(rfih);
        refFileHandle.push_back(rffh);
        count.push_back(c);
        selOffsets.push_back(off);
        selLens.push_back(len);
        selAttrTypes.push_back(type);
    }
    void setIndexManager(IndexManager *ind) { indman = ind; }

    int accept(void *buf, const RID &rid, IdxOps callback, IdxChk refCheck);
};

int doForEachIndex(const char *tableName, FileHandle *fht,
                   IndexPreprocessingData *&prep);
