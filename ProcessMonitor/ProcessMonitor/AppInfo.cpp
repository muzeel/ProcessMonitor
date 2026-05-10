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
        if (ProcessMonitor::terminateProcess(pid)) {
            pid = 0;
            status = "Stopped";
            autoRestart = false;
            Logger::log("Terminated by user: " + displayName());
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