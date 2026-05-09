#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QTimer>

class AppInfo;
class QTableWidget;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onMonitorTimeout();
    void onAddApp();
    void onEditApp();
    void onDeleteApp();
    void onTerminateApp();
    void onLaunchApp();
    void onExit();
    void showContextMenu(const QPoint& pos);

private:
    void setupUI();
    void loadConfig();
    void saveConfig();
    void updateTable();
    void scheduleDelayedLaunch(AppInfo* app);
    int currentRow() const;

    QTableWidget* tableWidget;
    QList<AppInfo*> apps;
    QTimer* monitorTimer;

    QAction* addAction;
    QAction* editAction;
    QAction* deleteAction;
    QAction* terminateAction;
    QAction* launchAction;
};

#endif // MAINWINDOW_H