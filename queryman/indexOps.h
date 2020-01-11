#pragma once
#include "../def.h"
#include "../index/IndexHandle.h"
#include "../recman/RID.h"

int indexTryInsert(IndexHandle *ih, bool isKey, int N, int *offsets,
                   int *lengths, AttrTypeAtom *types, void *buf, const RID &rid,
                   bool recover);

typedef decltype(indexTryInsert) IdxOps;

int doForEachIndex(const char *tableName, IdxOps callback, void *buf,
                   const RID &rid, FileHandle *fh);