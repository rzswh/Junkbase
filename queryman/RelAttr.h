#pragma once

#include <string>
using std::string;

struct RelAttr {
    string attrRel, attrName;
    RelAttr() {}
    RelAttr(const char *rel, const char *name) : attrRel(rel), attrName(name) {}
};