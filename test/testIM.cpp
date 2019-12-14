#include "test.h"
#include "../index/IndexHandle.h"
#include "../index/IndexManager.h"
#include "../index/bplus.h"
#include <cassert>
#include <algorithm>
#include <string>

void testIM1(int epoch = 1, bool newFile = false);
void testIM2(int epoch = 1);
void testIM3();

void testIM() {
    srand((unsigned)time(0));
    testIM1(1, true);
    testIM2();
    testIM1(2, false);
    testIM2();
    testIM1(3, false);
    testIM2();
    testIM1(4, false);
    testIM2();
    testIM3();
}

const char * file_name = "duplicate.db";

void testIM1(int epoch, bool newFile) {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    IndexManager * im = new IndexManager(bpm);
    if (newFile) {
        remove((std::string(file_name) + ".i0000").c_str());
        AttrType type = TYPE_INT | (TYPE_RID << 3);
        vector<int> lens; lens.push_back(4); lens.push_back(sizeof(RID));
        assert(im->createIndex(file_name, 0, type, lens) == 0);
    }
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
        // a[i] = x;
        ih->insertEntry((char*)(a + i), RID(epoch, a[i]));
    }
    ih->debug();
    // recycle
    im->closeIndex(*ih);
    delete fm, bpm, im, ih;
}

void testIM2(int epoch) {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    IndexManager * im = new IndexManager(bpm);
    IndexHandle *ih;
    assert(im->openIndex(file_name, 0, ih) == 0);
    const int MAX_SIZE = 683;
    int a[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
        a[i] = i;
    std::random_shuffle(a, a + MAX_SIZE);
    // remove
    for (int i = 0; i < 100; i++) {
        if (ih->deleteEntry((char*)(a + i), RID(epoch, a[i])) != 0) {
            printf("Delete error\n");
        }
    }
    ih->debug();
    // recycle
    im->closeIndex(*ih);
    delete ih;
    delete fm, bpm, im;
}

void testIM3() {
    FileManager * fm = new FileManager();
    BufPageManager * bpm = new BufPageManager(fm);
    IndexManager * im = new IndexManager(bpm);
    IndexHandle *ih;
    assert(im->openIndex(file_name, 0, ih) == 0);
    // search
    int a = 123456;
    for (auto iter = ih->findEntry(Operation(Operation::LESS, TYPE_INT), &a); !iter.end(); ++iter) {
        RID rid = *iter;
        printf("RID(%d, %d)\n", rid.getPageNum(), rid.getSlotNum());
    }
    // recycle
    im->closeIndex(*ih);
    delete fm, bpm, im, ih;
}