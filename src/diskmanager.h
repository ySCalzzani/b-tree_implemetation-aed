#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <string>
#include <fstream>

/* ----------------------------------------------------------------------------
* DiskManager: Classe para as estatísticas de I/O (ex: número de leituras e escritas no disco).
* ------------------------------------------------------------------------- */
class DiskManager {
private:
    int readCount = 0;
    int writeCount = 0;
    std::string filename;

public:
    DiskManager(const std::string& fname) : filename(fname) {}

    void incrementRead() { readCount++; }
    void incrementWrite() { writeCount++; }
    
    int getReadCount() const { return readCount; }
    int getWriteCount() const { return writeCount; }
    void resetCounters() { readCount = 0; writeCount = 0; }

    long getFileSize() {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return 0;
        return file.tellg();
    }
};

#endif