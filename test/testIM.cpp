#include "test.h"
#include "../index/IndexHandle.h"
#include "../index/IndexManager.h"
#include "../index/bplus.h"
#include <cassert>
#include <algorithm>

void testIM1();
void testIM2();
void testIM3();

void testIM() {
    srand((unsigned)time(0));
    testIM1();
    //testIM2();
    testIM3();
}

const char * file_name = "nodenum.db";

void testIM1() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    IndexManager * im = new IndexManager(bpm);
    // assert(im->createIndex(file_name, 0, TYPE_INT, 4) == 0);
    IndexHandle *ih;
    assert(im->openIndex(file_name, 0, ih) == 0);
    const int MAX_SIZE = 683;
    int a[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
        a[i] = i;
    std::random_shuffle(a, a + MAX_SIZE);
    // insert
    for (int i = 0; i < MAX_SIZE; i++) {
        int x = rand();
        a[i] = x;
        ih->insertEntry((char*)(a + i), RID(1, a[i]));
    }
    // recycle
    im->closeIndex(*ih);
    delete fm, bpm, im, ih;
}

void testIM2() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    IndexManager * im = new IndexManager(bpm);
    IndexHandle *ih;
    assert(im->openIndex(file_name, 0, ih) == 0);
    const int MAX_SIZE = 100;
    int a[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
        a[i] = i;
    // remove
    for (int i = 0; i < MAX_SIZE / 2; i++) {
        int x = rand();
        ih->deleteEntry((char*)(a + i), RID(1, a[i]));
    }
    // recycle
    im->closeIndex(*ih);
    delete fm, bpm, im, ih;
}

void testIM3() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    IndexManager * im = new IndexManager(bpm);
    IndexHandle *ih;
    assert(im->openIndex(file_name, 0, ih) == 0);
    // search
    int a = 12495558;
    for (auto iter = ih->findEntry(Operation(Operation::LESS, TYPE_INT), &a); !iter.end(); ++iter) {
        RID rid = *iter;
        printf("RID(%d, %d)\n", rid.getPageNum(), rid.getSlotNum());
    }
    // recycle
    im->closeIndex(*ih);
    delete fm, bpm, im, ih;
}