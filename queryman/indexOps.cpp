
#include "indexOps.h"
#include "../errors.h"
#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"
#include "../sysman/sysman.h"
#include "../utils/helper.h"
#include <cassert>
#include <map>
#include <vector>
using std::map;

int indexTryInsert(IndexHandle *ih, bool isKey, int N, int *offsets,
                   int *lengths, AttrTypeAtom *types, void *buf, const RID &rid,
                   bool recover)
{
    int errCode = 0;
    char *key =
        IndexManager::makeKey(N, buf, offsets, lengths, types, rid, errCode);
    int totalLen = 0;
    for (int i = 0; i < N; i++)
        totalLen += lengths[i];
    assert(totalLen == ih->attrLen);
    if (!recover) {
        bool res = ih->insertEntry(key, rid);
        if (res && isKey) errCode = DUPLICATED_KEY;
    } else {
        ih->deleteEntry(key, rid);
    }
    delete[] key;
    return errCode;
}

int indexTryDelete(IndexHandle *ih, bool isKey, int N, int *offsets,
                   int *lengths, AttrTypeAtom *types, void *buf, const RID &rid,
                   bool recover)
{
    int errCode = 0;
    char *key =
        IndexManager::makeKey(N, buf, offsets, lengths, types, rid, errCode);
    int totalLen = 0;
    for (int i = 0; i < N; i++)
        totalLen += lengths[i];
    // assert(totalLen == ih->attrLen);
    if (recover) {
        ih->insertEntry(key, rid);
    } else {
        ih->deleteEntry(key, rid);
    }
    delete[] key;
    return errCode;
}

int indexCheckPresent(int refIndex, IndexHandle *ih, int N, int *offsets,
                      int *lengths, AttrTypeAtom *types, void *buf,
                      const RID &rid)
{
    // if being referenced, ignore this check
    if (refIndex < 0) return 0;
    int errCode = 0;
    char *key =
        IndexManager::makeKey(N, buf, offsets, lengths, types, rid, errCode);
    int totalLen = 0;
    for (int i = 0; i < N; i++)
        totalLen += lengths[i];
    errCode =
        ih->findEntry(Operation(Operation::EQUAL, ih->attrType), key).end();
    errCode = errCode ? REF_COL_VAL_MISSING : 0;
    delete[] key;
    return errCode;
}

int indexCheckAbsent(int refIndex, IndexHandle *ih, int N, int *offsets,
                     int *lengths, AttrTypeAtom *types, void *buf,
                     const RID &rid)
{
    // if being referencing, ignore this check
    if (refIndex > 0) return 0;
    return indexCheckPresent(refIndex, ih, N, offsets, lengths, types, buf, rid)
               ? 0
               : REF_COL_VAL_CONFLICT;
}

int doForEachIndex(const char *tableName, FileHandle *fht,
                   IndexPreprocessingData *&prep)
{
    prep = new IndexPreprocessingData();
    FileHandle *fh;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0)
        return INDEX_TABLE_ERROR;
    vector<vector<int>> indexnos;
    vector<bool> isKey;
    vector<string> attrNames;
    map<int, int> referencing;
    int count = 0;
    // calculate attr indexes for each index
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        int index = IndexHelper::getIndexNo(mrec.d_ptr);
        int rank = IndexHelper::getRank(mrec.d_ptr);
        attrNames.push_back(IndexHelper::getAttrName(mrec.d_ptr));
        if (indexnos.size() <= index)
            indexnos.resize(index + 1), isKey.resize(index + 1);
        if (indexnos[index].size() <= rank) indexnos[index].resize(rank + 1);
        isKey[index] = IndexHelper::getIsKey(mrec.d_ptr);
        indexnos[index][rank] = count;
        referencing[index] = IndexHelper::getReferencing(mrec.d_ptr);
        count++;
        delete[] mrec.d_ptr;
    }
    map<int, string> refTableName;
    for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
        MRecord mrec = *iter;
        int indMr = IndexHelper::getIndexNo(mrec.d_ptr);
        string nameMr = IndexHelper::getTableName(mrec.d_ptr);
        delete[] mrec.d_ptr;
        if (indMr == 0) continue;
        for (auto p : referencing) {
            int refIndexNo = p.second > 0 ? p.second : -p.second;
            if (indMr == refIndexNo) {
                refTableName[p.first] = nameMr;
                break;
            }
        }
    }
    RecordManager::quickClose(fh);
    if (RecordManager::quickOpen(mainTableFilename, fh) != 0)
        return MAIN_TABLE_ERROR;
    // calculate offsets, types, lengths for each attribute
    vector<int> offsets = vector<int>(count), lengths = vector<int>(count);
    vector<AttrTypeAtom> types = vector<AttrTypeAtom>(count);
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;

        for (int i = 0; i < count; i++) {
            if (attrNames[i] == RecordHelper::getAttrName(mrec.d_ptr)) {
                offsets[i] = RecordHelper::getOffset(mrec.d_ptr);
                lengths[i] = RecordHelper::getLength(mrec.d_ptr);
                types[i] = RecordHelper::getType(mrec.d_ptr);
            }
        }
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    // pick out essential info for index operations
    int errCode = 0;
    auto indman = IndexManager::quickManager();
    for (int i = 0; i < indexnos.size(); i++) {
        auto &indexes = indexnos[i];
        if (indexes.size() == 0) continue;
        int *selOffs = new int[indexes.size()];
        int *selLens = new int[indexes.size()];
        AttrTypeAtom *selTypes = new AttrTypeAtom[indexes.size()];
        for (int j = 0; j < indexes.size(); j++) {
            selOffs[j] = offsets[indexes[j]];
            selLens[j] = lengths[indexes[j]];
            selTypes[j] = types[indexes[j]];
        }
        IndexHandle *ih;
        assert(IndexManager::quickOpen(tableName, i, ih, fht) == 0);
        IndexHandle *refih = nullptr;
        FileHandle *reffh = nullptr;
        if (referencing[i])
            assert(IndexManager::quickOpen(refTableName[i].c_str(),
                                           abs(referencing[i]), refih,
                                           reffh) == 0);
        prep->add(ih, isKey[i], referencing[i], refih, reffh, indexes.size(),
                  selOffs, selLens, selTypes);
    }
    // reserved for future memory free
    prep->setIndexManager(indman);
    return errCode;
}

int IndexPreprocessingData::accept(void *buf, const RID &rid, IdxOps callback,
                                   IdxChk refCheck)
{
    int errCode = 0;
    for (int i = 0; i < ihs.size(); i++) {
        errCode =
            refCheck(refIndexNo[i], refIndexHandle[i], count[i], selOffsets[i],
                     selLens[i], selAttrTypes[i], buf, rid);
        if (errCode == 0)
            errCode = callback(ihs[i], isKey[i], count[i], selOffsets[i],
                               selLens[i], selAttrTypes[i], buf, rid, false);
        if (errCode) {
            // failure recovery
            while (--i >= 0) {
                callback(ihs[i], isKey[i], count[i], selOffsets[i], selLens[i],
                         selAttrTypes[i], buf, rid, true);
            }
            break;
        }
    }
    return errCode;
}

IndexPreprocessingData::~IndexPreprocessingData()
{
    for (auto i : ihs) {
        IndexManager::quickClose(i);
    }
    for (auto i : refIndexHandle) {
        IndexManager::quickClose(i);
    }
    for (auto i : refFileHandle) {
        RecordManager::quickClose(i);
    }
    IndexManager::quickRecycleManager(indman);
    for (auto i : selOffsets)
        delete[] i;
    for (auto i : selLens)
        delete[] i;
    for (auto i : selAttrTypes)
        delete[] i;
}
