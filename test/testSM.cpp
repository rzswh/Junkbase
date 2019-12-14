#include "test.h"
#include "../sysman/sysman.h"

void create(SystemManager * sm) {
    // insert table
    AttrInfo attrsTest[3] = {
        AttrInfo("ID", true, true, false, TYPE_INT, 4),
        AttrInfo("Name", true, false, false, TYPE_CHAR, 20),
        AttrInfo("Address", false, false, false, TYPE_VARCHAR, 200)
    };
    int code = sm->createTable("Main", 3, attrsTest);
    printf("%d\n", code);
    AttrInfo attrsClass[3] = {
        AttrInfo("Class Name", true, false, false, TYPE_CHAR, 50),
        AttrInfo("Student Name", true, false, false, TYPE_CHAR, 20),
        AttrInfo("Student ID", true, false, true, TYPE_INT, 4, nullptr, "Main", "ID")
    };
    code = sm->createTable("Classes", 3, attrsClass);
    printf("%d\n", code);
}

void indexes(SystemManager * sm) {
    sm->dropPrimaryKey("Main");
    vector<const char *> columns;
    columns.push_back("Class Name");
    sm->addPrimaryKey("Classes", columns);
}

void testSM() {
    FileManager * fm1 = new FileManager();
    BufPageManager * bpm1 = new BufPageManager(fm1);
    RecordManager * rm = new RecordManager(*bpm1);
    FileManager * fm2 = new FileManager();
    BufPageManager * bpm2 = new BufPageManager(fm2);
    IndexManager * im = new IndexManager(bpm2);
    SystemManager * sm = new SystemManager(rm, im);
    create(sm);
    pageViewer("_MAIN.db");
    pageViewer("Classes.db");
    pageViewer("Main.db");
    indexes(sm);
    // sm->dropPrimaryKey("Main");
    delete fm1, bpm1, fm2, bpm2, rm, im, sm;
}
