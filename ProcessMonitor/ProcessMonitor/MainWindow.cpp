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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), monitorTimer(nullptr)
{
    setupUI();
    Logger::init();
    loadConfig();

    monitorTimer = new QTimer(this);
    connect(monitorTimer, &QTimer::timeout, this, &MainWindow::onMonitorTimeout);
    monitorTimer->start(2000); // проверка каждые 2 секунды
}

MainWindow::~MainWindow()
{
    saveConfig();
    Logger::close();
    qDeleteAll(apps);
}

void MainWindow::setupUI()
{
    setWindowTitle("Process Monitor");
    resize(800, 500);

    // “аблица
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(5);
    tableWidget->setHorizontalHeaderLabels({ "Application", "Arguments", "Delay (sec)", "Status", "PID" });
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableWidget, &QTableWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    setCentralWidget(tableWidget);

    // ƒействи€
    addAction = new QAction("Add", this);
    editAction = new QAction("Edit", this);
    deleteAction = new QAction("Delete", this);
    terminateAction = new QAction("Terminate", this);
    launchAction = new QAction("Launch", this);
    QAction* exitAction = new QAction("Exit", this);

    connect(addAction, &QAction::triggered, this, &MainWindow::onAddApp);
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditApp);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteApp);
    connect(terminateAction, &QAction::triggered, this, &MainWindow::onTerminateApp);
    connect(launchAction, &QAction::triggered, this, &MainWindow::onLaunchApp);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // ћеню
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(exitAction);

    QMenu* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction(addAction);
    editMenu->addAction(editAction);
    editMenu->addAction(deleteAction);

    QMenu* actionsMenu = menuBar()->addMenu("Actions");
    actionsMenu->addAction(terminateAction);
    actionsMenu->addAction(launchAction);

    // ѕанель инструментов
    QToolBar* toolbar = addToolBar("Main");
    toolbar->addAction(addAction);
    toolbar->addAction(editAction);
    toolbar->addAction(deleteAction);
    toolbar->addSeparator();
    toolbar->addAction(terminateAction);
    toolbar->addAction(launchAction);
}

int MainWindow::currentRow() const
{
    return tableWidget->currentRow();
}

void MainWindow::loadConfig()
{
    QFile file("apps.json");
    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return;

    QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        QJsonObject obj = val.toObject();
        QString path = obj["path"].toString();
        QString args = obj["arguments"].toString();
        int delay = obj["delay"].toInt();
        AppInfo* app = new AppInfo(path, args, delay);
        apps.append(app);
        // первоначальное обнаружение уже работающих процессов
        app->updateStatus();
        // отложенный запуск
        scheduleDelayedLaunch(app);
    }
    updateTable();
}

void MainWindow::saveConfig()
{
    QJsonArray arr;
    for (AppInfo* app : apps) {
        QJsonObject obj;
        obj["path"] = app->filePath;
        obj["arguments"] = app->arguments;
        obj["delay"] = app->delaySeconds;
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    QFile file("apps.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void MainWindow::scheduleDelayedLaunch(AppInfo* app)
{
    if (app->delaySeconds > 0 && !app->delayTimer) {
        app->delayTimer = new QTimer(this);
        app->delayTimer->setSingleShot(true);
        // захватываем app через [this, app] Ц будьте осторожны, если app удалитс€
        connect(app->delayTimer, &QTimer::timeout, this, [this, app]() {
            if (app->pid == 0 && app->autoRestart) {
                app->launch();
            }
            if (app->delayTimer) {
                app->delayTimer->deleteLater();
                app->delayTimer = nullptr;
            }
            updateTable();
            });
        app->delayTimer->start(app->delaySeconds * 1000);
    }
    else if (app->delaySeconds == 0 && app->autoRestart && app->pid == 0) {
        // запуск немедленно
        app->launch();
        updateTable();
    }
}

void MainWindow::updateTable()
{
    tableWidget->setRowCount(apps.size());
    for (int i = 0; i < apps.size(); ++i) {
        AppInfo* app = apps[i];
        tableWidget->setItem(i, 0, new QTableWidgetItem(app->displayName()));
        tableWidget->setItem(i, 1, new QTableWidgetItem(app->arguments));
        tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(app->delaySeconds)));
        tableWidget->setItem(i, 3, new QTableWidgetItem(app->status));
        tableWidget->setItem(i, 4, new QTableWidgetItem(app->pid == 0 ? "" : QString::number(app->pid)));
    }
    tableWidget->resizeColumnsToContents();
}

void MainWindow::onMonitorTimeout()
{
    for (AppInfo* app : apps) {
        app->updateStatus();

        // если процесс не запущен, разрешЄн автоперезапуск и нет активного отложенного таймера (первый запуск уже прошЄл) Ц запускаем
        if (app->pid == 0 && app->autoRestart && (app->delayTimer == nullptr || !app->delayTimer->isActive())) {
            app->launch();
        }
    }
    updateTable();
}

void MainWindow::onAddApp()
{
    AddEditDialog dlg("Add Application");
    if (dlg.exec() == QDialog::Accepted) {
        QString path = dlg.getFilePath();
        if (path.isEmpty()) {
            QMessageBox::warning(this, "Error", "Program path cannot be empty.");
            return;
        }
        AppInfo* app = new AppInfo(path, dlg.getArguments(), dlg.getDelaySeconds());
        apps.append(app);
        scheduleDelayedLaunch(app);
        updateTable();
        saveConfig();
    }
}

void MainWindow::onEditApp()
{
    int row = currentRow();
    if (row < 0 || row >= apps.size())
        return;
    AppInfo* app = apps[row];
    AddEditDialog dlg("Edit Application", app->filePath, app->arguments, app->delaySeconds);
    if (dlg.exec() == QDialog::Accepted) {
        QString newPath = dlg.getFilePath();
        if (newPath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Program path cannot be empty.");
            return;
        }
        // обновл€ем данные
        app->filePath = newPath;
        app->arguments = dlg.getArguments();
        app->delaySeconds = dlg.getDelaySeconds();
        // сбрасываем таймер отложенного запуска
        if (app->delayTimer) {
            app->delayTimer->stop();
            app->delayTimer->deleteLater();
            app->delayTimer = nullptr;
        }
        // возможно, процесс уже запущен Ц предлагаем перезапустить?
        int answer = QMessageBox::question(this, "Restart", "Application is running. Restart with new settings?",
            QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::Yes && app->pid != 0) {
            app->terminate();  // завершаем, autoRestart станет false
            // после завершени€ запустим снова
            app->autoRestart = true; // разрешаем перезапуск
            app->launch();
        }
        else if (app->pid == 0) {
            // не запущено Ц пересоздаЄм отложенный запуск
            scheduleDelayedLaunch(app);
        }
        updateTable();
        saveConfig();
    }
}

void MainWindow::onDeleteApp()
{
    int row = currentRow();
    if (row < 0 || row >= apps.size())
        return;
    AppInfo* app = apps[row];
    if (app->pid != 0) {
        int answer = QMessageBox::question(this, "Delete", "Application is running. Terminate and delete?",
            QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            app->terminate();
        }
        else {
            return;
        }
    }
    delete app;
    apps.removeAt(row);
    updateTable();
    saveConfig();
}

void MainWindow::onTerminateApp()
{
    int row = currentRow();
    if (row < 0 || row >= apps.size())
        return;
    AppInfo* app = apps[row];
    if (app->pid != 0) {
        app->terminate();
        updateTable();
    }
    else {
        QMessageBox::information(this, "Info", "Application is not running.");
    }
}

void MainWindow::onLaunchApp()
{
    int row = currentRow();
    if (row < 0 || row >= apps.size())
        return;
    AppInfo* app = apps[row];
    // отмен€ем отложенный запуск, если он ещЄ не сработал
    if (app->delayTimer && app->delayTimer->isActive()) {
        app->delayTimer->stop();
        app->delayTimer->deleteLater();
        app->delayTimer = nullptr;
    }
    app->autoRestart = true;
    app->launch();
    updateTable();
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    int row = tableWidget->currentRow();
    if (row < 0)
        return;
    QMenu menu;
    menu.addAction(editAction);
    menu.addAction(deleteAction);
    menu.addSeparator();
    menu.addAction(terminateAction);
    menu.addAction(launchAction);
    menu.exec(tableWidget->viewport()->mapToGlobal(pos));
}