#include "scorecalculator.h"
#include <cmath>
#include <QRegExp>
#include <QStringList>

ScoreCalculator::ScoreCalculator()
{
    ALPHA = 0.5;
    BETA = 0.3;
    GAMA = 0.2;
    FULL_SCORE = 5;
}

double ScoreCalculator::getScore(const QString &userName, const QString &fileID, QMap<QString, double> intimacyList, Rating *rating)
{
    if (rating->hasRating(userName, fileID)) {
        return rating->getUserRating(userName, fileID);
    } else {
        return getPredictedRating(userName, fileID, intimacyList, rating);
    }
}
double ScoreCalculator::getPredictedRating(const QString &userName, const QString &fileID, QMap<QString, double> intimacyList, Rating *rating)
{
    return ALPHA * getCredenceValue(userName, fileID, rating) + BETA * getFriendInfluence(fileID, intimacyList, rating)
                + GAMA * getPlisherInfluence(fileID, intimacyList);
}

double ScoreCalculator::getCredenceValue(const QString &userName, const QString &fileID, Rating *rating)
{
    QMap<QString, double> ratings = rating->getFileRating(fileID);
        double res = 0.0, dom = 0.0;
        //ratings.begin().value()
        for (QMap<QString, double>::iterator r = ratings.begin(); r != ratings.end(); r++) {
           double sim = getSimilarity(r.key(), userName, rating);
           //double imtimacy = intimacyList[r.key()];
           res += sim * r.value();
           dom += sim;
        }

       if (dom > 0) {
            qDebug() << "getCredenceValue" << res / dom ;
            return  res / dom;
       }
       return 0.0;


}

double ScoreCalculator::getFriendInfluence(const QString &fileID,  QMap<QString, double> intimacyList, Rating *rating)
{
    double res = 0.0;
    double dom = 0.0;
    qDebug() << "inrimacylist: " << intimacyList;
     for (QMap<QString, double>::iterator f = intimacyList.begin(); f != intimacyList.end(); f++) {
        if (f.value() > 0.0) {
            qDebug() << f.value() << "," << rating->getUserRating(f.key(), fileID);
            res += f.value() * rating->getUserRating(f.key(), fileID);
            dom += f.value();
        }
    }
    if (dom > 0){
     qDebug() << "getFriendInfluence" << res / dom;
    return res / dom;
    }
    return 0.0;
}

double ScoreCalculator::getSimilarity(const QString &u1,const  QString &u2, Rating *rating)
{
    QVector<QString> fileList = rating->getUser2Files(u1);
    double res = 0.0, d1 = 0.0, d2 = 0.0;
    for (auto f : fileList) {
        QMap<QString, double> ratings = rating->getFileRating(f);
        if (!ratings.contains(u2)) continue;
        res += ratings[u1] * ratings[u2];
        d1 += pow(ratings[u1], 2);
        d2 += pow(ratings[u2], 2);
    }
    d1 = sqrt(d1);
    d2 = sqrt(d2);
    if (d1 + d2 == 0.0) return 0.0;
    qDebug() << "getSimilarity " << res / (d1 * d2);
    return res / (d1 * d2);
}



// getFriendItimacy could be obtianed by requesting from the server. so, you can just replave the parameter friendRelation with other kind of pamameter you like.
double ScoreCalculator::getPlisherInfluence(const QString &fileID,  QMap<QString, double> intimacyList)
{
    QString publisher = fileID.split(QRegExp("[;]")).back();
    qDebug() << "getPlisherInfluence" << FULL_SCORE * intimacyList[publisher];
    return FULL_SCORE * intimacyList[publisher];

}
