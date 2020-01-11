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
typedef decltype(indexTryInsert) IdxOps;

class IndexPreprocessingData
{
private:
    IndexManager *indman;
    vector<IndexHandle *> ihs;
    vector<bool> isKey;
    vector<int> count;
    vector<int *> selOffsets;
    vector<int *> selLens;
    vector<AttrTypeAtom *> selAttrTypes;

public:
    IndexPreprocessingData() {}
    ~IndexPreprocessingData()
    {
        for (auto i : ihs) {
            IndexManager::closeIndex(*i);
            delete i;
        }
        IndexManager::quickRecycleManager(indman);
        for (auto i : selOffsets)
            delete[] i;
        for (auto i : selLens)
            delete[] i;
        for (auto i : selAttrTypes)
            delete[] i;
    }
    void add(IndexHandle *ih, bool ik, int c, int *off, int *len,
             AttrTypeAtom *type)
    {
        ihs.push_back(ih);
        isKey.push_back(ik);
        count.push_back(c);
        selOffsets.push_back(off);
        selLens.push_back(len);
        selAttrTypes.push_back(type);
    }
    void setIndexManager(IndexManager *ind) { indman = ind; }

    int accept(void *buf, const RID &rid, IdxOps callback);
};

int doForEachIndex(const char *tableName, FileHandle *fht,
                   IndexPreprocessingData *&prep);
