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
    int readCount = 0;        // total de leituras de nó no disco
    int writeCount = 0;       // total de escritas de nó no disco
    std::string filename;     // arquivo .dat associado

    double ioTime = 0.0;      // tempo acumulado gasto em I/O (segundos)
    std::chrono::time_point<std::chrono::high_resolution_clock> startIO;

public:
    DiskManager(const std::string& fname) : filename(fname) {}

    // Contadores de operações de disco.
    void incrementRead() { readCount++; }
    void incrementWrite() { writeCount++; }

    int getReadCount() const { return readCount; }
    int getWriteCount() const { return writeCount; }
    void resetCounters() { readCount = 0; writeCount = 0; }

    // Tamanho atual do arquivo em bytes (abre no fim com ios::ate e lê a posição).
    long getFileSize() {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return 0;
        return file.tellg();
    }

    // Marca o início de uma operação de I/O.
    void startTimer() {
        startIO = std::chrono::high_resolution_clock::now();
    }

    // Marca o fim e soma a duração ao tempo acumulado de I/O.
    void stopTimer() {
        auto endIO = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = endIO - startIO;
        ioTime += diff.count();
    }

    double getIoTime() const { return ioTime; }

    void resetIoTime() { ioTime = 0.0; }
};

#endif