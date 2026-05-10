#include "AddEditDialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QPushButton>

/**
 * @brief Конструктор диалога добавления/редактирования приложения
 *
 * Создаёт окно для ввода параметров запуска приложения:
 * - путь к исполняемому файлу
 * - аргументы командной строки
 * - задержка запуска
 *
 * @param title Заголовок окна
 * @param path Начальный путь к исполняемому файлу
 * @param args Начальные аргументы запуска
 * @param delay Начальная задержка запуска в секундах
 * @param parent Родительский виджет
 */
AddEditDialog::AddEditDialog(const QString& title,
    const QString& path,
    const QString& args,
    int delay,
    QWidget* parent)
    : QDialog(parent)
{
    // Установка заголовка окна
    setWindowTitle(title);

    // Минимальная ширина диалога
    setMinimumWidth(400);

    // Поле ввода пути к программе
    pathEdit = new QLineEdit(path);

    // Поле ввода аргументов командной строки
    argsEdit = new QLineEdit(args);

    // Поле выбора задержки запуска
    delaySpin = new QSpinBox();

    // Допустимый диапазон задержки: от 0 до 3600 секунд
    delaySpin->setRange(0, 3600);

    // Отображение единицы измерения
    delaySpin->setSuffix(" sec");

    // Установка начального значения
    delaySpin->setValue(delay);

    // Кнопка выбора исполняемого файла
    QPushButton* browseButton =
        new QPushButton("Browse...");

    // Обработчик выбора файла
    connect(browseButton,
        &QPushButton::clicked,
        [this]()
        {
            // Открытие стандартного диалога выбора файла
            QString file = QFileDialog::getOpenFileName(
                this,
                "Select executable",
                QString(),
                "Executable (*.exe)");

            // Если файл выбран — записываем путь
            if (!file.isEmpty())
                pathEdit->setText(file);
        });

    // Горизонтальный layout для поля пути и кнопки
    QHBoxLayout* pathLayout = new QHBoxLayout();

    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseButton);

    // Основной layout формы
    QFormLayout* layout = new QFormLayout(this);

    // Добавление строк формы
    layout->addRow("Program path:", pathLayout);
    layout->addRow("Arguments:", argsEdit);
    layout->addRow("Delay on start (sec):", delaySpin);

    // Кнопки подтверждения и отмены
    QDialogButtonBox* buttons =
        new QDialogButtonBox(
            QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel);

    // Подключение кнопки OK
    connect(buttons,
        &QDialogButtonBox::accepted,
        this,
        &QDialog::accept);

    // Подключение кнопки Cancel
    connect(buttons,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject);

    // Добавление кнопок в форму
    layout->addRow(buttons);
}