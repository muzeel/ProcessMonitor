#include "ProcessMonitor.h"
#include "Logger.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QDebug>
#include <QFileInfo>

#pragma comment(lib, "psapi.lib")

bool ProcessMonitor::isProcessRunning(const QString& filePath, int& outPid)
{
    QFileInfo fi(filePath);
    QString targetName = fi.fileName();  // "calc.exe"
    if (targetName.isEmpty()) return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            QString procName = QString::fromWCharArray(pe.szExeFile);
            if (procName.compare(targetName, Qt::CaseInsensitive) == 0) {
                outPid = pe.th32ProcessID;
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return false;
}

bool ProcessMonitor::terminateProcess(int pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        DWORD err = GetLastError();
        Logger::log(QString("OpenProcess failed for PID %1, error code %2").arg(pid).arg(err));
        return false;
    }
    BOOL result = TerminateProcess(hProcess, 0);
    DWORD err = GetLastError();
    CloseHandle(hProcess);
    if (result) {
        Logger::log(QString("TerminateProcess succeeded for PID %1").arg(pid));
        return true;
    }
    else {
        Logger::log(QString("TerminateProcess failed for PID %1, error code %2").arg(pid).arg(err));
        return false;
    }
}

bool ProcessMonitor::launchProcess(const QString& filePath, const QString& arguments, int& outPid)
{
    QString cmd = QString("\"%1\"").arg(filePath);
    if (!arguments.isEmpty())
        cmd += " " + arguments;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmdWide = cmd.toStdWString();
    wchar_t* cmdLine = &cmdWide[0];

    BOOL success = CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (success) {
        outPid = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    else {
        DWORD err = GetLastError();
        Logger::log(QString("CreateProcessW failed for %1, error %2").arg(filePath).arg(err));
        return false;
    }
}

struct EnumData {
    DWORD pid;
    HWND hwnd;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumData* data = (EnumData*)lParam;
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == data->pid && IsWindowVisible(hwnd)) {
        data->hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

QString ProcessMonitor::getProcessStatus(int pid)
{
    EnumData data;
    data.pid = pid;
    data.hwnd = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&data);

    if (data.hwnd == NULL)
        return "Running";

    DWORD_PTR result;
    LRESULT res = SendMessageTimeout(data.hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 5000, &result);
    return (res == 0) ? "Not Responding" : "Running";
}