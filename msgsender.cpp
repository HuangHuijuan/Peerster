#include "msgsender.h"

void MsgSender::sendIntimacyReq(const QString& file, const QStringList& voterAndPublisher)
{
    QVariantMap myMap;
    myMap.insert("FileID",file);
    myMap.insert("PeerList", voterAndPublisher);
    myMap.insert("UserName", userName);

    //send request to server
    sendToServer(myMap);
}

void MsgSender::sendAddFriendMsg(const QString& friendName)
{
    QVariantMap myMap;
    myMap.insert("UserName", userName);
    myMap.insert("FriendName",friendName);

    //send request to server
    sendToServer(myMap);
}

void MsgSender::sendFileRating(const QString& file, const QString& publisher, double score)
{
    QVariantMap myMap;
    myMap.insert("FileID", file);
    myMap.insert("Publisher", publisher);
    myMap.insert("UserName", userName);
    myMap.insert("Score", score);

    //send request to server
    sendToServer(myMap);
}

void MsgSender::sendToServer(const QVariantMap& mp){
    QByteArray msg;
    QDataStream outStream(&msg, QIODevice::WriteOnly);
    outStream << mp;
    sock->writeDatagram(msg.data(), msg.size(), address, port);
}
