#include <QDateTime>
#include <QApplication>
#include <QListWidget>
#include "netsocket.h"
#include "peer.h"
#include "node.h"

Node::Node()
{
    // Create a UDP network socket
    if (!sock.bind())
        exit(1);

    //initialize the seqNo to be 1;
    seqNo = 1;

    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);
    userName = QHostInfo::localHostName() + "-" + QString::number(sock.getPort()) + "-" + QString::number(qrand() % 1000);

    aETimer.setInterval(10000);
    rrTimer.setInterval(60000);


    //initalize statusMsg
    statusMsg.insert("Want", QVariant(QVariantMap()));

    connect(&sock, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    if (forward) {
        aETimer.start();
        connect(&aETimer, SIGNAL(timeout()), this, SLOT(aESendStatusMsg()));
    }
    connect(&rrTimer, SIGNAL(timeout()), this, SLOT(sendRouteRumor()));

    signalMapper = new QSignalMapper(this);

    forward = true;

    initializeNeighbors();
}

void Node::initializeNeighbors()
{
    qDebug() << QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    QHostAddress myAddress = info.addresses().first();
    qDebug() << myAddress.toString() << info.hostName();

    for (int p = sock.getMyPortMin(); p <= sock.getMyPortMax(); p++) {
        if (p == sock.getPort()) {
            continue;
        }
        Peer *peer = new Peer(p, myAddress, info.hostName());
        peers.insert(peer->toString(), peer);
        //peersList->addItem(peer->toString());
    }

    //add host from command line to peer
    QStringList arguments = QCoreApplication::arguments();

    if (arguments.size() > 1) {
        int start = 1;
        if (arguments[1] == "-noforward") {
            forward = false;
            start = 2;
        }
        for (int i = start; i < arguments.size(); i++) {
            QStringList list = arguments[i].split(":");
            int id = QHostInfo::lookupHost(list[0], this, SLOT(lookedUp(QHostInfo)));
            lookUp.insert(QString::number(id), list[1].toInt());
        }
    }

    //start up
    sendRouteRumor();
}

Peer* Node::getNeighbor()
{
    qDebug() << peers.size();
    int r = qrand() % peers.size();
    QMap<QString,Peer*>::iterator i = peers.begin();
    Peer *p = (i + r).value();
    return p;
}

int Node::getNeighborSize()
{
    return peers.size();
}

QString& Node::getUserName() {
    return userName;
}

void Node::addPeer(const QString& host, quint16 port)
{
    int id = QHostInfo::lookupHost(host, this, SLOT(lookedUp(QHostInfo)));
    lookUp.insert(QString::number(id), port);
}

void Node::lookedUp(const QHostInfo &host)
{
    if (host.error() != QHostInfo::NoError) {
        qDebug() << "Lookup failed:" << host.errorString();
        return;
    }

    QString lookUpId = QString::number(host.lookupId());
    int port = lookUp[lookUpId];
    lookUp.remove(lookUpId);
    QHostAddress address = host.addresses().first();
    QString hn = host.hostName();
    Peer *p = new Peer(port, address, hn);
    QString s = p->toString();

    if (!peers.contains(s)) {
        peers.insert(s, p);
//        emit newPeer(s);
        qDebug() << "Add peer to peer list:" << hn << address.toString() << port;
    }
}

void Node::sendRouteRumor() {
    qDebug() << "sendRouteMsg";
    QVariantMap msg;
    msg.insert("Origin", userName);
    msg.insert("SeqNo", seqNo);
    QString msgId = userName + "_" + QString::number(seqNo);
    msgRepo.insert(msgId, msg);
    Peer *neighbor = getNeighbor();
    sendRumorMsg(neighbor->getAddress(), neighbor->getPort(), msgId);
    addStatus(userName, seqNo);
    seqNo++;
}

void Node::sendNewMsg(const QString& content)
{
    QVariantMap msg;
    msg.insert("ChatText", content);
    msg.insert("Origin", userName);
    msg.insert("SeqNo", seqNo);
    QString msgId = userName + "_" + QString::number(seqNo);
    msgRepo.insert(msgId, msg);
    Peer *neighbor = getNeighbor();
    sendRumorMsg(neighbor->getAddress(), neighbor->getPort(), msgId);
    addStatus(userName, seqNo);
    seqNo++;
}
void Node::sendP2PMsg(const QString& des, const QString& chatText)
{
    qDebug() << "send private msg " << chatText << " to " << des;
    QVariantMap msg;
    msg.insert("Dest", des);
    msg.insert("ChatText", chatText);
    msg.insert("Origin", userName);
    msg.insert("HopLimit", 10);

    QPair<QHostAddress, quint16> pair = routingTable[des];
    sendMsg(pair.first, pair.second, msg);
}

void Node::sendMsg(const QHostAddress& receiverIP , quint16 receiverPort, const QVariantMap& msg)
{
    QByteArray datagram;
    QDataStream out(&datagram, QIODevice::WriteOnly);
    out << msg;
    sock.writeDatagram(datagram, receiverIP, receiverPort);
}

void Node::sendRumorMsg(const QHostAddress& receiverIP , quint16 receiverPort, const QString& msgId)
{
    qDebug() << "send rumor message " << msgId << " to " << receiverIP << ":" <<receiverPort;

    sendMsg(receiverIP, receiverPort, msgRepo[msgId].toMap());

    if (!rumorTimers.contains(msgId)) {
        //wait for responding status massage, timer start
        QTimer* timer = new QTimer();
        timer->setInterval(2000);
        rumorTimers.insert(msgId, timer);
        connect(timer, SIGNAL(timeout()), signalMapper, SLOT(map()));
        signalMapper -> setMapping(timer, msgId);
        connect(signalMapper, SIGNAL(mapped(QString)),this, SLOT(continueRumormongering(QString)));
        timer->start();
    }
}


void Node::addStatus(const QString& origin, int n)
{
    QVariantMap map = statusMsg["Want"].toMap();
    map.insert(origin, QVariant(n + 1));
    statusMsg["Want"] = QVariant(map);
   // qDebug() << origin << qvariant_cast<QVariantMap>(statusMsg["Want"])[origin].toString();
}

void Node::sendStatusMsg(const QHostAddress& desAddr, quint16 desPort)
{
    qDebug() << "send status message to" << desAddr.toString() << ":" << desPort;
    sendMsg(desAddr, desPort, statusMsg);
}

void Node::aESendStatusMsg()
{
    qDebug() << "Anti-entropy starts";
    Peer *neighbor = getNeighbor();
    sendStatusMsg(neighbor->getAddress(), neighbor->getPort());
}
void Node::readPendingDatagrams()
{
    qDebug() << "RECEIVE MSG";
    while (sock.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(sock.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        sock.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (!peers.contains(sender.toString() + ":" + senderPort)) {
            int id = QHostInfo::lookupHost(sender.toString(), this, SLOT(lookedUp(QHostInfo)));
            lookUp.insert(QString::number(id), senderPort);
        }
        QDataStream in(&datagram, QIODevice::ReadOnly);
        QVariantMap msg;
        in >> msg;
        if (msg.contains("Dest")) {
            processPrivateMsg(msg);
        } else if (msg.contains("Origin")) {
            processRumorMsg(msg, sender, senderPort);
        } else if (msg.contains("Want")) {
            if (forward) {
                processStatusMsg(msg, sender, senderPort);
            }
        }
    }
}
void Node::processPrivateMsg(QVariantMap privateMsg)
{
    qDebug() << "PRIV: receive private msg";

    if (privateMsg["Dest"] != userName && privateMsg["HopLimit"].toInt() > 0) {
        QPair<QHostAddress, quint16> pair = routingTable[privateMsg["Dest"].toString()];
        privateMsg["HopLimit"] = privateMsg["HopLimit"].toInt() - 1;
        if (forward) {
            sendMsg(pair.first, pair.second, privateMsg);
        }
    } else if (privateMsg["Dest"] == userName) {
        emit newPrivateLog(privateMsg["Origin"].toString(), privateMsg["ChatText"].toString());
    }
}

void Node::processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, quint16 senderPort)
{
    if (rumorMsg.contains("LastIP")) {
        QString lastIP = rumorMsg["LastIP"].toString();
        QString lastPort = rumorMsg["lastPort"].toString();
        if (!peers.contains(lastIP + ":" + lastPort)) {
            int id = QHostInfo::lookupHost(lastIP, this, SLOT(lookedUp(QHostInfo)));
            lookUp.insert(QString::number(id), senderPort);
        }
    }
    rumorMsg.insert("LastIP", sender.toString());
    rumorMsg.insert("LastPort", senderPort);
    int n = rumorMsg["SeqNo"].toInt();
    QString origin = rumorMsg["Origin"].toString();
    QVariantMap map = statusMsg["Want"].toMap();

    //new message
    if ((!map.contains(origin) && n == 1)|| (map.contains(origin) && map[origin].toInt() == n)) {
        QString msgId = origin + "_" +QString::number(n);
        if (rumorMsg.contains("ChatText")) {
            QString senderName = sender.toString() + ":" + QString::number(senderPort);
            qDebug() << "RUMOR: receive chat rumor from " << senderName << ": " << rumorMsg["ChatText"].toString() << "--->" << origin << n;
            emit newLog(origin + ": " + rumorMsg["ChatText"].toString());
            if (forward) {
                //send the new rumor to its neighbour
                continueRumormongering(msgId);
            }
        } else { //route rumor message
            sendMsgToAllPeers(rumorMsg);
        }
        //add rumor msg to the map
        msgRepo.insert(msgId, QVariant(rumorMsg));

        addStatus(origin, n);

        if (!routingTable.contains(origin)) {
            emit newPeer(origin);
            qDebug() << "Add peer to routing table:" << origin;
        }
        routingTable.insert(origin, QPair<QHostAddress, quint16>(sender, senderPort));
        if (rumorMsg.contains("LastIP")) {
            ifDirectMsg.insert(origin, false);
        } else {
            ifDirectMsg.insert(origin, true);
        }
    } else if ((map.contains(origin) && map[origin].toInt() == n - 1)) {
        //receive same message multiple time
        if(!ifDirectMsg[origin] && !rumorMsg.contains("LastIP")) {
            routingTable.insert(origin, QPair<QHostAddress, quint16>(sender, senderPort));
        }
    }
    //send status msg to the sender
    sendStatusMsg(sender, senderPort);

}

void Node::continueRumormongering(const QString& msgId) {
    Peer *receiver = getNeighbor();
    sendRumorMsg(receiver->getAddress(), receiver->getPort(), msgId);
}

void Node::sendMsgToAllPeers(const QVariantMap& msg)
{
    for (Peer* p: peers.values()) {
        sendMsg(p->getAddress(), p->getPort(), msg);
    }
}

void Node::processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, quint16 senderPort)
{
    QString senderName = sender.toString() + ":" + QString::number(senderPort);
    qDebug() << "STATUS: receive status message from" << senderName;

    QVariantMap::Iterator itr;
    QVariantMap status = statusMsg["Want"].toMap();
    QVariantMap senderStatus = senderStatusMsg["Want"].toMap();
    //qDebug() << "My Status:";
    for (itr = status.begin(); itr != status.end(); itr++) {
        QString origin = itr.key();
     //  qDebug() << origin << status[origin].toInt();
        if (senderStatus.contains(origin)) {
            int senderSeqNo = senderStatus[origin].toInt();
            int statusSeqNo = status[origin].toInt();
            if (senderSeqNo > statusSeqNo) {
                //send a status msg to the sender
                sendStatusMsg(sender, senderPort);
                return;
            } else if (senderSeqNo < statusSeqNo){
                //send the corresponding rumor msg
                QString msgId = origin + "_" + QString::number(senderSeqNo);
                sendRumorMsg(sender, senderPort, msgId);
                return;
            }
        } else {
            QString msgId = origin + "_1";
            sendRumorMsg(sender, senderPort, msgId);
            return;
        }
    }
    //qDebug() << "Sender status:";
    for (itr = senderStatus.begin(); itr != senderStatus.end(); itr++) {
     //   qDebug() << itr.key() << senderStatus[itr.key()].toInt();
        QString origin = itr.key();
        int senderSeqNo = senderStatus[origin].toInt();
        QString msgId = origin + "_" + QString::number(senderSeqNo - 1);
        if (!status.contains(origin)) {
            sendStatusMsg(sender, senderPort);
            return;
        } else if (rumorTimers.contains(msgId)) {
            if (qrand() % 2) {
                qDebug() << "stop timer for msg" << msgId;
                //1, head, send rumor
                rumorTimers[msgId]->stop();
                rumorTimers.remove(msgId);
            }
        }
    }

}



