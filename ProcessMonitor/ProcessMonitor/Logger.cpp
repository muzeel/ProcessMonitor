#include "Logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

static QFile* logFile = nullptr;
static QMutex mutex;

void Logger::init()
{
    logFile = new QFile("monitor.log");
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // если не открылся – работаем без лога (не критично)
        delete logFile;
        logFile = nullptr;
    }
}

void Logger::close()
{
    if (logFile) {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }
}

void Logger::log(const QString& message)
{
    QMutexLocker locker(&mutex);
    if (logFile && logFile->isOpen()) {
        QTextStream stream(logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - " << message << "\n";
        stream.flush();
    }
}