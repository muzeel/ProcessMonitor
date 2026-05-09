#ifndef PROCESSMONITOR_H
#define PROCESSMONITOR_H

#include <QString>

/**
 * Статический класс для работы с системными процессами Windows.
 */
class ProcessMonitor
{
public:
    static bool isProcessRunning(const QString& filePath, int& outPid);
    static bool terminateProcess(int pid);
    static bool launchProcess(const QString& filePath, const QString& arguments, int& outPid);
    static QString getProcessStatus(int pid); // "Running" или "Not Responding"
};

#endif // PROCESSMANAGER_H