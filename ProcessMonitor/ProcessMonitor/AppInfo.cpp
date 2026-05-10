/**
 * @file AppInfo.cpp
 * @brief Реализация класса AppInfo
 *
 * Файл содержит реализацию класса AppInfo,
 * представляющего одно отслеживаемое приложение.
 *
 * Основные функции класса:
 * - запуск процесса
 * - завершение процесса
 * - мониторинг состояния
 * - хранение параметров запуска
 * - логирование событий
 */

#include "AppInfo.h"
#include "ProcessMonitor.h"
#include "Logger.h"

#include <QFileInfo>

 /**
  * @brief Конструктор по умолчанию
  *
  * Создаёт пустой объект приложения.
  *
  * Начальные значения:
  * - delaySeconds = 0
  * - autoRestart = true
  * - pid = 0
  * - delayTimer = nullptr
  *
  * PID = 0 означает,
  * что процесс не запущен.
  */
AppInfo::AppInfo()
    : delaySeconds(0),
    autoRestart(true),
    pid(0),
    delayTimer(nullptr)
{
}

/**
 * @brief Конструктор с параметрами
 * @param filePath Путь к исполняемому файлу
 * @param arguments Аргументы командной строки
 * @param delaySeconds Задержка запуска в секундах
 *
 * Создаёт объект приложения
 * с указанными параметрами запуска.
 */
AppInfo::AppInfo(const QString& filePath,
    const QString& arguments,
    int delaySeconds)
    : filePath(filePath),
    arguments(arguments),
    delaySeconds(delaySeconds),
    autoRestart(true),
    pid(0),
    delayTimer(nullptr)
{
}

/**
 * @brief Деструктор класса AppInfo
 *
 * Освобождает ресурсы объекта.
 *
 * Если существует активный таймер задержки:
 * - таймер останавливается
 * - память освобождается
 *
 * Это предотвращает:
 * - утечки памяти
 * - обращения к удалённому объекту
 * - ошибки асинхронного выполнения
 */
AppInfo::~AppInfo()
{
    if (delayTimer)
    {
        delayTimer->stop();
        delete delayTimer;
    }
}

/**
 * @brief Запуск приложения
 *
 * Выполняет запуск процесса
 * через ProcessMonitor::launchProcess().
 *
 * При успешном запуске:
 * - сохраняется PID процесса
 * - статус меняется на "Running"
 * - событие записывается в лог
 *
 * При ошибке:
 * - PID сбрасывается в 0
 * - статус становится "Stopped"
 * - ошибка фиксируется в журнале
 */
void AppInfo::launch()
{
    // Попытка запуска процесса
    if (ProcessMonitor::launchProcess(
        filePath,
        arguments,
        pid))
    {
        // Успешный запуск
        status = "Running";

        Logger::log(
            "Launched: " +
            displayName() +
            " (PID: " +
            QString::number(pid) +
            ")");
    }
    else
    {
        // Ошибка запуска
        Logger::log(
            "Failed to launch: " +
            displayName());

        pid = 0;
        status = "Stopped";
    }
}

/**
 * @brief Завершение процесса
 *
 * Выполняет принудительное завершение
 * процесса по PID.
 *
 * После успешного завершения:
 * - pid сбрасывается в 0
 * - статус становится "Stopped"
 * - отключается autoRestart
 *
 * Отключение autoRestart необходимо,
 * чтобы пользователь мог вручную остановить процесс
 * без автоматического повторного запуска.
 *
 * Используется WinAPI-функция TerminateProcess().
 */
void AppInfo::terminate()
{
    Logger::log(
        QString("Terminate called for %1, PID=%2, autoRestart=%3")
        .arg(displayName())
        .arg(pid)
        .arg(autoRestart));

    // Проверяем наличие активного процесса
    if (pid != 0)
    {
        // Попытка завершения процесса
        if (ProcessMonitor::terminateProcess(pid))
        {
            // Процесс успешно завершён
            pid = 0;

            status = "Stopped";

            // Отключаем автоперезапуск
            autoRestart = false;

            Logger::log(
                "Terminated by user: " +
                displayName());
        }
        else
        {
            // Ошибка завершения
            Logger::log(
                "Failed to terminate: " +
                displayName());
        }
    }
    else
    {
        // Процесс уже не запущен
        Logger::log(
            "Terminate called but pid is 0, nothing to do");
    }
}

/**
 * @brief Обновление состояния процесса
 *
 * Периодически вызывается системой мониторинга.
 *
 * Алгоритм работы:
 * 1. Проверяется наличие процесса
 * 2. При необходимости обновляется PID
 * 3. Проверяется состояние процесса
 * 4. Обновляется статус
 * 5. Изменения фиксируются в логе
 *
 * Возможные статусы:
 * - Running
 * - Not Responding
 * - Stopped
 */
void AppInfo::updateStatus()
{
    int newPid;

    // Проверка наличия процесса
    bool running =
        ProcessMonitor::isProcessRunning(
            filePath,
            newPid);

    if (running)
    {
        // Процесс найден

        // Проверяем изменение PID
        if (pid != newPid)
        {
            pid = newPid;

            Logger::log(
                "Process detected: " +
                displayName() +
                " (PID: " +
                QString::number(pid) +
                ")");
        }

        // Получение текущего состояния процесса
        QString newStatus =
            ProcessMonitor::getProcessStatus(pid);

        // Проверяем изменение статуса
        if (status != newStatus)
        {
            status = newStatus;

            Logger::log(
                "Status changed for " +
                displayName() +
                ": " +
                status);
        }
    }
    else
    {
        // Процесс не найден

        // Если ранее был активен
        if (pid != 0)
        {
            Logger::log(
                "Process stopped: " +
                displayName());

            pid = 0;
            status = "Stopped";
        }
    }
}

/**
 * @brief Получение имени приложения
 * @return Имя исполняемого файла
 *
 * Из полного пути извлекается только имя файла.
 *
 * Пример:
 * C:\Apps\program.exe -> program.exe
 */
QString AppInfo::displayName() const
{
    return QFileInfo(filePath).fileName();
}