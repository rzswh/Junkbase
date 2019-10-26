#pragma once


class RID {
    int pageNum;
    int slotNum;
public:
    RID() : pageNum(0), slotNum(0) {}
    RID(int pn, int sn) : pageNum(pn), slotNum(sn) {}
    int getPageNum() const { return pageNum; }
    int getSlotNum() const { return slotNum; }
};
