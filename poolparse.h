#ifndef POOLPARSE_H
#define POOLPARSE_H

#include <QObject>
#include <QString>
#include <QVariantMap>

class PoolParse : public QObject
{
    Q_OBJECT
public:
    explicit PoolParse(QObject *parent = 0);

signals:

public slots:
    static QString getURL(QString poolName, QString apiKey);
    static QString parseData(QString poolName, QVariantMap data);

};

#endif // POOLPARSE_H
