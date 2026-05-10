#include "MainWindow.h"
#include "AppInfo.h"
#include "AddEditDialog.h"
#include "Logger.h"

#include <QTableWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QMessageBox>
#include <QContextMenuEvent>

// Конструктор главного окна приложения
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), monitorTimer(nullptr)
{
    // Инициализация пользовательского интерфейса
    setupUI();

    // Инициализация системы логирования
    Logger::init();

    // Загрузка конфигурации приложений из файла
    loadConfig();

    // Создание таймера мониторинга процессов
    monitorTimer = new QTimer(this);

    // Подключение обработчика периодической проверки процессов
    connect(monitorTimer, &QTimer::timeout,
        this, &MainWindow::onMonitorTimeout);

    // Запуск мониторинга каждые 2 секунды
    monitorTimer->start(2000);
}

// Деструктор главного окна
MainWindow::~MainWindow()
{
    // Сохранение конфигурации перед выходом
    saveConfig();

    // Завершение работы логгера
    Logger::close();

    // Освобождение памяти списка приложений
    qDeleteAll(apps);
}

// Настройка интерфейса приложения
void MainWindow::setupUI()
{
    setWindowTitle("Process Monitor");
    resize(800, 500);

    // Создание таблицы отображения приложений
    tableWidget = new QTableWidget(this);

    // Настройка количества колонок
    tableWidget->setColumnCount(5);

    // Заголовки таблицы
    tableWidget->setHorizontalHeaderLabels({
        "Application",
        "Arguments",
        "Delay (sec)",
        "Status",
        "PID"
        });

    // Последняя колонка растягивается автоматически
    tableWidget->horizontalHeader()->setStretchLastSection(true);

    // Включение контекстного меню
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // Подключение обработчика контекстного меню
    connect(tableWidget,
        &QTableWidget::customContextMenuRequested,
        this,
        &MainWindow::showContextMenu);

    // Таблица становится центральным виджетом окна
    setCentralWidget(tableWidget);

    // Создание действий меню и панели инструментов
    addAction = new QAction("Add", this);
    editAction = new QAction("Edit", this);
    deleteAction = new QAction("Delete", this);
    terminateAction = new QAction("Terminate", this);
    launchAction = new QAction("Launch", this);

    QAction* exitAction = new QAction("Exit", this);

    // Подключение обработчиков действий
    connect(addAction, &QAction::triggered,
        this, &MainWindow::onAddApp);

    connect(editAction, &QAction::triggered,
        this, &MainWindow::onEditApp);

    connect(deleteAction, &QAction::triggered,
        this, &MainWindow::onDeleteApp);

    connect(terminateAction, &QAction::triggered,
        this, &MainWindow::onTerminateApp);

    connect(launchAction, &QAction::triggered,
        this, &MainWindow::onLaunchApp);

    connect(exitAction, &QAction::triggered,
        this, &MainWindow::onExit);

    // Создание меню "File"
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(exitAction);

    // Создание меню "Edit"
    QMenu* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction(addAction);
    editMenu->addAction(editAction);
    editMenu->addAction(deleteAction);

    // Создание меню "Actions"
    QMenu* actionsMenu = menuBar()->addMenu("Actions");
    actionsMenu->addAction(terminateAction);
    actionsMenu->addAction(launchAction);

    // Создание панели инструментов
    QToolBar* toolbar = addToolBar("Main");

    toolbar->addAction(addAction);
    toolbar->addAction(editAction);
    toolbar->addAction(deleteAction);

    toolbar->addSeparator();

    toolbar->addAction(terminateAction);
    toolbar->addAction(launchAction);
}

// Получение текущей выбранной строки таблицы
int MainWindow::currentRow() const
{
    return tableWidget->currentRow();
}

// Загрузка конфигурации приложений из JSON-файла
void MainWindow::loadConfig()
{
    QFile file("apps.json");

    // Если файл не удалось открыть — выходим
    if (!file.open(QIODevice::ReadOnly))
        return;

    // Чтение содержимого файла
    QByteArray data = file.readAll();

    // Парсинг JSON-документа
    QJsonDocument doc = QJsonDocument::fromJson(data);

    // Проверка, что корень является массивом
    if (!doc.isArray())
        return;

    QJsonArray arr = doc.array();

    // Обработка всех записей приложений
    for (const QJsonValue& val : arr)
    {
        QJsonObject obj = val.toObject();

        QString path = obj["path"].toString();
        QString args = obj["arguments"].toString();
        int delay = obj["delay"].toInt();

        // Создание объекта приложения
        AppInfo* app = new AppInfo(path, args, delay);

        apps.append(app);

        // Обновление текущего статуса процесса
        app->updateStatus();

        // Планирование отложенного запуска
        scheduleDelayedLaunch(app);
    }

    // Обновление таблицы интерфейса
    updateTable();
}

// Сохранение конфигурации приложений в JSON-файл
void MainWindow::saveConfig()
{
    QJsonArray arr;

    // Формирование JSON-массива
    for (AppInfo* app : apps)
    {
        QJsonObject obj;

        obj["path"] = app->filePath;
        obj["arguments"] = app->arguments;
        obj["delay"] = app->delaySeconds;

        arr.append(obj);
    }

    // Создание JSON-документа
    QJsonDocument doc(arr);

    QFile file("apps.json");

    // Запись в файл
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
    }
}

// Настройка отложенного запуска приложения
void MainWindow::scheduleDelayedLaunch(AppInfo* app)
{
    // Если указана задержка и таймер еще не создан
    if (app->delaySeconds > 0 && !app->delayTimer)
    {
        app->delayTimer = new QTimer(this);

        // Однократный запуск таймера
        app->delayTimer->setSingleShot(true);

        // Лямбда-обработчик завершения таймера
        connect(app->delayTimer,
            &QTimer::timeout,
            this,
            [this, app]()
            {
                // Запуск приложения при необходимости
                if (app->pid == 0 && app->autoRestart)
                {
                    app->launch();
                }

                // Удаление таймера после срабатывания
                if (app->delayTimer)
                {
                    app->delayTimer->deleteLater();
                    app->delayTimer = nullptr;
                }

                updateTable();
            });

        // Старт таймера
        app->delayTimer->start(app->delaySeconds * 1000);
    }
    // Немедленный запуск без задержки
    else if (app->delaySeconds == 0 &&
        app->autoRestart &&
        app->pid == 0)
    {
        app->launch();
        updateTable();
    }
}

// Обновление содержимого таблицы
void MainWindow::updateTable()
{
    // Установка количества строк
    tableWidget->setRowCount(apps.size());

    for (int i = 0; i < apps.size(); ++i)
    {
        AppInfo* app = apps[i];

        // Заполнение ячеек таблицы
        tableWidget->setItem(i, 0,
            new QTableWidgetItem(app->displayName()));

        tableWidget->setItem(i, 1,
            new QTableWidgetItem(app->arguments));

        tableWidget->setItem(i, 2,
            new QTableWidgetItem(QString::number(app->delaySeconds)));

        tableWidget->setItem(i, 3,
            new QTableWidgetItem(app->status));

        tableWidget->setItem(i, 4,
            new QTableWidgetItem(
                app->pid == 0
                ? ""
                : QString::number(app->pid)));
    }

    // Автоматическая подстройка ширины колонок
    tableWidget->resizeColumnsToContents();
}

// Периодический мониторинг процессов
void MainWindow::onMonitorTimeout()
{
    for (AppInfo* app : apps)
    {
        // Обновление статуса процесса
        app->updateStatus();

        // Если процесс не запущен и включен автозапуск
        if (app->pid == 0 &&
            app->autoRestart &&
            (!app->delayTimer ||
                !app->delayTimer->isActive()))
        {
            app->launch();
        }
    }

    // Обновление интерфейса
    updateTable();
}

// Добавление нового приложения
void MainWindow::onAddApp()
{
    AddEditDialog dlg("Add Application");

    // Если пользователь подтвердил ввод
    if (dlg.exec() == QDialog::Accepted)
    {
        QString path = dlg.getFilePath();

        // Проверка корректности пути
        if (path.isEmpty())
        {
            QMessageBox::warning(
                this,
                "Error",
                "Program path cannot be empty.");

            return;
        }

        // Создание нового приложения
        AppInfo* app = new AppInfo(
            path,
            dlg.getArguments(),
            dlg.getDelaySeconds());

        apps.append(app);

        // Настройка запуска
        scheduleDelayedLaunch(app);

        updateTable();
        saveConfig();
    }
}

// Редактирование приложения
void MainWindow::onEditApp()
{
    int row = currentRow();

    // Проверка выбранной строки
    if (row < 0 || row >= apps.size())
        return;

    AppInfo* app = apps[row];

    // Открытие диалога редактирования
    AddEditDialog dlg(
        "Edit Application",
        app->filePath,
        app->arguments,
        app->delaySeconds);

    if (dlg.exec() == QDialog::Accepted)
    {
        QString newPath = dlg.getFilePath();

        // Проверка пути
        if (newPath.isEmpty())
        {
            QMessageBox::warning(
                this,
                "Error",
                "Program path cannot be empty.");

            return;
        }

        // Обновление параметров приложения
        app->filePath = newPath;
        app->arguments = dlg.getArguments();
        app->delaySeconds = dlg.getDelaySeconds();

        // Сброс старого таймера задержки
        if (app->delayTimer)
        {
            app->delayTimer->stop();
            app->delayTimer->deleteLater();
            app->delayTimer = nullptr;
        }

        // Предложение перезапуска приложения
        int answer = QMessageBox::question(
            this,
            "Restart",
            "Application is running. Restart with new settings?",
            QMessageBox::Yes | QMessageBox::No);

        // Перезапуск приложения
        if (answer == QMessageBox::Yes &&
            app->pid != 0)
        {
            app->terminate();

            // Повторное включение автозапуска
            app->autoRestart = true;

            app->launch();
        }
        // Если приложение не запущено
        else if (app->pid == 0)
        {
            scheduleDelayedLaunch(app);
        }

        updateTable();
        saveConfig();
    }
}

// Удаление приложения
void MainWindow::onDeleteApp()
{
    int row = currentRow();

    if (row < 0 || row >= apps.size())
        return;

    AppInfo* app = apps[row];

    // Если приложение запущено
    if (app->pid != 0)
    {
        int answer = QMessageBox::question(
            this,
            "Delete",
            "Application is running. Terminate and delete?",
            QMessageBox::Yes | QMessageBox::No);

        if (answer == QMessageBox::Yes)
        {
            app->terminate();
        }
        else
        {
            return;
        }
    }

    // Удаление объекта приложения
    delete app;

    apps.removeAt(row);

    updateTable();
    saveConfig();
}

// Завершение процесса приложения
void MainWindow::onTerminateApp()
{
    int row = currentRow();

    if (row < 0 || row >= apps.size())
        return;

    AppInfo* app = apps[row];

    // Если приложение запущено
    if (app->pid != 0)
    {
        app->terminate();
        updateTable();
    }
    else
    {
        QMessageBox::information(
            this,
            "Info",
            "Application is not running.");
    }
}

// Запуск приложения вручную
void MainWindow::onLaunchApp()
{
    int row = currentRow();

    if (row < 0 || row >= apps.size())
        return;

    AppInfo* app = apps[row];

    // Отмена активного таймера задержки
    if (app->delayTimer &&
        app->delayTimer->isActive())
    {
        app->delayTimer->stop();
        app->delayTimer->deleteLater();
        app->delayTimer = nullptr;
    }

    // Включение автоперезапуска
    app->autoRestart = true;

    // Запуск процесса
    app->launch();

    updateTable();
}

// Завершение работы приложения
void MainWindow::onExit()
{
    close();
}

// Отображение контекстного меню таблицы
void MainWindow::showContextMenu(const QPoint& pos)
{
    int row = tableWidget->currentRow();

    if (row < 0)
        return;

    // Создание контекстного меню
    QMenu menu;

    menu.addAction(editAction);
    menu.addAction(deleteAction);

    menu.addSeparator();

    menu.addAction(terminateAction);
    menu.addAction(launchAction);

    // Отображение меню в позиции курсора
    menu.exec(tableWidget->viewport()->mapToGlobal(pos));
}