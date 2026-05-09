#include "AppInfo.h"
#include "ProcessMonitor.h"
#include "Logger.h"
#include <QFileInfo>

AppInfo::AppInfo() : delaySeconds(0), autoRestart(true), pid(0), delayTimer(nullptr) {}

AppInfo::AppInfo(const QString& filePath, const QString& arguments, int delaySeconds)
    : filePath(filePath), arguments(arguments), delaySeconds(delaySeconds),
    autoRestart(true), pid(0), delayTimer(nullptr) {
}

AppInfo::~AppInfo()
{
    if (delayTimer) {
        delayTimer->stop();
        delete delayTimer;
    }
}

void AppInfo::launch()
{
    if (ProcessMonitor::launchProcess(filePath, arguments, pid)) {
        status = "Running";
        Logger::log("Launched: " + displayName() + " (PID: " + QString::number(pid) + ")");
    }
    else {
        Logger::log("Failed to launch: " + displayName());
        pid = 0;
        status = "Stopped";
    }
}

void AppInfo::terminate()
{
    Logger::log(QString("Terminate called for %1, current PID=%2, autoRestart=%3")
        .arg(displayName()).arg(pid).arg(autoRestart));

    if (pid != 0) {
        // Дополнительная проверка: реально ли процесс с таким PID существует?
        int checkPid = 0;
        bool exists = ProcessMonitor::isProcessRunning(filePath, checkPid);
        Logger::log(QString("Before termination: process exists? %1, found PID=%2").arg(exists).arg(checkPid));

        if (ProcessMonitor::terminateProcess(pid)) {
            // После supposed termination - проверим, действительно ли процесс исчез
            int afterPid = 0;
            bool stillExists = ProcessMonitor::isProcessRunning(filePath, afterPid);
            Logger::log(QString("After terminateProcess: process still exists? %1, PID=%2")
                .arg(stillExists).arg(afterPid));

            if (ProcessMonitor::terminateProcess(pid)) {
                // Принудительно сбрасываем autoRestart, даже если есть другие экземпляры
                Logger::log("Terminated by user: " + displayName() + " (PID: " + QString::number(pid) + ")");
                pid = 0;
                status = "Stopped";
                autoRestart = false;   // <-- сбрасываем всегда после попытки завершения
                // Но если процесс всё ещё жив (stillExists), но это уже другой экземпляр,
                // то мы его не трогаем и не перезапускаем.
            }
        }
        else {
            Logger::log("Failed to terminate: " + displayName());
        }
    }
    else {
        Logger::log("Terminate called but pid is 0, nothing to do");
    }
}

void AppInfo::updateStatus()
{
    int newPid;
    bool running = ProcessMonitor::isProcessRunning(filePath, newPid);
    if (running) {
        if (pid != newPid) {
            pid = newPid;
            Logger::log("Process detected: " + displayName() + " (PID: " + QString::number(pid) + ")");
        }
        QString newStatus = ProcessMonitor::getProcessStatus(pid);
        if (status != newStatus) {
            status = newStatus;
            Logger::log("Status changed for " + displayName() + ": " + status);
        }
    }
    else {
        if (pid != 0) {
            Logger::log("Process stopped: " + displayName());
            pid = 0;
            status = "Stopped";
        }
    }
}

QString AppInfo::displayName() const
{
    return QFileInfo(filePath).fileName();
}