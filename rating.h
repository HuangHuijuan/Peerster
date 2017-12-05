#ifndef RATING_H
#define RATING_H
#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QDataStream>

class Rating: public QObject
{
    Q_OBJECT
public:
    Rating();
    ~Rating();
    void storeLocal();
    bool hasRating(const QString& userName, const QString& fileID);
    double getUserRating(const QString& userName, const QString& fileID);
    bool addUserRating(const QString& fileID, const QString& userName, double score);
    QVector<QString> getUser2Files(const QString& username);
    QMap<QString, double> getFileRating(const QString& fileid);
private:
    QMap<QString, QMap<QString, double>> fileReputation;
    QMap<QString, QVector<QString>> userVotingList;
};

#endif // RATING_H
