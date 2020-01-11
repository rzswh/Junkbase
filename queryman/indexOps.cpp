
#include "indexOps.h"
#include "../errors.h"
#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"
#include "../sysman/sysman.h"
#include "../utils/helper.h"
#include <vector>

int indexTryInsert(IndexHandle *ih, bool isKey, int N, int *offsets,
                   int *lengths, AttrTypeAtom *types, void *buf, const RID &rid,
                   bool recover)
{
    int errCode = 0;
    char *key = IndexManager::makeKey(N, buf, offsets, lengths, types, errCode);
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

int doForEachIndex(const char *tableName, IdxOps callback, void *buf,
                   const RID &rid, FileHandle *fht)
{
    FileHandle *fh;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0)
        return INDEX_TABLE_ERROR;
    vector<vector<int>> indexnos;
    vector<bool> isKey;
    vector<string> attrNames;
    int count = 0;
    // calculate attr indexes for each index
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        int index = IndexHelper::getIndexNo(mrec.d_ptr);
        int rank = IndexHelper::getRank(mrec.d_ptr);
        bool key = IndexHelper::getIsKey(mrec.d_ptr);
        attrNames.push_back(IndexHelper::getAttrName(mrec.d_ptr));
        delete[] mrec.d_ptr;
        if (indexnos.size() <= index)
            indexnos.resize(index + 1), isKey.resize(index + 1);
        if (indexnos[index].size() <= rank) indexnos[index].resize(rank + 1);
        isKey[index] = key;
        indexnos[index][rank] = count;
        count++;
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
        assert(indman->openIndex(tableName, i, ih, fht) == 0);
        errCode = callback(ih, isKey[i], count, selOffs, selLens, selTypes, buf,
                           rid, false);
        IndexManager::closeIndex(*ih);
        delete ih;
        if (errCode) {
            // failure recovery
            while (--i >= 0) {
                assert(indman->openIndex(tableName, i, ih, fht) == 0);
                callback(ih, isKey[i], count, selOffs, selLens, selTypes, buf,
                         rid, true);
                IndexManager::quickClose(ih);
            }
        }
        delete[] selOffs;
        delete[] selLens;
        delete[] selTypes;
        if (errCode) break;
    }
    IndexManager::quickRecycleManager(indman);
    return errCode;
}