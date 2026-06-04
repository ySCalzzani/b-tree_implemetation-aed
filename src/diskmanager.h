#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <string>
#include <fstream>
#include <chrono>

/* ----------------------------------------------------------------------------
* DiskManager: Classe para as estatísticas de I/O (ex: número de leituras e escritas no disco).
* ------------------------------------------------------------------------- */
class DiskManager {
private:
    int readCount = 0;
    int writeCount = 0;
    std::string filename;

    double ioTime = 0.0;
    std::chrono::time_point<std::chrono::high_resolution_clock> startIO;

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

    void startTimer() {
        startIO = std::chrono::high_resolution_clock::now();
    }

    void stopTimer() {
        auto endIO = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = endIO - startIO;
        ioTime += diff.count();
    }

    double getIoTime() const { return ioTime; }
    
    void resetIoTime() { ioTime = 0.0; }
};

#endif