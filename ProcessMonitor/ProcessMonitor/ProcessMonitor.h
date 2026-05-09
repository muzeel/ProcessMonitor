#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ProcessMonitor.h"

class ProcessMonitor : public QMainWindow
{
    Q_OBJECT

public:
    ProcessMonitor(QWidget *parent = nullptr);
    ~ProcessMonitor();

private:
    Ui::ProcessMonitorClass ui;
};

