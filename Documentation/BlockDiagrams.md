# Блок-схемы алгоритмов ProcessMonitor

---

## 1. Алгоритм мониторинга процессов (Главный цикл)

```
 НАЧАЛО
    │
    ├── Инициализация Logger
    ├── Загрузка apps.json
    │
    ├─► ЦИКЛ (бесконечно, каждые 2 сек)
    │       │
    │       ├─► Для каждого приложения apps[i]:
    │       │       │
    │       │       ├── Вызов isProcessRunning()
    │       │       │       │
    │       │       │       ├── Создать снапшот процессов
    │       │       │       ├── ЦИКЛ по процессам
    │       │       │       │       ├── Сравнить имя процесса
    │       │       │       │       ├── Открыть процесс
    │       │       │       │       ├── Получить полный путь
    │       │       │       │       └── Если путь совпадает → вернуть PID
    │       │       │       └── КОНЕЦ ЦИКЛА
    │       │       │
    │       │       ├── Если процесс найден:
    │       │       │       ├── Обновить PID (если изменился)
    │       │       │       └── Вызов getProcessStatus()
    │       │       │               ├── Найти окно процесса
    │       │       │               ├── SendMessageTimeout(WM_NULL)
    │       │       │               └── "Running" или "Not Responding"
    │       │       │
    │       │       └── Если процесс НЕ найден и pid != 0:
    │       │               ├── Процесс завершился
    │       │               ├── pid = 0
    │       │               └── status = "Stopped"
    │       │
    │       ├── Если pid == 0 И autoRestart == true
    │       │   И таймер задержки НЕ активен:
    │       │       └── Вызов launch()
    │       │               ├── CreateProcessW()
    │       │               ├── Сохранить PID
    │       │               └── status = "Running"
    │       │
    │       └── Обновить таблицу UI
    │
    └── При закрытии: Сохранить apps.json
```

---

## 2. Алгоритм запуска процесса (launch)

```
 НАЧАЛО (launch)
    │
    ├── Формирование командной строки
    │       └── "C:\path\app.exe" [arguments]
    │
    ├── Заполнение STARTUPINFOW (нули)
    │       └── si.cb = sizeof(si)
    │
    ├── Вызов CreateProcessW()
    │       ├── lpApplicationName = NULL
    │       ├── lpCommandLine = сформированная строка
    │       └── bInheritHandles = FALSE
    │
    ├── Если успех:
    │       ├── outPid = pi.dwProcessId
    │       ├── CloseHandle(pi.hProcess)
    │       ├── CloseHandle(pi.hThread)
    │       ├── status = "Running"
    │       └── Запись в лог
    │
    └── Если неудача:
            ├── GetLastError()
            ├── Запись в лог
            └── pid = 0, status = "Stopped"

 КОНЕЦ
```

---

## 3. Алгоритм завершения процесса (terminate)

```
 НАЧАЛО (terminate)
    │
    ├── Проверка: pid != 0 ?
    │       └── Нет → выход (ничего не делать)
    │
    ├── OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION)
    │
    ├── Если hProcess == NULL:
    │       ├── GetLastError()
    │       ├── Логирование ошибки
    │       └── Возврат false
    │
    ├── GetExitCodeProcess(hProcess)
    │
    ├── Если exitCode != STILL_ACTIVE:
    │       ├── CloseHandle
    │       └── Возврат true (уже завершён)
    │
    ├── TerminateProcess(hProcess, 1)
    │
    ├── CloseHandle
    │
    ├── Если успех:
    │       ├── pid = 0
    │       ├── status = "Stopped"
    │       ├── autoRestart = false  ← отключаем автоперезапуск
    │       └── Запись в лог
    │
    └── Если неудача:
            └── Запись в лог ошибки

 КОНЕЦ
```

---

## 4. Алгоритм поиска процесса (isProcessRunning)

```
 НАЧАЛО (isProcessRunning)
    │
    ├── QFileInfo(filePath)
    │       └── targetName = имя_файла.exe (нижний регистр)
    │
    ├── Если targetName пуст → возврат false
    │
    ├── CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    │       └── Если ошибка → возврат false
    │
    ├── Process32FirstW() - первый процесс
    │
    ├─► ЦИКЛ пока есть процессы:
    │       │
    │       ├── procName = pe.szExeFile (нижний регистр)
    │       │
    │       ├── Если procName == targetName:
    │       │       │
    │       │       ├── OpenProcess(PROCESS_QUERY_INFORMATION |
    │       │       │                 PROCESS_VM_READ)
    │       │       │
    │       │       ├── GetModuleFileNameExW(hProcess, ...)
    │       │       │
    │       │       ├── Если полный путь содержит targetName:
    │       │       │       ├── foundPid = pe.th32ProcessID
    │       │       │       ├── CloseHandle
    │       │       │       └── ВЫХОД из цикла (break)
    │       │       │
    │       │       └── CloseHandle
    │       │
    │       └── Process32NextW() - следующий процесс
    │
    ├── CloseHandle(hSnapshot)
    │
    ├── Если foundPid != 0:
    │       ├── outPid = foundPid
    │       └── Возврат true
    │
    └── Возврат false

 КОНЕЦ
```

---

## 5. Алгоритм отложенного запуска (scheduleDelayedLaunch)

```
 НАЧАЛО (scheduleDelayedLaunch)
    │
    ├── Проверка: delaySeconds > 0 ?
    │       │
    │       └── ДА:
    │               ├── Создать QTimer (singleShot = true)
    │               ├── Подключить timeout → лямбда:
    │               │       ├── Если pid == 0 И autoRestart:
    │               │       │       └── app->launch()
    │               │       ├── Удалить таймер
    │               │       └── updateTable()
    │               │
    │               └── delayTimer->start(delaySeconds * 1000)
    │
    ├── Иначе (delaySeconds == 0):
    │       ├── Если autoRestart == true И pid == 0:
    │       │       └── app->launch()
    │       └── updateTable()

 КОНЕЦ
```

---

## 6. Алгоритм сохранения/загрузки конфигурации

```
=== ЗАГРУЗКА (loadConfig) ===

 НАЧАЛО
    │
    ├── Открыть файл "apps.json" для чтения
    │
    ├── Если файл не открыт → выход
    │
    ├── QJsonDocument::fromJson(data)
    │
    ├── Если НЕ массив → выход
    │
    ├─► ЦИКЛ по элементам массива:
    │       ├── Чтение: path, arguments, delay
    │       ├── Создание AppInfo(path, args, delay)
    │       ├── apps.append(app)
    │       ├── app->updateStatus()
    │       └── scheduleDelayedLaunch(app)
    │
    └── updateTable()

 КОНЕЦ


=== СОХРАНЕНИЕ (saveConfig) ===

 НАЧАЛО
    │
    ├── QJsonArray arr
    │
    ├─► ЦИКЛ по apps:
    │       ├── QJsonObject obj
    │       ├── obj["path"] = app->filePath
    │       ├── obj["arguments"] = app->arguments
    │       ├── obj["delay"] = app->delaySeconds
    │       └── arr.append(obj)
    │
    ├── QJsonDocument doc(arr)
    │
    ├── Открыть "apps.json" для записи
    │
    └── file.write(doc.toJson())

 КОНЕЦ
```

---

## 7. Алгоритм добавления приложения (onAddApp)

```
 НАЧАЛО (onAddApp)
    │
    ├── Создать AddEditDialog("Add Application")
    │
    ├── dialog.exec()
    │
    ├── Если Accepted:
    │       ├── path = dialog.getFilePath()
    │       │
    │       ├── Если path пуст:
    │       │       └── QMessageBox::warning → выход
    │       │
    │       ├── args = dialog.getArguments()
    │       ├── delay = dialog.getDelaySeconds()
    │       │
    │       ├── Создание: AppInfo* app = new AppInfo(path, args, delay)
    │       │
    │       ├── apps.append(app)
    │       │
    │       ├── scheduleDelayedLaunch(app)
    │       │
    │       ├── updateTable()
    │       │
    │       └── saveConfig()
    │
    └── КОНЕЦ
```

---

## 8. Алгоритм завершения приложения пользователем

```
 НАЧАЛО (onTerminateApp)
    │
    ├── row = currentRow()
    │
    ├── Если row < 0 → выход
    │
    ├── app = apps[row]
    │
    ├── Проверка: app->pid != 0 ?
    │       │
    │       ├── ДА:
    │       │       ├── app->terminate()
    │       │       │       ├── OpenProcess
    │       │       │       ├── TerminateProcess
    │       │       │       ├── pid = 0
    │       │       │       ├── autoRestart = false
    │       │       │       └── status = "Stopped"
    │       │       │
    │       │       └── updateTable()
    │       │
    │       └── НЕТ (pid == 0):
    │               └── QMessageBox::information("Не запущен")

 КОНЕЦ
```

---

## 9. Алгоритм определения статуса процесса (getProcessStatus)

```
 НАЧАЛО (getProcessStatus)
    │
    ├── EnumData data { pid, hwnd = NULL }
    │
    ├── EnumWindows(EnumWindowsProc, &data)
    │       │
    │       └── EnumWindowsProc:
    │               ├── GetWindowThreadProcessId(hwnd)
    │               ├── Если processId == pid И IsWindowVisible:
    │               │       ├── data.hwnd = hwnd
    │               │       └── return FALSE (остановить перечисление)
    │               └── return TRUE (продолжить)
    │
    ├── Если hwnd == NULL:
    │       └── Возврат "Running" (нет окна, но процесс работает)
    │
    ├── SendMessageTimeout(hwnd, WM_NULL, 0, 0,
    │                      SMTO_ABORTIFHUNG, 5000, &result)
    │
    ├── Если res == 0 (таймаут):
    │       └── Возврат "Not Responding"
    │
    └── Возврат "Running"

 КОНЕЦ
```

---

## 10. Жизненный цикл приложения в мониторе

```
Создание AppInfo
       │
       ▼
┌──────────────────────┐
│  Конструктор         │
│  - filePath, args    │
│  - delaySeconds      │
│  - autoRestart = true│
│  - pid = 0           │
│  - status = ""       │
└──────────────────────┘
       │
       ▼
┌──────────────────────┐
│  scheduleDelayedLaunch│
└──────────────────────┘
       │
       ├──► delay > 0? ──ДАТА──► Запуск таймера
       │                              │
       │                         По таймауту:
       │                              │
       │    NO ◄──────────────────────┘
       │      │
       ▼      ▼
┌──────────────────────┐
│  launch()            │
│  CreateProcessW()    │
└──────────────────────┘
       │
       ▼
┌──────────────────────┐
│  РАБОТАЕТ            │
│  - pid != 0          │
│  - status = "Running"│
│  - Мониторинг: 2 сек │
└──────────────────────┘
       │
       ├──► Пользователь нажал Terminate:
       │       │
       │       ▼
       │   ┌──────────────────────┐
       │   │  terminate()         │
       │   │  - TerminateProcess()│
       │   │  - autoRestart=false │
       │   │  - pid = 0           │
       │   │  - status = "Stopped"│
       │   └──────────────────────┘
       │           │
       │           ▼
       │      КОНЕЦ (не перезапускается)
       │
       ├──► Процесс завершился сам:
       │       │
       │       ▼
       │   ┌──────────────────────┐
       │   │  updateStatus()       │
       │   │  - pid = 0           │
       │   │  - status = "Stopped"│
       │   └──────────────────────┘
       │           │
       │           ▼
       │      Проверка в onMonitorTimeout:
       │           │
       │           ├──► autoRestart == true?
       │           │       │
       │           │      ДА──► launch() ──► РАБОТАЕТ
       │           │               (перезапуск)
       │           │
       │          НЕТ──► КОНЕЦ (остановлен)
       │
       └──► Процесс завис:
               │
               ▼
           status = "Not Responding"
               │
               ▼
         Возможен Terminate или
         автоматическое определение
```