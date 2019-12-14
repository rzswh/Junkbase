#include "sysman.h"
#include "../recman/recPacker.h"
#include <cstdio>
#include <string>
#include <algorithm>

using std::string;

const int RecordSize = MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN + 3 * 4 + sizeof(RID) * 3 + 2;
const int AttrRecordSize = MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN + MAX_ATTR_NAME_LEN * MAX_KEY_COMBIN_NUM + sizeof(int)*2;

const char * mainTableFilename = "_MAIN.db";
const char * indexTableFilename = "_INDEX.db";
const char * PRIMARY_INDEX_NAME = "$PRIMARY";

SystemManager::SystemManager(RecordManager * rm, IndexManager * im):rm(rm), im(im) {
}

int SystemManager::createTable(const char * tableName, 
        int attrCount, AttrInfo * attributes) {
    // construct table name c-style string
    string filename_str = string(tableName) + ".db";
    const char * filename = filename_str.c_str();
    // check exist
    FileHandle * fh;
    if (rm->openFile(filename, fh) == 0) {
        rm->closeFile(*fh);
        delete fh;
        return TABLE_EXIST;
    }
    // create table file
    if (rm->openFile(mainTableFilename, fh) != 0) {
        remove(mainTableFilename);
        if (rm->createFile(mainTableFilename, RecordSize) != 0
            || rm->openFile(mainTableFilename, fh) != 0) {
            return MAIN_TABLE_ERROR;
        }
    }
    // add attribute entries to Attr table
    char buf[RecordSize];
    int newTableRecordSize = 0;
    for (int i = 0; i < attrCount; i++) {
        AttrInfo & attr = attributes[i];
        // variant fields
        RID default_rid, attr_rid, ref_table_name_rid, ref_col_name_rid;
        if (attr.defaultValue)
            fh->insertVariant(attr.defaultValue, attr.length, default_rid);
        int indexno = -1; 
        if (attr.isPrimary) {
            indexno = 0;
            vector<int> _arr; _arr.push_back(attr.length);
            im->createIndex(tableName, indexno, attr.type, _arr);
        } else if (attr.isForeign) {
            indexno = i + 1; // TODO: how to allocate?
            fh->insertVariant(attr.refTableName, strlen(attr.refTableName), ref_table_name_rid);
            fh->insertVariant(attr.refColumnName, strlen(attr.refColumnName), ref_col_name_rid);
            vector<const char*> _arr; _arr.push_back(attr.refColumnName);
            createIndex(attr.refTableName, "ForeignKey" + indexno, _arr, indexno);
        }
        RecordPacker packer = RecordPacker(RecordSize);
        packer.addChar(tableName, strlen(tableName), MAX_TABLE_NAME_LEN);
        packer.addChar(attr.attrName, strlen(attr.attrName), MAX_ATTR_NAME_LEN);
        packer.addInt(i + 1); // attr index (what is it for?)
        packer.addInt(attr.length);
        packer.addBinary(&(attr.type), 1);
        packer.addBinary(&(attr.notNull), 1);
        packer.addInt(indexno);
        packer.addRID(default_rid);
        packer.addRID(ref_table_name_rid);
        packer.addRID(ref_col_name_rid);
        packer.unpack(buf, RecordSize);
        fh->insertRecord(buf, attr_rid);
        newTableRecordSize += attr.type == TYPE_VARCHAR ? sizeof(RID) : attr.length;
    }
    rm->closeFile(*fh);
    rm->createFile(filename, newTableRecordSize);
    delete fh;
    return 0;
}

int SystemManager::dropTable(const char* tableName) {
    string filename_str = string(tableName) + ".db";
    const char * filename_cstr = filename_str.c_str();
    FileHandle * fh;
    // remove table file
    if (rm->deleteFile(filename_cstr) != 0) {
        return TABLE_NOT_EXIST;
    }
    // attribute table maintain
    if (rm->openFile(mainTableFilename, fh) != 0) {
        return MAIN_TABLE_ERROR;
    }
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0, 
                Operation(Operation::EQUAL, TYPE_CHAR), tableName);
            !iter.end(); ++iter) {
        MRecord rec = *iter;
        int indexno = *((int*)(rec.d_ptr + RecordSize - sizeof(RID) * 3 - 4));
        RID r1 = *((RID*)(rec.d_ptr + RecordSize - sizeof(RID) * 3));
        RID r2 = *((RID*)(rec.d_ptr + RecordSize - sizeof(RID) * 2));
        RID r3 = *((RID*)(rec.d_ptr + RecordSize - sizeof(RID) * 1));
        dropIndex(tableName, indexno);
        fh->removeVariant(r1);
        fh->removeVariant(r2);
        fh->removeVariant(r3);
        fh->removeRecord(rec.rid);
        delete [] rec.d_ptr;
    }
    // index table maintain
    if (rm->openFile(indexTableFilename, fh) != 0) {
        return INDEX_TABLE_ERROR;
    }
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0, 
                Operation(Operation::EQUAL, TYPE_CHAR), tableName);
            !iter.end(); ++iter) {
        MRecord rec = *iter;
        fh->removeRecord(rec.rid);
        delete [] rec.d_ptr;
    }
    rm->closeFile(*fh);
    delete fh;
    return 0;
}

int SystemManager::createIndex(const char * tableName, const char * indexName, vector<const char *>& attrNames, int indexno) {
    /*int indexNum = allocateIndexNo(tableName);/*getIndexNum(tableName);
    indexno = indexNum;
    setIndexNum(indexNum + 1);*/
    vector<int> attrLens;
    AttrType mergedType = 0;
    // 
    int num = 0;
    for (const char * atn : attrNames) {
        MRecord attr = findAttrRecord(tableName, atn);
        int attrLen = *(int*)(attr.d_ptr + 4 + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN);
        attrLens.push_back(attrLen);
        int attrType_i = *(unsigned char*)(attr.d_ptr + 8 + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN);
        mergedType = mergedType | (attrType_i << (num * 3));
        num++;
        delete [] attr.d_ptr;
    }
    int ret = im->createIndex(tableName, indexno, mergedType, attrLens);
    // establish index on the foreign key of the child table
    FileHandle * fh;
    if (rm->openFile(indexTableFilename, fh) != 0) return INDEX_TABLE_ERROR;
    MRecord mrecs[MAX_KEY_COMBIN_NUM];
    for (int i = 0; i < attrNames.size(); i++) {
        MRecord mrec = findAttrRecord(tableName, attrNames[i]);
        if (mrec.d_ptr == nullptr) return KEY_NOT_EXIST;
        delete [] mrec.d_ptr;
    }
    for (int i = 0; i < attrNames.size(); i++) {
        RecordPacker packer = RecordPacker(AttrRecordSize);
        packer.addChar(tableName, strlen(tableName), MAX_TABLE_NAME_LEN);
        packer.addChar(indexName, strlen(indexName), MAX_KEY_NAME_LEN);
        packer.addInt(indexno);
        packer.addChar(attrNames[i], strlen(attrNames[i]), MAX_ATTR_NAME_LEN);
        packer.addInt(i); // rank
        char buf[AttrRecordSize];
        packer.unpack(buf, AttrRecordSize);
        RID rid;
        fh->insertRecord(buf, rid);
    }
    rm->closeFile(*fh);
    delete fh;
    return ret;
}

int SystemManager::dropIndex(const char * tableName, int indexno) {
    // recycleIndexNo(tableName, indexno);
    int ret = im->deleteIndex(tableName, indexno);
    // edit index table
    FileHandle *fh;
    RID rid;
    if (rm->openFile(indexTableFilename, fh) != 0) return INDEX_TABLE_ERROR;
    MRecord mrec;
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0, 
                Operation(Operation::EQUAL, TYPE_CHAR), (void*)tableName); 
            !iter.end(); ++iter) {
        mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_INT).check(
            &indexno, mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN, 4))
        {
            fh->removeRecord(mrec.rid);
        }
        delete [] mrec.d_ptr;
    }
    return ret;
}

int SystemManager::setIndexNo(const char * tableName, const char * columnName, int indexno) {
    FileHandle * fh;
    if (rm->openFile(mainTableFilename, fh) != 0) return TABLE_NOT_EXIST;
    // modify index no
    MRecord rec = findAttrRecord(tableName, columnName);
    const int IndexPos = RecordSize - sizeof(RID) * 3 - 4;
    *((int*)(rec.d_ptr + IndexPos)) = indexno;
    fh->updateRecord(rec);
    delete [] rec.d_ptr;
    rm->closeFile(*fh);
    delete fh;
    return 0;
}

int SystemManager::addPrimaryKey(const char * tableName, vector<const char *> columnNames) {
    IndexHandle * ih;
    if (im->openIndex(tableName, 0, ih) == 0) {
        im->closeIndex(*ih);
        delete ih;
        return KEY_EXIST;
    }
    int code = createIndex(tableName, PRIMARY_INDEX_NAME, columnNames, 0);
    if (code != 0) return code;
    // setIndexNo(tableName, columnName, 0);
    return 0;
}

int SystemManager::dropPrimaryKey(const char* tableName) {
    FileHandle * fh;
    char * primaryKeyName;
    const int ZERO = 0;
    // find primary key column
    MRecord mrec;
    if (rm->openFile(mainTableFilename, fh) != 0) return MAIN_TABLE_ERROR;
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0, 
            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
        !iter.end(); ++iter) {
        mrec = *iter;
        char * base = mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_ATTR_NAME_LEN;
        if (Operation(Operation::EQUAL, TYPE_INT).check(
                &ZERO, base + 10, 4)) {
            primaryKeyName = mrec.d_ptr + MAX_ATTR_NAME_LEN;
            *(int*)(base + 10) = *(int*)base;
            fh->updateRecord(mrec);
            break;
        }
        delete [] mrec.d_ptr;
    }
    rm->closeFile(*fh);
    delete fh;
    int primaryIndexNo = 0;
    int code = dropIndex(tableName, primaryIndexNo);
    if (code != 0) return code;
    delete [] mrec.d_ptr;
    return 0;
}

MRecord SystemManager::findAttrRecord(const char * tableName, const char * columnName) {
    FileHandle * fh;
    if (rm->openFile(mainTableFilename, fh) != 0) return MRecord();
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0, 
            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
        !iter.end(); ++iter) {
        MRecord mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_CHAR).check(
                columnName, mrec.d_ptr + MAX_TABLE_NAME_LEN, MAX_ATTR_NAME_LEN)) {
            return mrec;
        }
        delete [] mrec.d_ptr;
    }
    rm->closeFile(*fh);
    delete fh;
    return MRecord();
}

int SystemManager::allocateIndexNo() {
    FileHandle * fh;
    if (rm->openFile(indexTableFilename, fh) != 0) return INDEX_TABLE_ERROR;
    vector<int> indexnos;
    for (auto iter = fh->findRecord(4, 0, Operation(Operation::EVERY, TYPE_INT), fh); 
            !iter.end(); ++iter) {
        MRecord mrec = *iter;
        indexnos.push_back(*(int*)(mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN));
        delete [] mrec.d_ptr;
    }
    std::sort(indexnos.begin(), indexnos.end());
    int ret = 1;
    for (int x: indexnos) {
        if (ret == x) ret++;
        else if (ret < x) break;
    }
    return ret;
}

int SystemManager::addForeignKey(const char * tableName, 
        const char * foreignKeyName,
        vector<const char *> columnNames) {
    if (columnNames.size() > MAX_KEY_COMBIN_NUM) return TOO_MANY_KEYS_COMBINED;
    const int IndexPos = RecordSize - sizeof(RID) * 3 - 4;
    int indexno = allocateIndexNo();
    int code = createIndex(tableName, foreignKeyName, columnNames, indexno);
    if (code != 0) return code;
    // setIndexNo(tableName, columnName, indexno);
    return 0;
}

int SystemManager::dropForeignKey(const char * tableName, 
        const char * foreignKeyName) {
    // search in index table
    FileHandle *fh;
    RID rid;
    if (rm->openFile(indexTableFilename, fh) != 0) return INDEX_TABLE_ERROR;
    int indexno = -1;
    MRecord mrec;
    for (auto iter = fh->findRecord(MAX_TABLE_NAME_LEN, 0, 
                Operation(Operation::EQUAL, TYPE_CHAR), (void*)tableName); 
            !iter.end(); ++iter) {
        mrec = *iter;
        if (Operation(Operation::EQUAL, TYPE_CHAR).check(
            foreignKeyName, mrec.d_ptr + MAX_TABLE_NAME_LEN, MAX_KEY_NAME_LEN))
        {
            indexno = *((int*)(mrec.d_ptr + MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN));
            // if (indexno == -1) return KEY_NOT_EXIST;
            break;
        }
        delete [] mrec.d_ptr;
    }
    rm->closeFile(*fh);
    dropIndex(tableName, indexno);
    // const int AttrNamePos = MAX_TABLE_NAME_LEN + MAX_KEY_NAME_LEN + sizeof(int);
    // setIndexNo(tableName, mrec.d_ptr + AttrNamePos, -1);
    delete [] mrec.d_ptr;
    delete fh;
}

int SystemManager::addColumn(const char * tableName, AttrInfo attribute) {
    //
}
int SystemManager::dropColumn(const char * tableName, char * columnName) {
    //
}
int SystemManager::changeColumn(const char * tableName, 
        const char * oldColumnName, 
        AttrInfo attribute){
    //
}