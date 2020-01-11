#include "../index/IndexManager.h"
#include "../recman/RecordManager.h"

int main()
{
    IndexHandle *ih;
    auto man = IndexManager::quickManager();
    vector<int> parm;
    parm.push_back(1);
    man->createIndex("_MAIN.db", 0, TYPE_CHAR, parm);
    IndexManager::quickRecycleManager(man);
    IndexManager::quickOpen("_MAIN.db", ih);
    IndexManager::quickClose(ih);
    return 0;
}