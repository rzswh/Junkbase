#include "../sysman/sysman.h"
#include "test.h"

void create(SystemManager *sm)
{
    // insert table
    vector<AttrInfo> attrsTest = {
        AttrInfo("ID", true, TYPE_INT, 4),
        AttrInfo("Name", true, TYPE_CHAR, 20),
        AttrInfo("Username", true, TYPE_CHAR, 20),
        AttrInfo("Address", false, TYPE_VARCHAR, 200),
        // AttrInfo("ID")
    };
    int code = sm->createTable("Main", attrsTest);
    printf("%d\n", code);
    vector<AttrInfo> attrsClass = {
        AttrInfo("Class Name", true, TYPE_CHAR, 50),
        AttrInfo("Student Name", true, TYPE_CHAR, 20),
        AttrInfo("User Name", true, TYPE_CHAR, 20),
        AttrInfo("Student ID", true, TYPE_INT, 4),
        // AttrInfo("Student ID", "Main", "ID")
    };
    code = sm->createTable("Classes", attrsClass);
    printf("%d\n", code);
}

void indexes(SystemManager *sm)
{
    sm->dropPrimaryKey("Main");
    vector<const char *> columns;
    columns.push_back("Class Name");
    sm->addPrimaryKey("Classes", columns);
    columns = {"Username", "Name"};
    vector<const char *> refColumns = {"User Name", "Student Name"};
    sm->addForeignKey("testForeign", "Main", columns, "Classes", refColumns);
}

void testSM()
{
    SystemManager *sm = new SystemManager();
    create(sm);
    pageViewer("_MAIN.db");
    pageViewer("Classes.db");
    pageViewer("Main.db");
    pageViewer("_INDEX.db");
    indexes(sm);
    // sm->dropPrimaryKey("Main");
    delete sm;
}
