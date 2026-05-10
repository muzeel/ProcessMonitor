/**
 * @file Logger.cpp
 * @brief Реализация системы логирования приложения
 *
 * Файл содержит реализацию класса Logger,
 * предназначенного для записи событий приложения
 * в текстовый лог-файл monitor.log.
 *
 * Основные возможности:
 * - инициализация лог-файла
 * - потокобезопасная запись сообщений
 * - автоматическое добавление временных меток
 * - корректное завершение работы логгера
 */

#include "Logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

 /**
  * @brief Глобальный указатель на файл лога
  *
  * Используется всеми методами Logger.
  * Если файл не открыт — содержит nullptr.
  */
static QFile* logFile = nullptr;

/**
 * @brief Мьютекс для синхронизации записи
 *
 * Обеспечивает потокобезопасную работу логгера.
 */
static QMutex mutex;

/**
 * @brief Инициализация логирования
 *
 * Создаёт и открывает файл monitor.log
 * в режиме дозаписи.
 *
 * Используемые флаги:
 * - WriteOnly  — запись в файл
 * - Append     — добавление в конец файла
 * - Text       — текстовый режим
 *
 * Если файл открыть невозможно,
 * логирование автоматически отключается.
 */
void Logger::init()
{
    // Создание объекта файла
    logFile = new QFile("monitor.log");

    // Попытка открытия файла
    if (!logFile->open(
        QIODevice::WriteOnly |
        QIODevice::Append |
        QIODevice::Text))
    {
        // Ошибка открытия файла
        delete logFile;
        logFile = nullptr;
    }
}

/**
 * @brief Завершение работы логгера
 *
 * Корректно закрывает файл лога
 * и освобождает выделенную память.
 *
 * Обычно вызывается при завершении приложения.
 */
void Logger::close()
{
    // Проверяем, открыт ли файл
    if (logFile)
    {
        // Закрытие файла
        logFile->close();

        // Освобождение памяти
        delete logFile;

        // Сброс указателя
        logFile = nullptr;
    }
}

/**
 * @brief Запись сообщения в лог-файл
 * @param message Текст сообщения
 *
 * Формат записи:
 * yyyy-MM-dd hh:mm:ss - сообщение
 *
 * Пример:
 * 2026-05-10 14:35:12 - Application launched
 *
 * Метод потокобезопасен благодаря QMutexLocker.
 */
void Logger::log(const QString& message)
{
    // Автоматическая блокировка мьютекса
    QMutexLocker locker(&mutex);

    // Проверка доступности файла
    if (logFile && logFile->isOpen())
    {
        // Поток записи в файл
        QTextStream stream(logFile);

        // Формирование строки лога
        stream << QDateTime::currentDateTime()
            .toString("yyyy-MM-dd hh:mm:ss")
            << " - "
            << message
            << "\n";

        // Принудительная запись на диск
        stream.flush();
    }
}