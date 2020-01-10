#pragma once

#include "FileHandle.h"
#include "MRecord.h"
#include "RID.h"
#include <string>
using std::string;

struct HeaderPage {
    int record_size;
    int next_empty_page;
    int next_empty_variant_data_page;
    int total_pages;
    HeaderPage()
        : record_size(0), next_empty_page(0), total_pages(0),
          next_empty_variant_data_page(0)
    {
    }
    HeaderPage(int recordSize, int nextEmptyPage, int nxtEmpVDPage,
               int totalPage)
    {
        record_size = recordSize;
        next_empty_page = nextEmptyPage;
        next_empty_variant_data_page = nxtEmpVDPage;
        total_pages = totalPage;
    }
};

class RecordManager
{
    BufPageManager *pm;

public:
    RecordManager(BufPageManager *pm);
    int createFile(const char *file_name, int record_size);
    int deleteFile(const char *file_name);
    int openFile(const char *file_name, FileHandle *&fh);
    static int closeFile(FileHandle &fh);
    static int quickOpen(const char *fileName, FileHandle *&fh,
                         int recordSize = 0)
    {
        RecordManager *rm = quickManager();
        if (rm->openFile(fileName, fh) != 0) {
            if (recordSize == 0) return 1;
            remove(fileName);
            if (rm->createFile(fileName, recordSize) != 0 ||
                rm->openFile(fileName, fh) != 0) {
                return 1;
            }
        }
        delete rm;
        return 0;
    }
    static int quickOpenTable(const char *tableName, FileHandle *&fh)
    {
        return quickOpen((string(tableName) + ".db").c_str(), fh);
    }
    static RecordManager *quickManager()
    {
        return new RecordManager(new BufPageManager(new FileManager()));
    }
    static void quickRecycleManager(RecordManager *rm)
    {
        delete rm->pm->fileManager;
        delete rm->pm;
        delete rm;
    }
    static int quickClose(FileHandle *fh)
    {
        RecordManager::closeFile(*fh);
        BufPageManager *bpm = fh->bpman;
        FileManager *fm = bpm->fileManager;
        delete fh, bpm, fm;
        return 0;
    }
};