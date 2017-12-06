#ifndef CREDENCECALCULATOR_H
#define CREDENCECALCULATOR_H
#include "rating.h"
#include <QMap>

class ScoreCalculator
{
public:
    ScoreCalculator();
    double getScore(const QString &userName, const QString &fileID, QMap<QString, double> intimacyList, Rating *rating);
    double getPredictedRating(const QString &userName, const QString &fileID, QMap<QString, double> intimacyList, Rating *rating);
    double getCredenceValue(const QString &userName, const QString &fileID, Rating *rating);
    double getSimilarity(const QString &u1,const  QString &u2, Rating *rating);
    double getPlisherInfluence(const QString &fileID,  QMap<QString, double> intimacyList);
    double getFriendInfluence(const QString &fileID,  QMap<QString, double> intimacyList, Rating *rating);
private:
    double ALPHA;
    double BETA;
    double GAMA;
    double FULL_SCORE;
};

#endif // CREDENCECALCULATOR_H
