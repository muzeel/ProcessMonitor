/**
 * @file AddEditDialog.h
 * @brief Диалог добавления и редактирования приложений
 *
 * Определяет класс AddEditDialog — модальное окно,
 * используемое для ввода и изменения параметров запуска приложения.
 *
 * Диалог позволяет пользователю:
 * - выбрать исполняемый файл
 * - задать аргументы командной строки
 * - указать задержку запуска
 */

#ifndef ADDEDITDIALOG_H
#define ADDEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

 /**
  * @class AddEditDialog
  * @brief Диалог настройки параметров приложения
  *
  * Используется в двух режимах:
  * - добавление нового приложения
  * - редактирование существующего
  *
  * Содержит поля:
  * - путь к исполняемому файлу
  * - аргументы запуска
  * - задержка старта
  *
  * Наследуется от QDialog и работает как модальное окно.
  */
class AddEditDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief Конструктор диалога
     *
     * @param title Заголовок окна
     * @param path Начальный путь к исполняемому файлу
     * @param args Начальные аргументы командной строки
     * @param delay Начальная задержка запуска в секундах
     * @param parent Родительский виджет
     *
     * Создаёт форму ввода параметров приложения.
     */
    AddEditDialog(const QString& title,
        const QString& path = "",
        const QString& args = "",
        int delay = 0,
        QWidget* parent = nullptr);

    /**
     * @brief Получение пути к исполняемому файлу
     * @return Путь, введённый пользователем
     */
    QString getFilePath() const
    {
        return pathEdit->text();
    }

    /**
     * @brief Получение аргументов запуска
     * @return Строка аргументов командной строки
     */
    QString getArguments() const
    {
        return argsEdit->text();
    }

    /**
     * @brief Получение задержки запуска
     * @return Задержка в секундах
     */
    int getDelaySeconds() const
    {
        return delaySpin->value();
    }

private:

    /**
     * @brief Поле ввода пути к приложению
     */
    QLineEdit* pathEdit;

    /**
     * @brief Поле ввода аргументов запуска
     */
    QLineEdit* argsEdit;

    /**
     * @brief Поле выбора задержки запуска
     */
    QSpinBox* delaySpin;
};

#endif // ADDEDITDIALOG_H