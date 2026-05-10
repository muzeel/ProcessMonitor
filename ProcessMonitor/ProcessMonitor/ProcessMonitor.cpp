/**
 * @file ProcessMonitor.cpp
 * @brief Реализация функций мониторинга и управления процессами Windows
 *
 * Файл содержит реализацию класса ProcessMonitor,
 * использующего WinAPI для взаимодействия
 * с процессами операционной системы Windows.
 *
 * Реализованные возможности:
 * - поиск запущенных процессов
 * - запуск приложений
 * - завершение процессов
 * - проверка состояния окна процесса
 *
 * Используемые WinAPI:
 * - CreateToolhelp32Snapshot
 * - Process32First / Process32Next
 * - OpenProcess
 * - TerminateProcess
 * - CreateProcessW
 * - EnumWindows
 * - SendMessageTimeout
 */

#include "ProcessMonitor.h"
#include "Logger.h"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <QDebug>
#include <QFileInfo>

 /**
  * @brief Подключение библиотеки psapi.lib
  *
  * Необходима для функций:
  * - GetModuleFileNameExW
  * - работы с информацией о процессах
  */
#pragma comment(lib, "psapi.lib")

  /**
   * @brief Проверка наличия запущенного процесса
   * @param filePath Полный путь к executable-файлу
   * @param outPid Переменная для сохранения PID найденного процесса
   * @return true — процесс найден
   *
   * Алгоритм работы:
   * 1. Создание снимка всех процессов системы
   * 2. Перебор списка процессов
   * 3. Сравнение имени executable-файла
   * 4. Проверка полного пути процесса
   * 5. Возврат PID найденного процесса
   *
   * Поиск выполняется без учёта регистра.
   */
bool ProcessMonitor::isProcessRunning(const QString& filePath,
    int& outPid)
{
    // Получаем имя файла из полного пути
    QFileInfo fi(filePath);

    QString targetName =
        fi.fileName().toLower();

    // Проверяем корректность имени
    if (targetName.isEmpty())
        return false;

    /**
     * @brief Создание снимка процессов системы
     *
     * TH32CS_SNAPPROCESS —
     * получить список всех процессов.
     */
    HANDLE hSnapshot =
        CreateToolhelp32Snapshot(
            TH32CS_SNAPPROCESS,
            0);

    // Проверка успешности создания snapshot
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    DWORD foundPid = 0;

    // Начинаем перечисление процессов
    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            // Получаем имя текущего процесса
            QString procName =
                QString::fromWCharArray(pe.szExeFile)
                .toLower();

            // Проверяем совпадение имени
            if (procName == targetName)
            {
                /**
                 * @brief Открываем процесс
                 *
                 * PROCESS_QUERY_INFORMATION —
                 * доступ к информации процесса
                 *
                 * PROCESS_VM_READ —
                 * чтение памяти процесса
                 */
                HANDLE hProcess =
                    OpenProcess(
                        PROCESS_QUERY_INFORMATION |
                        PROCESS_VM_READ,
                        FALSE,
                        pe.th32ProcessID);

                if (hProcess)
                {
                    wchar_t pathBuf[MAX_PATH];

                    // Получение полного пути executable-файла
                    if (GetModuleFileNameExW(
                        hProcess,
                        NULL,
                        pathBuf,
                        MAX_PATH) > 0)
                    {
                        QString fullPath =
                            QString::fromWCharArray(pathBuf)
                            .toLower();

                        /**
                         * @brief Проверка совпадения пути
                         *
                         * contains() используется
                         * для более гибкого сравнения путей.
                         */
                        if (fullPath.contains(targetName))
                        {
                            foundPid = pe.th32ProcessID;

                            CloseHandle(hProcess);

                            // Процесс найден
                            break;
                        }
                    }

                    CloseHandle(hProcess);
                }
            }

        } while (Process32NextW(hSnapshot, &pe));
    }

    // Освобождаем snapshot
    CloseHandle(hSnapshot);

    // Проверяем результат поиска
    if (foundPid != 0)
    {
        outPid = foundPid;
        return true;
    }

    return false;
}

/**
 * @brief Принудительное завершение процесса
 * @param pid Идентификатор процесса
 * @return true при успешном завершении
 *
 * Использует WinAPI-функцию TerminateProcess().
 *
 * Если процесс уже завершился —
 * метод также возвращает true.
 */
bool ProcessMonitor::terminateProcess(int pid)
{
    /**
     * @brief Открытие процесса
     *
     * PROCESS_TERMINATE —
     * право завершения процесса
     *
     * PROCESS_QUERY_INFORMATION —
     * запрос информации о процессе
     */
    HANDLE hProcess =
        OpenProcess(
            PROCESS_TERMINATE |
            PROCESS_QUERY_INFORMATION,
            FALSE,
            pid);

    // Проверяем успешность открытия
    if (!hProcess)
    {
        DWORD err = GetLastError();

        Logger::log(
            QString("OpenProcess failed for PID %1, error: %2")
            .arg(pid)
            .arg(err));

        // Частая причина ошибки
        if (err == ERROR_ACCESS_DENIED)
        {
            Logger::log(
                "Access denied. Try running as administrator.");
        }

        return false;
    }

    /**
     * @brief Проверяем состояние процесса
     *
     * STILL_ACTIVE —
     * процесс ещё выполняется.
     */
    DWORD exitCode = 0;

    GetExitCodeProcess(hProcess, &exitCode);

    // Процесс уже завершён
    if (exitCode != STILL_ACTIVE)
    {
        CloseHandle(hProcess);
        return true;
    }

    // Принудительное завершение процесса
    BOOL result =
        TerminateProcess(hProcess, 1);

    DWORD err = GetLastError();

    CloseHandle(hProcess);

    if (result)
    {
        Logger::log(
            QString("TerminateProcess succeeded for PID %1")
            .arg(pid));

        return true;
    }
    else
    {
        Logger::log(
            QString("TerminateProcess failed for PID %1, error: %2")
            .arg(pid)
            .arg(err));

        return false;
    }
}

/**
 * @brief Запуск нового процесса
 * @param filePath Путь к executable-файлу
 * @param arguments Аргументы командной строки
 * @param outPid Переменная для сохранения PID процесса
 * @return true при успешном запуске
 *
 * Использует CreateProcessW().
 *
 * Формируемая командная строка:
 * "path_to_exe" arguments
 */
bool ProcessMonitor::launchProcess(const QString& filePath,
    const QString& arguments,
    int& outPid)
{
    // Формирование командной строки
    QString cmd =
        QString("\"%1\"").arg(filePath);

    // Добавляем аргументы
    if (!arguments.isEmpty())
        cmd += " " + arguments;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    // Обнуление структур
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    // Обязательное поле структуры
    si.cb = sizeof(si);

    /**
     * @brief Преобразование QString -> wchar_t*
     *
     * WinAPI CreateProcessW()
     * использует wide-строки.
     */
    std::wstring cmdWide =
        cmd.toStdWString();

    wchar_t* cmdLine = &cmdWide[0];

    // Создание процесса
    BOOL success =
        CreateProcessW(
            NULL,
            cmdLine,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi);

    if (success)
    {
        // Сохраняем PID процесса
        outPid = pi.dwProcessId;

        /**
         * @brief Закрываем ненужные handles
         *
         * Процесс продолжает работать,
         * даже после закрытия handles.
         */
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return true;
    }
    else
    {
        DWORD err = GetLastError();

        Logger::log(
            QString("CreateProcessW failed for %1, error %2")
            .arg(filePath)
            .arg(err));

        return false;
    }
}

/**
 * @struct EnumData
 * @brief Структура для передачи данных в EnumWindows
 *
 * Используется callback-функцией
 * при поиске главного окна процесса.
 */
struct EnumData
{
    DWORD pid;   /**< PID процесса */
    HWND hwnd;   /**< Найденное окно */
};

/**
 * @brief Callback-функция перечисления окон
 * @param hwnd Хэндл текущего окна
 * @param lParam Указатель на структуру EnumData
 * @return TRUE — продолжить поиск, FALSE — остановить
 *
 * Проверяет принадлежность окна
 * заданному процессу.
 */
static BOOL CALLBACK EnumWindowsProc(HWND hwnd,
    LPARAM lParam)
{
    EnumData* data =
        (EnumData*)lParam;

    DWORD processId;

    // Получаем PID владельца окна
    GetWindowThreadProcessId(hwnd, &processId);

    /**
     * @brief Проверяем:
     * - принадлежность процессу
     * - видимость окна
     */
    if (processId == data->pid &&
        IsWindowVisible(hwnd))
    {
        data->hwnd = hwnd;

        // Останавливаем перечисление
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Получение состояния процесса
 * @param pid PID процесса
 * @return Строковый статус процесса
 *
 * Возможные значения:
 * - Running
 * - Not Responding
 *
 * Проверка выполняется через:
 * SendMessageTimeout(WM_NULL)
 *
 * WM_NULL —
 * безопасное сообщение,
 * не изменяющее состояние окна.
 */
QString ProcessMonitor::getProcessStatus(int pid)
{
    EnumData data;

    data.pid = pid;
    data.hwnd = NULL;

    // Поиск главного окна процесса
    EnumWindows(
        EnumWindowsProc,
        (LPARAM)&data);

    /**
     * @brief Если окно отсутствует
     *
     * Консольные приложения
     * могут не иметь окон.
     */
    if (data.hwnd == NULL)
        return "Running";

    DWORD_PTR result;

    /**
     * @brief Проверка отзывчивости окна
     *
     * SMTO_ABORTIFHUNG —
     * прервать при зависании
     *
     * Таймаут: 5000 мс
     */
    LRESULT res =
        SendMessageTimeout(
            data.hwnd,
            WM_NULL,
            0,
            0,
            SMTO_ABORTIFHUNG,
            5000,
            &result);

    // Если ответа нет — приложение зависло
    return (res == 0)
        ? "Not Responding"
        : "Running";
}