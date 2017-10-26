#include <QDateTime>
#include <QApplication>
#include <QListWidget>
#include <QFile>
#include <QCryptographicHash>
#include "netsocket.h"
#include "peer.h"
#include "node.h"
#include "file.h"

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
    aETimer.start();
    connect(&aETimer, SIGNAL(timeout()), this, SLOT(aESendStatusMsg()));
    rrTimer.start();
    connect(&rrTimer, SIGNAL(timeout()), this, SLOT(sendRouteRumor()));

    signalMapper = new QSignalMapper(this);

    forward = true;

    initializeNeighbors();
}

void Node::initializeNeighbors()
{
    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    myAddress = info.addresses().first();

    qDebug() << myAddress.toIPv4Address() << myAddress.toString() << info.hostName();

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
            QHostInfo peerinfo = QHostInfo::fromName(list[0]);
            QHostAddress address = peerinfo.addresses().first();
            Peer *peer = new Peer(list[1].toInt(), address, peerinfo.hostName());
            peers.insert(peer->toString(), peer);
            qDebug() << "Add peer to peer list";
//            int id = QHostInfo::lookupHost(list[0], this, SLOT(lookedUp(QHostInfo)));
//            lookUp.insert(QString::number(id), list[1].toInt());
        }
    }

    //start up
    sendRouteRumor();
}

Peer* Node::getNeighbor()
{
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
//    Peer *neighbor = getNeighbor();
//    sendRumorMsg(neighbor->getAddress(), neighbor->getPort(), msgId);
    sendMsgToAllPeers(msg);
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

    QVariantMap msg;
    msg.insert("Dest", des);
    msg.insert("ChatText", chatText);
    msg.insert("Origin", userName);
    msg.insert("HopLimit", 10);

    QPair<QHostAddress, quint16> pair = routingTable[des];
    sendMsg(pair.first, pair.second, msg);
    qDebug() << "send private msg " << chatText << " to " << pair.first;
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
    QVariantMap msg = msgRepo[msgId].toMap();
    if (forward || !msg.contains("ChatText")) {
     //   qDebug() << "send rumor message " << msgId << " to " << receiverIP << ":" <<receiverPort;
        sendMsg(receiverIP, receiverPort, msg);
    }

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
  //  qDebug() << "send status message to" << desAddr.toString() << ":" << desPort;
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
     //   qDebug() << msg;
        if (msg.contains("Dest")) {
            processPrivateMsg(msg);
        } else if (msg.contains("Origin")) {
            processRumorMsg(msg, sender, senderPort);
        } else if (msg.contains("Want")) {
            processStatusMsg(msg, sender, senderPort);
        }
    }
}
void Node::processPrivateMsg(QVariantMap privateMsg)
{
    qDebug() << "PRIV: receive private msg";

    if (privateMsg["Dest"] != userName && privateMsg["HopLimit"].toInt() > 0 && forward) {
        QPair<QHostAddress, quint16> pair = routingTable[privateMsg["Dest"].toString()];
        privateMsg["HopLimit"] = privateMsg["HopLimit"].toInt() - 1;
        sendMsg(pair.first, pair.second, privateMsg);
    } else if (privateMsg["Dest"] == userName) {
        emit newPrivateLog(privateMsg["Origin"].toString(), privateMsg["ChatText"].toString());
    }
}

void Node::processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, quint16 senderPort)
{
    int n = rumorMsg["SeqNo"].toInt();
    QString origin = rumorMsg["Origin"].toString();
    QString msgId = origin + "_" + QString::number(n);
    QVariantMap map = statusMsg["Want"].toMap();

    if (rumorMsg.contains("LastIP")) { 
        quint32 lastIP = rumorMsg["LastIP"].toInt();
        QString address= QHostAddress(lastIP).toString();
        QString lastPort = rumorMsg["lastPort"].toString();
        QString lastId = address + ":" + lastPort;
        if (!peers.contains(lastId) && address != myAddress.toString()) {
            int id = QHostInfo::lookupHost(address, this, SLOT(lookedUp(QHostInfo)));
            lookUp.insert(QString::number(id), senderPort);
        }
    }

    //new message

    if ((!map.contains(origin) && n == 1)|| (map.contains(origin) && map[origin].toInt() == n)) {
        QHostAddress sourceIP = sender;
        quint16 sourcePort = senderPort;
        if(rumorMsg.count("LastIP") && rumorMsg.count("LastPort")){
            sourceIP = QHostAddress(rumorMsg["LastIP"].toInt());
            sourcePort = (quint16)rumorMsg["LastPort"].toInt();
        }
        rumorMsg.insert("LastIP", sender.toIPv4Address());
        rumorMsg.insert("LastPort", senderPort);

        if (rumorMsg.contains("ChatText")) {
            qDebug() << "CHAT RUMOR: receive chat rumor from " << origin << ": " << rumorMsg["ChatText"].toString();
            emit newLog(origin + ": " + rumorMsg["ChatText"].toString());

                //send the new rumor to its neighbour
            continueRumormongering(msgId);

        } else { //route rumor message
            qDebug() << "ROUTE RUMOR: receive route rumor from " << sender.toString() << ": " << msgId;
            sendMsgToAllPeers(rumorMsg);
        }
        //add rumor msg to the map
        msgRepo.insert(msgId, QVariant(rumorMsg));

        addStatus(origin, n);

        if (!routingTable.contains(origin)) {
            emit newPeer(origin);
            qDebug() << "Add peer to routing table:" << origin;
            sendRouteRumor();
        }
        routingTable.insert (origin, QPair<QHostAddress, quint16>(sourceIP, sourcePort));
        qDebug() << routingTable;
    //    qDebug() << "routingTable insert" << origin << " " << sender;
    } else if (map.contains(origin) && map[origin].toInt() - 1 == n) {
         if(!rumorMsg.contains("LastIP")) {
             qDebug() << "receive direct msg from" << sender.toString() << ": " << msgId;;
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
    qDebug() << "send route msg to all peers";
    for (Peer* p: peers.values()) {
        sendMsg(p->getAddress(), p->getPort(), msg);
    }
}

void Node::processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, quint16 senderPort)
{
    QString senderName = sender.toString() + ":" + QString::number(senderPort);
//    qDebug() << "STATUS: receive status message from" << senderName;

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

void Node::processBlockRequest(QVariantMap request)
{
    qDebug() << "BLOCK REQ: receive block request";

    if (request["Dest"] != userName && request["HopLimit"].toInt() > 0 && forward) {
        QPair<QHostAddress, quint16> pair = routingTable[request["Dest"].toString()];
        request["HopLimit"] = request["HopLimit"].toInt() - 1;
        sendMsg(pair.first, pair.second, request);
    } else if (request["Dest"] == userName) {
        QByteArray hash = request["BlockRequest"].toByteArray();
        QString filename = hashToFile[hash].first;
        int blockid = hashToFile[hash].second;
        QByteArray data = readBlockData(filename, blockid);

        QVariantMap reply;
        reply.insert("Dest", request["Origin"].toString());
        reply.insert("Origin", userName);
        reply.insert("HopLimit", 10);
        reply.insert("BlockReply", hash);
        reply.insert("Data", data);

        QPair<QHostAddress, quint16> pair = routingTable[request["Origin"].toString()];
        sendMsg(pair.first, pair.second, reply);
    }
}

void Node::receiveBlockReply(QVariantMap reply)
{
    qDebug() << "BLOCK REPlY: receive block reply";
    QByteArray hash = reply["BlockReply"].toByteArray();
    QByteArray data = reply["Data"].toByteArray();
    QString origin = reply["Origin"].toString();
    if (metaFileRequests.contains(hash)) {
        //receive a metadata
        int size = data.size();
        QByteArray ba1 = data.mid(0, 20);
        QByteArray ba2;
        if (size > 20) {
            ba2 += data.mid(20);
        }
        sendBlockRequest(origin, ba1);
        QString fileName = "file_" + QString(file_id);
        file_id++;
        blockRequests[ba1] = QPair<QString, QByteArray>(fileName, ba2);
        metaFileRequests.remove(hash);
    } else if (blockRequests.contains(hash)){
        //receive a data block
        QString filename = blockRequests[hash].first;
        write(filename, data);
        QByteArray ba = blockRequests[hash].second;
        if (ba.size() != 0) {
            QByteArray ba1 = ba.mid(0, 20);
            QByteArray ba2;
            if (ba.size() > 20) {
                ba2 += data.mid(20);
            }
            blockRequests[ba1] = QPair<QString, QByteArray>(filename, ba2);
            sendBlockRequest(origin, ba1);
        }
        blockRequests.remove(hash);
    }
}

QByteArray Node::readBlockData(QString& filename, int id)
{
    QFile file(filename);
    QByteArray data;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return data;
    QTextStream in(&file);
    in.seek(id * BLOCKSIZE);
    data += in.read(8000);
    return data;
}

void Node::shareFile(const QString& filename)
{
   QFile file(filename);
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
       return;
   qDebug() << "file size: " << file.size();
   QByteArray bhash;
   int i = 0;
   while (!file.atEnd()) {
       QByteArray data = file.read(BLOCKSIZE);
       QByteArray h = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
       qDebug() << "hash size: " << h.size();
       hashToFile.insert(h, QPair<QString, int>(filename, i));
       bhash += h;
   }

 //  qDebug() << hash;
   QString mfpath = filename.split(".")[0] + ".mf";
   qDebug() << "metafile: " << mfpath;
   write(mfpath, bhash);
   QByteArray fid = QCryptographicHash::hash(bhash, QCryptographicHash::Sha1);
   File *mf = new File(filename, file.size(), mfpath, fid);
   metafiles[fid] = mf;
   hashToFile.insert(fid, QPair<QString, int>(mfpath, 0));
}
void Node::download(const QString& nodeId, const QString& hash)
{
    sendBlockRequest(nodeId, hash);
    metaFileRequests.insert(hash);
}

void Node::sendBlockRequest(const QString& nodeId, const QString& hash)
{
    QVariantMap msg;
    msg.insert("Dest", nodeId);
    msg.insert("Origin", userName);
    msg.insert("HopLimit", 10);
    msg.insert("BlockRequest", hash);

    QPair<QHostAddress, quint16> pair = routingTable[nodeId];
    sendMsg(pair.first, pair.second, msg);

    qDebug() << "send block request to " << pair.first;
}

void Node::write(const QString& filename, const QByteArray& data)
{
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }
    QTextStream out(&f);
    out << data;
}



