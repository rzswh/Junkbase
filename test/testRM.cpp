#include "test.h"
#include "../recman/RecordManager.h"
#include "../recman/RID.h"
#include <assert.h>
#include <cstring>

struct Record {
    int age;
    int stu_id;
};

RID insert(FileHandle *fh, int age, int stu_id) {
    RID rid;
    Record record = (Record) { age, stu_id };
    fh->insertRecord((char*)&record, rid);
    return rid;
}

void testRM1();
void testRM2();
void testRM3();
void testRMVar1();

void testRM() { 
    testRM1();
    testRM2();
    testRM3();
    pageViewer("test1.db");
    testRMVar1();
}

/**
 * Create record file "test1.db" and open it. Record length=8
 * Insert 3 records
 * Get a record and update it
 * Remove the second record
 * Batch insert: 4096 records
 */ 
void testRM1() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    RecordManager * rm = new RecordManager(*bpm);
    remove("test1.db");
    assert(rm->createFile("test1.db", sizeof(Record)) == 0);
    FileHandle *fh;
    assert(rm->openFile("test1.db", fh) == 0);
    // single insert
    RID rid1, rid2, rid3, rid4;
    rid1 = insert(fh, 17, 2011011865);
    rid2 = insert(fh, 31, 2017012345);
    rid3 = insert(fh, 31, 2017012345);
    Record record1 = (Record){17, 2011011865};
    Record record3 = (Record){65, 65535};
    // get & update
    MRecord mrec1;
    fh->getRecord(rid1, mrec1);
    assert(memcmp(mrec1.d_ptr, &record1, sizeof(Record)) == 0);
    record1.age = 2147483647;
    memcpy(mrec1.d_ptr, &record1, sizeof(Record));
    fh->updateRecord(mrec1);
    // remove
    fh->removeRecord(rid2);
    // batch insert
    for (int i = 0; i < 128 * 32; i++)
        rid4 = insert(fh, 65, i);
    // recycle
    rm->closeFile(*fh);
    delete fm, bpm, rm, fh;
}
void testRM2() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    RecordManager * rm = new RecordManager(*bpm);
    FileHandle *fh;
    assert(rm->openFile("test1.db", fh) == 0);
    // remove!
    RID rid = RID(3, 2);
    fh->removeRecord(rid);
    rid = RID(4, 5);
    fh->removeRecord(rid);
    fh->removeRecord(insert(fh, 11, 222));
    // recycle
    rm->closeFile(*fh);
    delete fm, bpm, rm, fh;
}

void testRM3() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    RecordManager * rm = new RecordManager(*bpm);
    FileHandle *fh;
    assert(rm->openFile("test1.db", fh) == 0);
    // scanning
    int operand = 0, counter = 0;
    for (FileHandle::iterator iter = fh->findRecord(4, 0, Operation(Operation::EQUAL, TYPE_INT), &operand); 
         !iter.end(); ++iter)
    {
        counter ++;
    }
    assert(counter == 0);
    // recycle
    rm->closeFile(*fh);
    delete fm, bpm, rm, fh;
}

int pageViewer(const char * filename) {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    RecordManager * rm = new RecordManager(*bpm);
    FileHandle *fh;
    if (rm->openFile(filename, fh) != 0) return 1;
    fh->debug();
    delete fm, bpm, rm, fh;
    return 0;
}

void testRMVar1() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    RecordManager * rm = new RecordManager(*bpm);
    FileHandle *fh;
    if (rm->openFile("test1.db", fh) != 0) return;
    RID rid1, rid2, rid3;
    fh->insertVariant("123", 3, rid1);
    fh->insertVariant("123456789012345678901234567890123456789012345678901234567890", 60, rid2);
    char buf1[50], buf2[100];
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    fh->getVariant(rid1, buf1, 49);
    printf("%s\n", buf1);
    fh->getVariant(rid2, buf1, 49);
    printf("%s\n", buf1);
    fh->getVariant(rid2, buf2, 99);
    printf("%s\n", buf2);
    fh->removeVariant(rid1);
    memset(buf2, 0, sizeof(buf2));
    fh->insertVariant("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890", 90, rid3);
    fh->getVariant(rid3, buf2, 99);
    printf("%s\n", buf2);
    rm->closeFile(*fh);
    delete fm, bpm, rm, fh;
}

