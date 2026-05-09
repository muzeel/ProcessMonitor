#ifndef ADDEDITDIALOG_H
#define ADDEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class AddEditDialog : public QDialog
{
    Q_OBJECT
public:
    AddEditDialog(const QString& title, const QString& path = "", const QString& args = "", int delay = 0, QWidget* parent = nullptr);

    QString getFilePath() const { return pathEdit->text(); }
    QString getArguments() const { return argsEdit->text(); }
    int getDelaySeconds() const { return delaySpin->value(); }

private:
    QLineEdit* pathEdit;
    QLineEdit* argsEdit;
    QSpinBox* delaySpin;
};

#endif // ADDEDITDIALOG_H