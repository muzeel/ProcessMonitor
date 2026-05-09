#ifndef APPINFO_H
#define APPINFO_H

#include <QString>
#include <QTimer>

/**
 *  ласс, представл€ющий одно приложение из списка пользовател€.
 * ’ранит параметры запуска, состо€ние и управл€ет таймером отложенного старта.
 */
class AppInfo
{
public:
    AppInfo();
    AppInfo(const QString& filePath, const QString& arguments, int delaySeconds);
    ~AppInfo();

    QString filePath;       // полный путь к исполн€емому файлу
    QString arguments;      // аргументы командной строки
    int delaySeconds;       // задержка при первом запуске (сек)
    bool autoRestart;       // флаг, нужно ли перезапускать при аварийном завершении
    int pid;                // текущий PID (0 Ц не запущен)
    QString status;         // "Running", "Not Responding", "Stopped"

    QTimer* delayTimer;     // таймер дл€ отложенного запуска (только при старте монитора)

    void launch();          // запустить процесс
    void terminate();       // завершить процесс и отключить автоперезапуск
    void updateStatus();    // обновить статус (вызываетс€ монитором)
    QString displayName() const; // им€ файла без пути
};

#endif // APPINFO_H