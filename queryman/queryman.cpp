#include "queryman.h"
#include "../def.h"
#include "../errors.h"
#include "../recman/MRecord.h"
#include "../recman/RecordManager.h"
#include "../sysman/sysman.h"
#include "../utils/helper.h"
#include <map>
#include <set>
using std::map;
using std::set;

QueryManager::QueryManager() {}
QueryManager::~QueryManager() {}

bool testCompOpOnTables(int numWhere, int tableIndex, MRecord *mrecs,
                        int *wcTableIndex, int *wcOffsets, int *wcLengths,
                        ValueHolder *wcOperands, vector<CompOp *> compops,
                        FileHandle *fh)
{
    for (int j = 0; j < numWhere; j++) {
        int m1 = wcTableIndex[j << 1], m2 = wcTableIndex[j << 1 | 1];
        // attr op val / attr op
        if (m1 == tableIndex && m2 == -1 &&
            !compops[j]->check(mrecs[m1].d_ptr + wcOffsets[j << 1],
                               wcOperands[j].buf, wcLengths[j << 1], fh)) {
            return false;
        }
        if (m2 == -1) continue;
        // attr op attr
        if (m1 == tableIndex && m2 <= tableIndex ||
            m1 <= tableIndex && m2 == tableIndex) {
            if (compops[j]->getType() == TYPE_VARCHAR) {
                // TODO: get variant data
            }
            if (!compops[j]->check(mrecs[m1].d_ptr + wcOffsets[j << 1],
                                   mrecs[m2].d_ptr + wcOffsets[j << 1 | 1],
                                   wcLengths[j << 1], fh)) {
                return false;
            }
        }
    }
    return true;
}

int getOffsetAndType(const char *relName, const char *attrName, int &offset,
                     int &length, AttrTypeAtom &type)
{
    FileHandle *fh;
    if (RecordManager::quickOpen(mainTableFilename, fh) != 0)
        return MAIN_TABLE_ERROR;
    int errCode = 0;
    auto comparator = ComposedOperation(
        2,
        {Operation(Operation::EQUAL, TYPE_CHAR),
         Operation(Operation::EQUAL, TYPE_CHAR)},
        {0, MAX_TABLE_NAME_LEN}, {MAX_TABLE_NAME_LEN, MAX_ATTR_NAME_LEN},
        {(void *)relName, (void *)attrName});
    auto iter = fh->findRecord(comparator);
    // - check table and attr existence
    if (iter.end()) {
        errCode = ATTR_NOT_EXIST;
    }
    // - get offsets and types
    else {
        MRecord mrec = *iter;
        offset = RecordHelper::getOffset(mrec.d_ptr);
        length = RecordHelper::getLength(mrec.d_ptr);
        type = RecordHelper::getType(mrec.d_ptr);
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    return errCode;
}

/**
 * @param wcOffsets length=numWhere*2
 * @param wcLengths length=numWhere*2
 * @param wcOperands length=numWhere
 */
void selectOnCartesianProduct(const vector<string> &tables, int numSel,
                              int *selTableIndex, int *selOffsets,
                              int *selLengths, AttrTypeAtom *selTypes,
                              int numWhere, int *wcTableIndex, int *wcOffsets,
                              int *wcLengths, ValueHolder *wcOperands,
                              vector<CompOp *> compops, PrintableTable *printer)
{
    int numTables = tables.size();
    FileHandle **fhs = new FileHandle *[numTables];
    for (int i = 0; i < numTables; i++)
        assert(0 ==
               RecordManager::quickOpen((tables[i] + ".db").c_str(), fhs[i]));
    FileHandle::iterator **iters = new FileHandle::iterator *[numTables];
    memset(iters, 0, sizeof(FileHandle::iterator *) * numTables);
    int tableIndex = 0;
    MRecord *mrecs = new MRecord[numTables];
    while (true) {
        // find a record!
        if (tableIndex == numTables) {
            for (int k = 0; k < numSel; k++) {
                //
                int tbInd = selTableIndex[k];
                ValueHolder val = ValueHolder(
                    selTypes[k], mrecs[tbInd].d_ptr + selOffsets[k],
                    AttrTypeHelper::getRawLength(selLengths[k], selTypes[k]));
                if (selTypes[k] == TYPE_VARCHAR && !val.isNull()) {
                    ValueHolder nv =
                        ValueHolder(TYPE_CHAR, nullptr, selLengths[k]);
                    fhs[tbInd]->getVariant(*(RID *)val.buf, nv.buf,
                                           selLengths[k]);
                    val = nv;
                }
                printer->setData(k, val);
            }
            printer->addRow();
        }
        // iteration finish on a table
        if (tableIndex == numTables ||
            iters[tableIndex] && iters[tableIndex]->end()) {
            if (tableIndex < numTables) {
                delete[](char *)(iters[tableIndex]);
                iters[tableIndex] = nullptr;
            }
            tableIndex--;
            if (tableIndex < 0) break;
            ++(*iters[tableIndex]);
            continue;
        }
        // initialize a search iterator
        if (iters[tableIndex] == nullptr) {
            const int SI = sizeof(FileHandle::iterator);
            iters[tableIndex] = (FileHandle::iterator *)(new char[SI]);
            auto &&val = fhs[tableIndex]->findRecord(
                0, 0, Operation(Operation::EVERY, TYPE_CHAR), "");
            memcpy(iters[tableIndex], &val, SI);
            continue;
        }
        // iterate over it to find next satisfying record
        bool ok = true;
        // tableIndex, d_ptr, fh
        MRecord &mrec = mrecs[tableIndex] = **iters[tableIndex];
        ok = testCompOpOnTables(numWhere, tableIndex, mrecs, wcTableIndex,
                                wcOffsets, wcLengths, wcOperands, compops,
                                fhs[tableIndex]);
        if (!ok) {
            ++(*iters[tableIndex]);
            delete[] mrec.d_ptr;
            continue;
        } else {
            assert(tableIndex < numTables);
            tableIndex++;
            continue;
        }
    }
    for (int i = 0; i < numTables; i++)
        RecordManager::quickClose(fhs[i]);
    delete[] fhs, iters;
}

int parseWhereClause(const Condition &condition, const vector<string> tables,
                     int *&wcTableIndex, int *&wcLengths, int *&wcOffsets,
                     AttrTypeAtom *&wcTypes, ValueHolder *&wcOperands,
                     vector<CompOp *> &operations)
{
    int errCode = 0;
    // - match condition attr with real attrs
    int NW = condition.judSet.size();
    wcTableIndex = new int[NW * 2];
    wcOperands = new ValueHolder[NW];
    int index = 0;
    for (auto i : condition.judSet) {
        if (tables.size() > 1 && i.lhs.attrRel == "") {
            errCode = AMBIGUOUS_TABLE;
            break;
        }
        // match lhs attr with table
        int ltbi = 0;
        for (; ltbi < tables.size(); ltbi++)
            if (i.lhs.attrRel == "" || tables[ltbi] == i.lhs.attrRel) break;
        if (ltbi == tables.size()) {
            errCode = MISSING_TABLE_IN_FROM_CLAUSE;
            break;
        }
        wcTableIndex[index << 1] = ltbi;
        // 1. attr op attr; 2. attr op val; 3. attr op
        if (i.rhs.attrName != "") {
            // match rhs attr with table
            int rtbi = 0;
            for (; rtbi < tables.size(); rtbi++)
                if (tables[rtbi] == i.rhs.attrRel) break;
            if (i.lhs.attrRel == "" || rtbi == tables.size()) {
                errCode = MISSING_TABLE_IN_FROM_CLAUSE;
                break;
            }
            wcTableIndex[index << 1 | 1] = rtbi;
        } else {
            wcTableIndex[index << 1 | 1] = -1;
            wcOperands[index] = i.valRhs;
        }
        index++;
    }
    if (errCode) {
        // recycle
        delete[] wcTableIndex, wcOperands;
        return errCode;
    }
#ifdef DEBUG
    for (int i = 0; i < NW + NW; i++)
        printf("ind=%d\t", wcTableIndex[i]);
    puts("");
#endif
    // - match where clause attrNames with RelAttr object
    // - match where clause sub clause with RelAttr
    // - turn single table where conditions into compop data such as type,
    // offset and length.
    wcLengths = new int[NW + NW];
    wcOffsets = new int[NW + NW];
    wcTypes = new AttrTypeAtom[NW + NW];
    for (int i = 0; i < NW + NW; i++) {
        if (wcTableIndex[i] < 0) // operand is ValueHolder
        {
            auto &val = wcOperands[i >> 1];
            // do type conversion!
            errCode = convertType(val, wcTypes[i - 1]);
            if (errCode) break;
            if (val.len && !val.isNull() &&
                !AttrTypeHelper::checkTypeCompliable(wcTypes[i - 1],
                                                     val.attrType)) {
                errCode = ATTRIBUTE_TYPE_MISMATCH;
                break;
            }
            if (val.isNull() && wcTypes[i - 1] == TYPE_INT)
                val = ValueHolder::makeNull(TYPE_INT);
            continue;
        }
        const char *attrName =
            i % 2 ? condition.judSet[i >> 1].rhs.attrName.c_str()
                  : condition.judSet[i >> 1].lhs.attrName.c_str();
        errCode = getOffsetAndType(tables[wcTableIndex[i]].c_str(), attrName,
                                   wcOffsets[i], wcLengths[i], wcTypes[i]);
        if (errCode) break;
        // /* || wcLengths[i] != wcLengths[i - 1])*/
        if (i % 2 &&
            (AttrTypeHelper::checkTypeCompliable(wcTypes[i], wcTypes[i - 1]))) {
            errCode = ATTRIBUTE_TYPE_MISMATCH;
            break;
        }
    }
    if (errCode) {
        // recycle
        delete[] wcTableIndex, wcOperands;
        delete[] wcOffsets, wcTypes, wcLengths;
        return errCode;
    }
#ifdef DEBUG
    for (int i = 0; i < NW + NW; i++)
        printf("off=%d len=%d type=%d\t", wcOffsets[i], wcLengths[i],
               wcTypes[i]);
    puts("");
#endif
    operations = vector<CompOp *>();
    for (int i = 0; i < NW; i++) {
        CompOp *newop = nullptr;
        switch (condition.judSet[i].op) {
        case WO_EQUAL:
            newop = new Equal(wcTypes[i << 1]);
            break;
        case WO_GREATER:
            newop = new Greater(wcTypes[i << 1]);
            break;
        case WO_LESS:
            newop = new Less(wcTypes[i << 1]);
            break;
        case WO_NEQUAL:
            newop = new NotEqual(wcTypes[i << 1]);
            break;
        case WO_ISNULL:
            newop = new IsNull(wcTypes[i << 1]);
            break;
        case WO_NOTNULL:
            newop = new IsNotNull(wcTypes[i << 1]);
            break;
        case WO_GE:
            newop = new GreaterEqual(wcTypes[i << 1]);
            break;
        case WO_LE:
            newop = new LessEqual(wcTypes[i << 1]);
            break;
        default:
            errCode = NOT_IMPLEMENTED_OP;
            break;
        }
        operations.push_back(newop);
        if (errCode) break;
    }
    if (errCode) {
        delete[] wcTableIndex, wcOperands;
        delete[] wcOffsets, wcTypes;
        for (auto i : operations)
            delete i;
        return errCode;
    }
    return 0;
}

// #define DEBUG
//
int QueryManager::select(vector<RelAttr> &selAttrs, vector<char *> &tableNames,
                         Condition &condition)
{
    for (auto i = selAttrs.begin(); i != selAttrs.end(); ++i) {
        if (i->attrRel == "") {
            if (tableNames.size() > 1) return AMBIGUOUS_TABLE;
            i->attrRel = tableNames[0];
            continue;
        }
        // - check attr.tableName in tableNames
        bool tableExist = false;
        for (auto j = tableNames.begin(); j != tableNames.end(); j++) {
            if (i->attrRel == *j) {
                tableExist = true;
                break;
            }
        }
        if (!tableExist) return MISSING_TABLE_IN_FROM_CLAUSE;
    }
    // - check multiple
    for (auto i = selAttrs.begin(); i != selAttrs.end(); ++i) {
        for (auto j = i + 1; j != selAttrs.end(); ++j) {
            if (i->attrName == j->attrName) {
                if (i->attrRel == j->attrRel) return DUPLICATED_ATTRS;
            }
        }
    }
#ifdef DEBUG
    for (auto i : selAttrs)
        printf("%s.%s\t", i.attrRel.c_str(), i.attrName.c_str());
    puts("");
#endif
    int NS = selAttrs.size();
    vector<string> tables;
    int *tableIndex;
    int *offsets;
    int *lengths;
    AttrTypeAtom *types;
    int errCode = 0;
    int index = 0;
    // used for results table header
    vector<string> selAttrNames;
    do {
        if (selAttrs[0].attrName == "*") {
            FileHandle *fh;
            if (RecordManager::quickOpen(mainTableFilename, fh)) {
                errCode = MAIN_TABLE_ERROR;
                break;
            }
            vector<int> off, len, tbIndex;
            vector<AttrTypeAtom> typ;
            for (int i = 0; i < tableNames.size(); i++) {
                auto &tbname = tableNames[i];
                tables.push_back(tbname);
                for (auto iter = fh->findRecord(
                         MAX_TABLE_NAME_LEN, 0,
                         Operation(Operation::EQUAL, TYPE_CHAR), tbname);
                     !iter.end(); ++iter) {
                    MRecord mrec = *iter;
                    off.push_back(RecordHelper::getOffset(mrec.d_ptr));
                    len.push_back(RecordHelper::getLength(mrec.d_ptr));
                    typ.push_back(RecordHelper::getType(mrec.d_ptr));
                    tbIndex.push_back(i);
                    index++;
                    string attrName;
                    if (tableNames.size() > 1)
                        attrName = string(tbname) + "." +
                                   RecordHelper::getAttrName(mrec.d_ptr);
                    else
                        attrName = RecordHelper::getAttrName(mrec.d_ptr);
                    selAttrNames.push_back(attrName);
                    delete[] mrec.d_ptr;
                }
            }
            // transfer to native array type
            NS = index;
            offsets = new int[index];
            lengths = new int[index];
            types = new AttrTypeAtom[index];
            tableIndex = new int[index];
            for (int i = 0; i < index; i++) {
                offsets[i] = off[i];
                lengths[i] = len[i];
                types[i] = typ[i];
                tableIndex[i] = tbIndex[i];
            }
            RecordManager::quickClose(fh);
        } else {
            types = new AttrTypeAtom[NS];
            tableIndex = new int[NS];
            offsets = new int[NS];
            lengths = new int[NS];
            // - appoint each attr to its corresponding table
            for (int index = 0; index < NS; index++) {
                auto i = selAttrs.begin() + index;
                tableIndex[index] = 0;
                for (; tableIndex[index] < tables.size(); tableIndex[index]++)
                    if (tables[tableIndex[index]] == i->attrRel) break;
                if (tableIndex[index] == tables.size())
                    tables.push_back(i->attrRel);
            }
#ifdef DEBUG
            for (auto name : tables)
                printf("%s\t", name.c_str());
            puts("");
            for (int i = 0; i < NS; i++)
                printf("%d\t", tableIndex[i]);
            puts("");
#endif
            for (auto i = selAttrs.begin(); i != selAttrs.end(); i++) {
                errCode = getOffsetAndType(i->attrRel.c_str(),
                                           i->attrName.c_str(), offsets[index],
                                           lengths[index], types[index]);
                if (errCode) break;
                index++;
            }
            if (errCode) {
                // recycle
                delete[] offsets, lengths, types, tables;
                return errCode;
            }
            for (auto i : selAttrs)
                if (tableNames.size() > 1)
                    selAttrNames.push_back(i.attrRel + "." + i.attrName);
                else
                    selAttrNames.push_back(i.attrName);
        }
    } while (false);

#ifdef DEBUG
    for (int i = 0; i < NS; i++)
        printf("off=%d len=%d type=%d\t", offsets[i], lengths[i], types[i]);
    puts("");
#endif
    int *wcTableIndex, *wcLengths, *wcOffsets;
    AttrTypeAtom *wcTypes;
    ValueHolder *wcOperands;
    vector<CompOp *> operations;
    errCode = parseWhereClause(condition, tables, wcTableIndex, wcLengths,
                               wcOffsets, wcTypes, wcOperands, operations);
    if (errCode) {
        delete[] offsets, lengths, types, tableIndex;
        return errCode;
    }
    PrintableTable *printer = new PrintableTable(selAttrNames);
    // - NAIVE version: brute force search over entire table
    selectOnCartesianProduct(tables, NS, tableIndex, offsets, lengths, types,
                             condition.judSet.size(), wcTableIndex, wcOffsets,
                             wcLengths, wcOperands, operations, printer);
    // - output results
    printer->show();
    delete[] wcTableIndex, wcOperands;
    delete[] wcOffsets, wcTypes;
    delete[] offsets, lengths, types, tableIndex;
    for (auto i : operations)
        delete i;
    if (errCode == 0) puts("OK");
    return 0;
}

//
// Insert the values into relName
//
int QueryManager::insert(const char *tableName,
                         vector<vector<ValueHolder>> valss)
{
    int errCode;
    for (auto &vals : valss) {
        errCode = insert(tableName, vals);
        if (errCode) return errCode;
    }
    return 0;
}
int QueryManager::insert(const char *tableName, vector<ValueHolder> vals)
{
    int errCode = 0;
    FileHandle *fh, *fht;
    RID rid;
    assert(RecordManager::quickOpen((string(tableName) + ".db").c_str(), fht) ==
           0);
    int RS = fht->getRecordSize();
    if (RecordManager::quickOpen(mainTableFilename, fh) != 0)
        return MAIN_TABLE_ERROR;
    char *buf = new char[RS];
    memset(buf, 0, RS);
    vector<bool> checked = vector<bool>(vals.size());
    int checkedNum = 0;
    for (auto iter =
             fh->findRecord(MAX_TABLE_NAME_LEN, 0,
                            Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        int offset = RecordHelper::getOffset(mrec.d_ptr);
        int length = RecordHelper::getLength(mrec.d_ptr);
        AttrTypeAtom type = RecordHelper::getType(mrec.d_ptr);
        int index = RecordHelper::getIndex(mrec.d_ptr);
        delete[] mrec.d_ptr;
        if (index >= vals.size()) {
            errCode = INCOMPLETE_INSERT_DATA;
            break;
        }
        assert(checked[index] == 0);
        checked[index] = 1;
        checkedNum++;
        errCode = convertType(vals[index], type);
        if (errCode) break;
        if (type != vals[index].attrType && !vals[index].isNull()) {
            // Special case: CHAR/VARCHAR -- CHAR
            if (type != TYPE_VARCHAR || vals[index].attrType != TYPE_CHAR) {
                errCode = INCOMPATIBLE_TYPE;
                break;
            }
        }
        if (type == TYPE_CHAR) {
            int len = strlen(vals[index].buf);
            if (len > length) len = length;
            memcpy(buf + offset, vals[index].buf, len);
        } else if (type == TYPE_VARCHAR) {
            int len = strlen(vals[index].buf);
            if (len > length) len = length;
            char *varstr = new char[len + 1];
            memcpy(varstr, vals[index].buf, len);
            varstr[len] = 0;
            RID varRID;
            fh->insertVariant(varstr, len, varRID);
            memcpy(buf + offset, &varRID, sizeof(RID));
        } else {
            if (type == TYPE_INT) {
                if (vals[index].isNull())
                    vals[index] = ValueHolder::makeNull(type);
            }
            memcpy(buf + offset, vals[index].buf, length);
        }
    }
    if (checkedNum == 0) errCode = TABLE_NOT_EXIST;
    if (errCode) goto finish;
    for (auto i : checked)
        if (!i) {
            errCode = TOO_MANY_ARGUMENTS;
            break;
        }
    if (checkedNum < vals.size()) errCode = TOO_MANY_ARGUMENTS;
    if (errCode) goto finish;
    // preparation is ok.insert!
    errCode = fht->insertRecord(buf, rid);
finish:
    RecordManager::quickClose(fh);
    RecordManager::quickClose(fht);
    delete[] buf;
    return errCode;
}

//
// Delete from the relName all tuples that satisfy conditions
//
int QueryManager::deletes(const char *tableName, Condition &condition)
{
    // check condition format
    for (auto t : condition.judSet) {
        // if (t.rhs.attrName != "") {
        //     return BAD_WHERE_FORMAT;
        // }
        if (t.lhs.attrRel != "" && t.lhs.attrRel != tableName)
            return BAD_WHERE_FORMAT;
        if (t.lhs.attrRel == "") t.lhs.attrRel = tableName;
    }
    // open table and check if table exists
    FileHandle *fh;
    if (RecordManager::quickOpenTable(tableName, fh)) return TABLE_NOT_EXIST;
    // parse where clause
    int *wcTableIndex, *wcLengths, *wcOffsets;
    AttrTypeAtom *wcTypes;
    ValueHolder *wcOperands;
    vector<CompOp *> wcOperations;
    int errCode =
        parseWhereClause(condition, {tableName}, wcTableIndex, wcLengths,
                         wcOffsets, wcTypes, wcOperands, wcOperations);
    if (errCode) {
        RecordManager::quickClose(fh);
        return errCode;
    }
    // make a list of variant data
    // brute search algorithm
    int numW = condition.judSet.size();
    for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
        MRecord mrec = *iter;
        // if satisfy condition?
        bool ok = testCompOpOnTables(numW, 0, &mrec, wcTableIndex, wcOffsets,
                                     wcLengths, wcOperands, wcOperations, fh);
        if (ok) {
            // TODO: remove variant data
            // remove top data
            fh->removeRecord(mrec.rid);
        }
        delete[] mrec.d_ptr;
    }
    RecordManager::quickClose(fh);
    for (auto o : wcOperations)
        delete o;
    delete[] wcTypes, wcLengths, wcOffsets, wcTableIndex, wcOperands;
    return 0;
}

//
// Update from the relName all tuples that satisfy conditions
//
int QueryManager::update(const char *tableName, vector<SetClause> &updSet,
                         Condition &condition)
{
    // check condition format
    for (auto t : condition.judSet) {
        // if (t.rhs.attrName != "") {
        //     return BAD_WHERE_FORMAT;
        // }
        if (t.lhs.attrRel != "" && t.lhs.attrRel != tableName)
            return BAD_WHERE_FORMAT;
        if (t.lhs.attrRel == "") t.lhs.attrRel = tableName;
    }
    // open table and check if table exists
    FileHandle *fh;
    if (RecordManager::quickOpenTable(tableName, fh)) return TABLE_NOT_EXIST;
    // parse where clause
    int *wcTableIndex, *wcLengths, *wcOffsets;
    AttrTypeAtom *wcTypes;
    ValueHolder *wcOperands;
    vector<CompOp *> wcOperations;
    int errCode =
        parseWhereClause(condition, {tableName}, wcTableIndex, wcLengths,
                         wcOffsets, wcTypes, wcOperands, wcOperations);
    if (errCode) {
        RecordManager::quickClose(fh);
        return errCode;
    }
    const int NU = updSet.size();
    // check update set and types
    int *updOffsets = new int[NU], *updLengths = new int[NU];
    memset(updOffsets, 255, sizeof(int) * NU);
    AttrTypeAtom *updTypes = new AttrTypeAtom[NU];
    FileHandle *fhm;
    assert(RecordManager::quickOpen(mainTableFilename, fhm) == 0);
    for (auto iter =
             fhm->findRecord(MAX_TABLE_NAME_LEN, 0,
                             Operation(Operation::EQUAL, TYPE_CHAR), tableName);
         !iter.end(); ++iter) {
        MRecord mrec = *iter;
        for (int i = 0; i < NU; i++) {
            if (RecordHelper::getAttrName(mrec.d_ptr) == updSet[i].attrName) {
                updOffsets[i] = RecordHelper::getOffset(mrec.d_ptr);
                updLengths[i] = RecordHelper::getLength(mrec.d_ptr);
                updTypes[i] = RecordHelper::getType(mrec.d_ptr);
                // type conversion
                errCode = convertType(updSet[i].val, updTypes[i]);
                if (errCode) break;
                // check if type compliable
                if (!updSet[i].val.isNull() &&
                    !AttrTypeHelper::checkTypeCompliable(
                        updTypes[i], updSet[i].val.attrType))
                    errCode = ATTRIBUTE_TYPE_MISMATCH;
                break;
            }
        }
        delete[] mrec.d_ptr;
    }
    do {
        if (errCode) break;
        // Any attr missed out? not exist!
        for (int i = 0; i < NU; i++) {
            if (updOffsets[i] < 0) {
                errCode = ATTR_NOT_EXIST;
            }
        }
        if (errCode) break;
        // edit it now!
        for (auto iter = fh->findRecord(); !iter.end(); ++iter) {
            MRecord mrec = *iter;
            if (testCompOpOnTables(condition.judSet.size(), 0, &mrec,
                                   wcTableIndex, wcOffsets, wcLengths,
                                   wcOperands, wcOperations, fh)) {
                for (int i = 0; i < NU; i++) {
                    ValueHolder &val = updSet[i].val;
                    if (updTypes[i] == TYPE_VARCHAR) {
                        RID varRID = *(RID *)(mrec.d_ptr + updOffsets[i]);
                        // valid? remove!
                        fh->removeVariant(varRID);
                        // buffer length <= max string length + 1 <= attr length
                        fh->insertVariant(val.buf, val.len + 1, varRID);
                        memcpy(mrec.d_ptr + updOffsets[i], &varRID,
                               sizeof(RID));
                    }
                    if (val.isNull() && updTypes[i] == TYPE_INT) {
                        val = ValueHolder::makeNull(TYPE_INT);
                    }
                    memset(mrec.d_ptr + updOffsets[i], 0, updLengths[i]);
                    // TODO: NULL?
                    memcpy(mrec.d_ptr + updOffsets[i], val.buf, val.len);
                }
                fh->updateRecord(mrec);
            }
            delete[] mrec.d_ptr;
        }
    } while (false);
    RecordManager::quickClose(fh);
    RecordManager::quickClose(fhm);
    for (auto o : wcOperations)
        delete o;
    delete[] updOffsets, updTypes, updLengths;
    delete[] wcTypes, wcLengths, wcOffsets, wcTableIndex, wcOperands;
    return errCode;
}

PrintableTable::PrintableTable(const vector<string> &headers)
    : headers(headers), row(new ValueHolder[headers.size()])
{
}

void PrintableTable::addRow()
{
    int N = headers.size();
    if (row != nullptr) {
        vals.push_back(row);
    }
    row = new ValueHolder[N];
}

void PrintableTable::setData(int index, const ValueHolder &v)
{
    row[index] = v;
}

void PrintableTable::show()
{
    int N = headers.size();
    for (int i = 0; i < N; i++) {
        printf("%s\t", headers[i].c_str());
    }
    puts("");
    for (auto v : vals) {
        for (int j = 0; j < N; j++) {
            if (v[j].isNull()) {
                printf("NULL\t");
            } else if (v[j].attrType == TYPE_INT) {
                int _tmp = *(int *)v[j].buf;
                printf("%d\t", _tmp);
            } else if (v[j].attrType == TYPE_NUMERIC) {
                Numeric _tmp = *(Numeric *)v[j].buf;
                printf("%s\t", _tmp.toString().c_str());
            } else if (v[j].attrType == TYPE_DATE) {
                Date _tmp = *(Date *)v[j].buf;
                printf("%s\t", _tmp.toString().c_str());
            } else if (v[j].attrType == TYPE_CHAR) {
                printf("%s\t", v[j].buf);
            } else {
                std::cerr << "ERROR: Not printable attribute. Attr ID="
                          << v[j].attrType << "." << std::endl;
            }
        }
        puts("");
    }
}

PrintableTable::~PrintableTable()
{
    if (row) delete[] row;
    for (auto i : vals)
        delete[] i;
}