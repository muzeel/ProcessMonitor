#include "AddEditDialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QPushButton>

AddEditDialog::AddEditDialog(const QString& title, const QString& path, const QString& args, int delay, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setMinimumWidth(400);

    pathEdit = new QLineEdit(path);
    argsEdit = new QLineEdit(args);
    delaySpin = new QSpinBox();
    delaySpin->setRange(0, 3600);
    delaySpin->setSuffix(" sec");
    delaySpin->setValue(delay);

    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select executable", QString(), "Executable (*.exe)");
        if (!file.isEmpty())
            pathEdit->setText(file);
        });

    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseButton);

    QFormLayout* layout = new QFormLayout(this);
    layout->addRow("Program path:", pathLayout);
    layout->addRow("Arguments:", argsEdit);
    layout->addRow("Delay on start (sec):", delaySpin);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons);
}