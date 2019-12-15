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
    columns.clear();
    columns.push_back("ID");
    columns.push_back("Name");
    sm->addForeignKey("Main", "testForeign", columns);
}

void testSM() {
    SystemManager * sm = new SystemManager();
    create(sm);
    pageViewer("_MAIN.db");
    pageViewer("Classes.db");
    pageViewer("Main.db");
    pageViewer("_INDEX.db");
    indexes(sm);
    // sm->dropPrimaryKey("Main");
    delete sm;
}
