#ifndef MSGSENDER_H
#define MSGSENDER_H

#include <QString>
#include <QHostAddress>
#include <QVariant>
#include "netsocket.h"

class MsgSender : public QObject {

    Q_OBJECT

public:
    MsgSender(const QHostAddress& addr, quint16 p, const QString& name, NetSocket* sock)
        :address(addr),port(p),userName(name), sock(sock){}
    //send file name to server
    void sendIntimacyReq(const QString& file, const QStringList& voterAndPublisher);
    //send friend name to server
    void sendAddFriendMsg(const QString& friendName);
    //send the score of the file to server
    void sendFileRating(const QString& file, const QString& publisher, double score);
    void sendToServer(const QVariantMap& mp);

private:
    QHostAddress address;
    quint16 port;
    QString userName;
    NetSocket* sock;
};

#endif // MSGSENDER_H
