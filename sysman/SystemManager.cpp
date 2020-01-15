#include "../queryman/queryman.h"
#include "../recman/recPacker.h"
#include "../utils/helper.h"
#include "sysman.h"
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
using std::set;
using std::string;

void showDatabases()
{
    DIR *pDir = opendir(".");
    if (pDir) {
        while (1) {
            dirent *pFile = readdir(pDir);
            if (pFile == NULL) break;
            if (pFile->d_type == DT_DIR && pFile->d_name[0] != '.') {
                printf("%s\n", pFile->d_name);
            }
        }
    }
}

int fillIndexByEntry(const char *tableName, int indexno,
                     vector<const char *> attrNames, int refIndexNo,
                     string refTableName)
{
    FileHandle *fh;
    if (RecordManager::quickOpenTable(tableName, fh) != 0)
        return TABLE_NOT_EXIST;
    int N = attrNames.size();
    int *offsets = new int[N];
    int *lengths = new int[N];
    AttrTypeAtom *types = new AttrTypeAtom[N];
    int errCode = QueryManager::getAttrStorage(tableName, attrNames, offsets,
                                               lengths, types);
    if (errCode) {
        delete[] offsets;
        delete[] lengths;
        delete[] types;
        RecordManager::quickClose(fh);
        return errCode;
    }
    IndexHandle *ih;
    FileHandle *irfh = nullptr;
    if (IndexManager::quickOpen(tableName, indexno, ih, irfh) != 0) {
        delete[] offsets;
        delete[] lengths;
        delete[] types;
        RecordManager::quickClose(fh);
        return INDEX_NOT_EXIST;
    }
    IndexHandle *refIh = nullptr;
    FileHandle *refFh = nullptr;
    if (refIndexNo > 0) {
        if (IndexManager::quickOpen(refTableName.c_str(), refIndexNo, refIh,
                                    refFh) != 0) {
            delete[] offsets;
            delete[] lengths;
            delete[] types;
            RecordManager::quickClose(fh);
            IndexManager::quickClose(ih);
            RecordManager::quickClose(irfh);
            return errCode;
        }
    }
    bool conflict = false;
    do {
        for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
            MRecord mrec = *iter;
            char *key = IndexManager::makeKey(N, mrec.d_ptr, offsets, lengths,
                                              types, mrec.rid, errCode);
            if (!errCode) {
                errCode = refIh && refIh
                                       ->findEntry(Operation(Operation::EQUAL,
                                                             refIh->attrType),
                                                   key)
                                       .end()
                              ? REF_COL_VAL_MISSING
                              : 0;
                if (errCode) {
                    printf("%d\n", *((int *)key));
                    auto iter = refIh->findEntry(
                        Operation(Operation::EQUAL, refIh->attrType), key);
                    auto rid = *iter;
                    iter.end();
                }
                if (!errCode) {
                    int result = ih->insertEntry(key, mrec.rid);
                    if (result) conflict = true;
                }
            }
            if (conflict) ih->deleteEntry(key, mrec.rid);
            delete[] key;
            delete[] mrec.d_ptr;
            if (conflict) break;
        }
        if (conflict) errCode = DUPLICATED_KEY;
    } while (false);
#ifdef DEBUG
    if (!errCode) {
        for (auto iter =
                 ih->findEntry(Operation(Operation::EVERY, TYPE_CHAR), nullptr);
             !iter.end(); ++iter) {
            RID rid = *iter;
            printf("%d %d\n", rid.getPageNum(), rid.getSlotNum());
        }
    }
#endif
    delete[] offsets;
    delete[] lengths;
    delete[] types;
    RecordManager::quickClose(fh);
    IndexManager::quickClose(ih);
    RecordManager::quickClose(irfh);
    if (refIh) IndexManager::quickClose(refIh);
    if (refFh) RecordManager::quickClose(refFh);
    return errCode;
}

int SystemManager::useDatabase(const char *name)
{
    if (usingDatabase) {
        chdir("../");
        usingDatabase = false;
    }
    if (chdir(name) == -1) {
        printf("ERROR: Database \"%s\" does not exist.\n", name);
    } else {
        usingDatabase = true;
        currentDatabase = name;
    }
    return 0;
}
int SystemManager::dropDatabase(const char *name)
{
    if (currentDatabase == name) {
        chdir("../");
        usingDatabase = false;
    }
    string cmd;
    if (usingDatabase)
        cmd = string("rm -r ") + "../" + string(name);
    else
        cmd = string("rm -r ") + string(name);
    system(cmd.c_str());
    return 0;
}

const int RecordSize =
    MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN + 3 * 4 + sizeof(RID) * 3 + 2;
const int IndexRecordSize = MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN +
                            MAX_ATTR_NAME_LEN + sizeof(int) * 3 + 1 +
                            sizeof(RID);

SystemManager::SystemManager() { usingDatabase = false; }
SystemManager::~SystemManager() {}

int SystemManager::createTable(const char *tableName,
                               vector<AttrInfo> attributes)
{
    // construct table name c-style string
    string filename_str = string(tableName) + ".db";
    const char *filename = filename_str.c_str();
    // check exist
    FileHandle *fh;
    if (RecordManager::quickOpen(filename, fh) == 0) {
        RecordManager::quickClose(fh);
        return TABLE_EXIST;
    }
    // check foreign key valid
    for (int i = 0; i < attributes.size(); i++) {
        AttrInfo &attr = attributes[i];
        if (attr.isForeign) {
            for (auto s : attr.refColumnName) {
                MRecord mrec = findAttrRecord(attr.refTableName, s);
                if (mrec.rid == RID(0, 0)) {
                    delete[] mrec.d_ptr;
                    return REF_NOT_EXIST;
                }
                delete[] mrec.d_ptr;
            }
        }
    }
    // create table file
    if (RecordManager::quickOpen(mainTableFilename, fh, RecordSize) != 0)
        return MAIN_TABLE_ERROR;
    // add attribute entries to Attr table
    char buf[RecordSize];
    int newTableRecordSize = 0;
    int code = 0;
    for (int i = 0; i < attributes.size(); i++) {
        AttrInfo &attr = attributes[i];
        // variant fields
        RID default_rid, attr_rid;
        RID ref_table_name_rid, ref_col_name_rid;
        if (attr.isForeign) {
            continue;
            // separate tag!
            // fh->insertVariant(attr.refTableName, strlen(attr.refTableName),
            //                   ref_table_name_rid);
            // fh->insertVariant(attr.refColumnName, strlen(attr.refColumnName),
            //                   ref_col_name_rid);
        }
        if (attr.isForeign || attr.isPrimary) continue;
        if (attr.defaultValue)
            fh->insertVariant(attr.defaultValue, attr.length, default_rid);
        RecordPacker packer = RecordPacker(RecordSize);
        packer.addChar(tableName, strlen(tableName), MAX_TABLE_NAME_LEN);
        packer.addChar(attr.attrName, strlen(attr.attrName), MAX_ATTR_NAME_LEN);
        packer.addInt(newTableRecordSize); // offset
        packer.addInt(attr.length);
        packer.addBinary(&(attr.type), 1);
        packer.addBinary(&(attr.notNull), 1);
        packer.addInt(i); // index
        packer.addRID(default_rid);
        packer.addRID(ref_table_name_rid);
        packer.addRID(ref_col_name_rid);
        packer.unpack(buf, RecordSize);
        fh->insertRecord(buf, attr_rid);
        newTableRecordSize +=
            attr.type == TYPE_VARCHAR ? sizeof(RID) : attr.length;
    }
    RecordManager::quickClose(fh);
    RecordManager *rm = RecordManager::quickManager();
    rm->createFile(filename, newTableRecordSize);
    RecordManager::quickRecycleManager(rm);
    // printf("Creating index...\n");
    // create index
    for (int i = 0; i < attributes.size(); i++) {
        AttrInfo &attr = attributes[i];
        int indexno = -1;
        if (attr.isPrimary) {
            addPrimaryKey(tableName, attr.keyCols);
            continue;
        } else if (attr.isForeign) {
            string keyNameString =
                string("ForeignKey") + to_string(indexno); // --std=c++11
            addForeignKey(keyNameString.c_str(), tableName, attr.keyCols,
                          attr.refTableName, attr.refColumnName);
            // indexno = allocateIndexNo();
            // createIndex(attr.refTableName, keyNameString.c_str(),
            //             attr.refColumnName, indexno, true);
            // // TODO: referencing columns?
            continue;
        }
    }
    // printf("Creation complete.\n");
    return code;
}

int SystemManager::dropTable(const char *tableName)
{
    string filename_str = string(tableName) + ".db";
    const char *filename_cstr = filename_str.c_str();
    RecordManager *rm = RecordManager::quickManager();
    FileHandle *fh;
    // remove table file
    if (rm->deleteFile(filename_cstr) != 0) {
        return TABLE_NOT_EXIST;
    }
    // attribute table maintain
    if (rm->openFile(mainTableFilename, fh) != 0) {
        return MAIN_TABLE_ERROR;
    }
    // TODO: referenced table? deny!
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord rec = *iter;
        // int indexno = *((int *)(rec.d_ptr + RecordSize - sizeof(RID) * 3 -
        // 4));
        RID r1 = *((RID *)(rec.d_ptr + RecordSize - sizeof(RID) * 3));
        RID r2 = *((RID *)(rec.d_ptr + RecordSize - sizeof(RID) * 2));
        RID r3 = *((RID *)(rec.d_ptr + RecordSize - sizeof(RID) * 1));
        // dropIndex(tableName, indexno);
        fh->removeVariant(r1);
        fh->removeVariant(r2);
        fh->removeVariant(r3);
        fh->removeRecord(rec.rid);
        delete[] rec.d_ptr;
    }
    rm->closeFile(*fh);
    // index table maintain
    if (rm->openFile(indexTableFilename, fh) != 0) {
        return INDEX_TABLE_ERROR;
    }
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord rec = *iter;
        fh->removeRecord(rec.rid);
        delete[] rec.d_ptr;
    }
    rm->closeFile(*fh);
    delete fh;
    RecordManager::quickRecycleManager(rm);
    return 0;
}

int SystemManager::createIndex(const char *tableName, const char *indexName,
                               const vector<const char *> &attrNames)
{
    int indexno;

    allocateIndexNo(1, &indexno);
    createIndex(tableName, indexName,
                const_cast<vector<const char *> &>(attrNames), indexno, false,
                0);
}

int SystemManager::createIndex(const char *tableName, const char *indexName,
                               vector<const char *> &attrNames, int indexno,
                               bool isKey, int referencing)
{
    /*int indexNum = allocateIndexNo(tableName);/*getIndexNum(tableName);
    indexno = indexNum;
    setIndexNum(indexNum + 1);*/
    vector<int> attrLens;
    AttrType mergedType = 0;
    //
    int num = 0;
    for (const char *atn : attrNames) {
        MRecord attr = findAttrRecord(tableName, atn);
        if (attr.d_ptr == nullptr) return ATTR_NOT_EXIST;
        int attrLen = RecordHelper::getLength(attr.d_ptr);
        attrLens.push_back(attrLen);
        int attrType_i = RecordHelper::getType(attr.d_ptr);
        mergedType = mergedType | (attrType_i << (num * 3));
        num++;
        delete[] attr.d_ptr;
    }
    if (!isKey) {
        // duplicated key supported
        attrLens.push_back(sizeof(RID));
        mergedType = mergedType | (TYPE_RID << (num * 3));
    }
    IndexManager *im = IndexManager::quickManager();
    IndexHandle *ih;
    FileHandle *fh = nullptr;
    if (im->openIndex(tableName, indexno, ih, fh) == 0) {
        im->closeIndex(*ih);
        delete im;
        if (fh) RecordManager::closeFile(*fh), delete fh;
        IndexManager::quickRecycleManager(im);
        return INDEX_EXIST;
    }
    int ret = im->createIndex(tableName, indexno, mergedType, attrLens);
    IndexManager::quickRecycleManager(im);
    // printf("Ready to fill...\n");
    if (ret != 0) return ret;
    // establish index on the foreign key of the child table
    if (RecordManager::quickOpen(indexTableFilename, fh, IndexRecordSize) != 0)
        return INDEX_TABLE_ERROR;
    MRecord mrecs[MAX_KEY_COMBIN_NUM];
    for (int i = 0; i < attrNames.size(); i++) {
        MRecord mrec = findAttrRecord(tableName, attrNames[i]);
        if (mrec.d_ptr == nullptr) return KEY_NOT_EXIST;
        delete[] mrec.d_ptr;
    }
    string refTableName = "";
    // look up for referencing table name
    if (referencing > 0) {
        auto iter =
            fh->findRecord(sizeof(int), MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN,
                           Operation(Operation::EQUAL, TYPE_INT), &referencing);
        assert(!iter.end());
        MRecord mrec = *iter;
        refTableName = RecordHelper::getTableName(mrec.d_ptr);
        delete[] mrec.d_ptr;
    }
    // fill in index table
    RID varRID;
    fh->insertVariant("", 1, varRID);
    for (int i = 0; i < attrNames.size(); i++) {
        RecordPacker packer = RecordPacker(IndexRecordSize);
        packer.addChar(tableName, strlen(tableName), MAX_TABLE_NAME_LEN);
        packer.addChar(indexName, strlen(indexName), MAX_KEY_NAME_LEN);
        packer.addInt(indexno);
        packer.addChar(attrNames[i], strlen(attrNames[i]), MAX_ATTR_NAME_LEN);
        packer.addInt(i); // rank
        packer.addChar((char *)&isKey, 1);
        packer.addInt(referencing);
        packer.addRID(varRID);
        char buf[IndexRecordSize];
        packer.unpack(buf, IndexRecordSize);
        RID rid;
        fh->insertRecord(buf, rid);
    }
    RecordManager::quickClose(fh);
    // fill index by entries
    ret = fillIndexByEntry(tableName, indexno, attrNames, referencing,
                           refTableName);
    if (ret) {
        // undo
        dropIndex(tableName, indexno);
    }
    return ret;
}

int SystemManager::dropIndex(const char *tableName, const char *indexName)
{
    FileHandle *fh;
    RID rid;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0)
        return INDEX_TABLE_ERROR;
    int indexno = -1;
    MRecord mrec;
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                                    Operation(Operation::EQUAL, TYPE_CHAR),
                                    (void *)tableName);
         !iter.end(); ++iter) {
        mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_CHAR)
                .check(indexName, mrec.d_ptr + MAX_TABLE_NAME_LEN,
                       MAX_KEY_NAME_LEN)) {
            indexno =
                *((int *)(mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN));
        }
        delete[] mrec.d_ptr;
        if (indexno >= 0) break;
    }
    if (indexno > -1) dropIndex(tableName, indexno);
    RecordManager::quickClose(fh);
    return indexno > -1 ? 0 : INDEX_NOT_EXIST;
}

int SystemManager::dropIndex(const char *tableName, int indexno)
{
    // recycleIndexNo(tableName, indexno);
    IndexManager *im = IndexManager::quickManager();
    int ret = im->deleteIndex(tableName, indexno);
    IndexManager::quickRecycleManager(im);
    // edit index table
    FileHandle *fh;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0)
        return INDEX_TABLE_ERROR;
    RID varRID;
    MRecord mrec;
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                                    Operation(Operation::EQUAL, TYPE_CHAR),
                                    (void *)tableName);
         !iter.end(); ++iter) {
        mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_INT)
                .check(&indexno,
                       mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN, 4)) {
            if (varRID == RID(0, 0)) {
                varRID = IndexHelper::getReferenced(mrec.d_ptr);
                fh->removeVariant(varRID);
            }
            fh->removeRecord(mrec.rid);
        }
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    return ret;
}

// int SystemManager::setIndexNo(const char *tableName, const char *columnName,
//                               int indexno)
// {
//     FileHandle *fh;
//     if (RecordManager::quickOpen(mainTableFilename, fh) != 0)
//         return TABLE_NOT_EXIST;
//     // modify index no
//     MRecord rec = findAttrRecord(tableName, columnName);
//     const int IndexPos = RecordSize - sizeof(RID) * 3 - 4;
//     *((int *)(rec.d_ptr + IndexPos)) = indexno;
//     fh->updateRecord(rec);
//     delete[] rec.d_ptr;
//     RecordManager::quickClose(fh);
//     return 0;
// }

int SystemManager::addPrimaryKey(const char *tableName,
                                 vector<const char *> columnNames)
{
    IndexHandle *ih;
    FileHandle *fh;
    if (RecordManager::quickOpenTable(tableName, fh) != 0) {
        return TABLE_NOT_EXIST;
    }
    RecordManager::quickClose(fh);
    if (IndexManager::quickOpen(tableName, 0, ih, fh) == 0) {
        IndexManager::quickClose(ih);
        return KEY_EXIST;
    }
    int code = createIndex(tableName, PRIMARY_INDEX_NAME, columnNames, 0, true);
    if (code != 0) return code;
    // setIndexNo(tableName, columnName, 0);
    return 0;
}

int SystemManager::dropPrimaryKey(const char *tableName)
{
    // FileHandle * fh;
    // char * primaryKeyName;
    // const int ZERO = 0;
    // // find primary key column
    // MRecord mrec;
    // if (rm->openFile(mainTableFilename, fh) != 0) return MAIN_TABLE_ERROR;
    // for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0,
    //         Operation(Operation::EQUAL, TYPE_CHAR), tableName);
    //     !iter.end(); ++iter) {
    //     mrec = *iter;
    //     char * base = mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN;
    //     if (Operation(Operation::EQUAL, TYPE_INT).check(
    //             &ZERO, base + 10, 4)) {
    //         primaryKeyName = mrec.d_ptr + MAX_ATTR_NAME_LEN;
    //         *(int*)(base + 10) = *(int*)base;
    //         fh->updateRecord(mrec);
    //         break;
    //     }
    //     delete [] mrec.d_ptr;
    // }
    // rm->closeFile(*fh);
    // delete fh;
    int primaryIndexNo = 0;
    int code = dropIndex(tableName, primaryIndexNo);
    if (code != 0) return code;
    // delete [] mrec.d_ptr;
    return 0;
}

MRecord SystemManager::findAttrRecord(const char *tableName,
                                      const char *columnName)
{
    FileHandle *fh;
    if (RecordManager::quickOpen(mainTableFilename, fh) != 0) return MRecord();
    MRecord ret;
    // if (rm->openFile(mainTableFilename, fh) != 0) return MRecord();
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_CHAR)
                .check(columnName, mrec.d_ptr + MAX_TABLE_NAME_LEN,
                       MAX_ATTR_NAME_LEN)) {
            ret = mrec;
            break;
        }
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    return ret;
}

int SystemManager::allocateIndexNo(int num, int *dst)
{
    FileHandle *fh;
    if (RecordManager::quickOpen(indexTableFilename, fh, IndexRecordSize) != 0)
        return 1;
    // if (rm->openFile(indexTableFilename, fh) != 0) return 1;
    vector<int> indexnos;
    for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
        MRecord mrec = *iter;
        indexnos.push_back(IndexHelper::getIndexNo(mrec.d_ptr));
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    std::sort(indexnos.begin(), indexnos.end());
    int ret = 1;
    for (int i = 0; i < num; i++) {
        for (int x : indexnos) {
            if (ret == x)
                ret++;
            else if (ret < x) {
                break;
            }
        }
        dst[i] = ret++;
    }
    return 0;
}

int SystemManager::addReferenced(const char *tableName, int dst, int src)
{
    FileHandle *fh;
    RID rid_ret;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0)
        return INDEX_TABLE_ERROR;
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        if (dst != IndexHelper::getIndexNo(mrec.d_ptr)) continue;
        if (rid_ret == RID(0, 0)) {
            RID varRID = IndexHelper::getReferenced(mrec.d_ptr);
            const int MAX_REFERENCED = 10;
            char buf[3 * MAX_REFERENCED];
            fh->getVariant(varRID, buf, 3 * MAX_REFERENCED);
            fh->removeVariant(varRID);
            int len = 0;
            while (buf[len])
                len++;
            sprintf(buf + len, "%d;", src);
            while (buf[len++])
                ;
            fh->insertVariant(buf, len + 1, rid_ret);
        }
        IndexHelper::getReferenced(mrec.d_ptr) = rid_ret;
        fh->updateRecord(mrec);
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    return 0;
}

int SystemManager::addForeignKey(const char *foreignKeyName,
                                 const char *tableName,
                                 vector<const char *> columnNames,
                                 const char *refTableName,
                                 vector<const char *> refColumnNames)
{
    if (columnNames.size() > MAX_KEY_COMBIN_NUM) return TOO_MANY_KEYS_COMBINED;
    // check if attr Types are consistent
    int indexno[2];
    allocateIndexNo(2, indexno);
    string masterName = string(foreignKeyName) + "-m";
    string slaveName = string(foreignKeyName) + "-s";
    int code = createIndex(refTableName, masterName.c_str(), refColumnNames,
                           indexno[0], true, -indexno[1]);
    // addReferenced(refTableName, indexno[0], indexno[1]);
    if (code != 0 && code != INDEX_EXIST) return code;
    code = createIndex(tableName, slaveName.c_str(), columnNames, indexno[1],
                       false, indexno[0]);
    // setIndexNo(tableName, columnName, indexno);
    return code;
}

int SystemManager::dropForeignKey(const char *tableName,
                                  const char *foreignKeyName)
{
    // search in index table
    FileHandle *fh;
    RID rid;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0)
        return INDEX_TABLE_ERROR;
    int indexno[2] = {-1, -1};
    string keyName1 = string(foreignKeyName) + "-m";
    string keyName2 = string(foreignKeyName) + "-s";
    MRecord mrec;
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                                    Operation(Operation::EQUAL, TYPE_CHAR),
                                    (void *)tableName);
         !iter.end(); ++iter) {
        mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_CHAR)
                .check(keyName1.c_str(), mrec.d_ptr + MAX_TABLE_NAME_LEN,
                       MAX_KEY_NAME_LEN)) {
            indexno[0] =
                *((int *)(mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN));
        }
        if (Operation(Operation::EQUAL, TYPE_CHAR)
                .check(keyName2.c_str(), mrec.d_ptr + MAX_TABLE_NAME_LEN,
                       MAX_KEY_NAME_LEN)) {
            indexno[1] =
                *((int *)(mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN));
        }
        delete[] mrec.d_ptr;
        if (indexno[0] >= 0 || indexno[1] >= 0) break;
    }
    RecordManager::quickClose(fh);
    dropIndex(tableName, indexno[0]);
    dropIndex(tableName, indexno[1]);
    // const int AttrNamePos = MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN +
    // sizeof(int); setIndexNo(tableName, mrec.d_ptr + AttrNamePos, -1);
    return 0;
}

int SystemManager::addColumn(const char *tableName, AttrInfo attribute)
{
    //
}
int SystemManager::dropColumn(const char *tableName, char *columnName)
{
    //
}
int SystemManager::changeColumn(const char *tableName,
                                const char *oldColumnName, AttrInfo attribute)
{
    //
}

int SystemManager::displayIndexesInfo()
{
    FileHandle *fh;
    if (RecordManager::quickOpen(indexTableFilename, fh) != 0) {
        return INDEX_TABLE_ERROR;
    }
    for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
        MRecord mrec = *iter;
        string tableName = string(mrec.d_ptr);
        printf("%s\t", tableName.c_str());
        printf("%s\t", IndexHelper::getIndexName(mrec.d_ptr).c_str());
        printf("%d\t", IndexHelper::getIndexNo(mrec.d_ptr));
        printf("%s\t", IndexHelper::getAttrName(mrec.d_ptr).c_str());
        printf("%d\t", IndexHelper::getRank(mrec.d_ptr));
        printf("%s\t", IndexHelper::getIsKey(mrec.d_ptr) ? "Key" : "Not key");
        printf("%d\t", IndexHelper::getReferencing(mrec.d_ptr));
        RID rid = IndexHelper::getReferenced(mrec.d_ptr);
        const int MAX_REFERENCED = 10;
        char buf[3 * MAX_REFERENCED];
        fh->getVariant(rid, buf, 3 * MAX_REFERENCED);
        printf("%s\n", buf);
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    return 0;
}

int SystemManager::descTable(const char *tableName)
{
    FileHandle *fh;
    if (RecordManager::quickOpen(mainTableFilename, fh) != 0)
        return MAIN_TABLE_ERROR;
    printf("** Table: %s **\n", tableName);
    printf("KeyName\tType\tNullable?\n");
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        printf("%s\t", RecordHelper::getAttrName(mrec.d_ptr).c_str());
        AttrTypeAtom type = RecordHelper::getType(mrec.d_ptr);
        int len = RecordHelper::getLength(mrec.d_ptr);
        switch (type) {
        case TYPE_CHAR:
            printf("char(%d)\t", len);
            break;
        case TYPE_INT:
            printf("int\t");
            break;
        case TYPE_VARCHAR:
            printf("varchar(%d)\t", len);
            break;
        case TYPE_NUMERIC:
            printf("numeric\t");
            break;
        case TYPE_DATE:
            printf("date\t");

        default:
            break;
        }
        if (RecordHelper::getNotNull(mrec.d_ptr))
            printf("Not Null\t");
        else
            printf("\t\t");
        printf("\n");
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    vector<string> indexNames;
    assert(RecordManager::quickOpen(indexTableFilename, fh) == 0);
    printf("Indexes and Keys:\n");
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        indexNames.push_back(IndexHelper::getIndexName(mrec.d_ptr));
        delete[] mrec.d_ptr;
    }
    for (auto name : indexNames) {
        bool isKey;
        int ref;
        for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                                        Operation(Operation::EQUAL, TYPE_CHAR),
                                        tableName);
             !iter.end(); ++iter) {
            MRecord mrec = *iter;
            if (name == IndexHelper::getIndexName(mrec.d_ptr)) {
                isKey = IndexHelper::getIsKey(mrec.d_ptr);
                ref = IndexHelper::getReferencing(mrec.d_ptr);
            }
            delete[] mrec.d_ptr;
        }
        printf("\"%s\"\t", name.c_str());
        if (isKey) printf("%s\t", ref == 0 ? "PRIMARY KEY" : "FOREIGN KEY");
        puts("");
    }
}

int SystemManager::showTables()
{
    std::set<string> names;
    FileHandle *fh;
    if (RecordManager::quickOpen(mainTableFilename, fh, RecordSize) != 0) {
        return MAIN_TABLE_ERROR;
    }
    for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
        MRecord mrec = *iter;
        names.insert(RecordHelper::getTableName(mrec.d_ptr));
        delete[] mrec.d_ptr;
    }
    printf("--Tables--\n");
    for (auto s : names) {
        printf("%s\n", s.c_str());
    }
    RecordManager::quickClose(fh);
    return 0;
}