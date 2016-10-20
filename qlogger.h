#ifndef QLOGGER_H
#define QLOGGER_H

#include <QObject>
#include <QPlainTextEdit>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

class QLogger : public QObject
{
 Q_OBJECT
public:
 explicit QLogger(QObject *parent, QString fileName, QPlainTextEdit *editor = 0);
 ~QLogger();
 void setShowDateTime(bool value);

private:
 QFile *file;
 QPlainTextEdit *m_editor;
 bool m_showDate;

signals:

public slots:
 void write(const QString &value);

};

#endif // LOGGER_H
