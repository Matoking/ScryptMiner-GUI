#include "poolparse.h"

PoolParse::PoolParse(QObject *parent) :
    QObject(parent)
{
}

QString PoolParse::getURL(QString poolName, QString apiKey)
{
    QString url;

    if (poolName == "Litecoinpool.org")
        url = "https://www.litecoinpool.org/api?api_key=";
    else if (poolName == "OzCoin LTC Pool")
        url = "https://lc.ozcoin.net/api.php?api_key=";
    else if (poolName == "Elitist Jerks")
        url = "http://www.ejpool.info:8080/api?api_key=";

    url.append(apiKey);

    return url;
}

QString PoolParse::parseData(QString poolName, QVariantMap data)
{
    QString message;
    double totalRewards = -1;
    double estimatedRewards = -1;
    double paidRewards = -1;
    double unpaidRewards = -1;
    double past24hRewards = -1;
    double hashrate=-1;

    QVariantMap userMap;

    if (poolName == "Litecoinpool.org")
    {
        userMap = data.value("user").toMap();
        hashrate = userMap.value("hashrate").toReal();
        totalRewards = userMap.value("total_rewards").toReal();
        paidRewards = userMap.value("paid_rewards").toReal();
        unpaidRewards = userMap.value("unpaid_rewards").toReal();
        past24hRewards = userMap.value("past_24h_rewards").toReal();

    }
    else if (poolName == "OzCoin LTC Pool")
    {
        unpaidRewards = data.value("current_balance").toReal();
    }
    else if (poolName == "Elitist Jerks")
    {
        unpaidRewards = data.value("confirmed_rewards").toReal();
        paidRewards = data.value("payout_history").toReal();
        estimatedRewards = data.value("round_estimate").toReal();
    }

    if (totalRewards != -1)
    {
        QString totalString = QString("<b>Total rewards:</b><br/>%1 LTC<br/><br/>").arg(totalRewards);
        message.append(totalString);
    }
    if (hashrate != -1)
    {
        QString totalString = QString("<b>Hash Rate:</b><br/>%1 LTC<br/><br/>").arg(hashrate);
        message.append(totalString);
    }
    if (estimatedRewards != -1)
    {
        QString estString = QString("<b>Estimated rewards:</b><br/>%1 LTC<br/><br/>").arg(estimatedRewards);
        message.append(estString);
    }
    if (paidRewards != -1)
    {
        QString paidString = QString("<b>Paid rewards:</b><br/>%1 LTC<br/><br/>").arg(paidRewards);
        message.append(paidString);
    }
    if (unpaidRewards != -1)
    {
        QString unpaidString = QString("<b>Unpaid rewards:</b><br/>%1 LTC<br/><br/>").arg(unpaidRewards);
        message.append(unpaidString);
    }
    if (past24hRewards != -1)
    {
        QString past24hString = QString("<b>Rewards in past 24 hours:</b><br/>%1 LTC<br/><br/>").arg(past24hRewards);
        message.append(past24hString);
    }

    return message;
}
