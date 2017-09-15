#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QVector>
#include <QFormLayout>
#include "main.hh"
#include "netsocket.h"
#include "peer.h"

ChatDialog::ChatDialog()
{
	setWindowTitle("Peerster");

    label = new QLabel("hostname:port/ipaddr:port :", this);
    peerInfo = new QLineEdit(this);
    addButton = new QPushButton("Add Peer",this);

    peerLabel = new QLabel("Peers:");
    peersList = new QTextEdit(this);
    peersList->setReadOnly(true);
//    peersList->setMinimumWidth(50);
//    peersList->setMaximumWidth(200);

//    QFormLayout *formLayout = new QFormLayout();
//    formLayout->setFormAlignment(Qt::AlignLeft);
//    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
//    formLayout->addRow(label1, host);
//    formLayout->addRow(label2, port);

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
    textview->setMinimumHeight(300);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new QTextEdit(this);
	textline->setFocus();
    textline->setMinimumHeight(50);
    textline->setMaximumHeight(100);
	textline->installEventFilter(this);
	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout1 = new QVBoxLayout();
 //   layout->addLayout(formLayout);

    layout1->addWidget(textview);
    layout1->addWidget(textline);

    QVBoxLayout *layout2 = new QVBoxLayout();
    layout2->addWidget(label);
    layout2->addWidget(peerInfo);
    layout2->addWidget(addButton);
    layout2->addWidget(peerLabel);
    layout2->addWidget(peersList);

    QHBoxLayout *globalLayout = new QHBoxLayout();
    globalLayout->addLayout(layout1);
    globalLayout->addLayout(layout2);
    setLayout(globalLayout);

	// Create a UDP network socket
	if (!sock.bind())
        exit(1);

	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	// connect(textline, SIGNAL(returnPressed()),
	// 	this, SLOT(gotReturnPressed()));
    connect(&sock, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

    //initialize the seqNo to be 1;
    seqNo = 1;
    hostName = QHostInfo::localHostName() + ":" + QString::number(sock.getPort());
    timer.setInterval(2000);
    aETimer.setInterval(10000);
    aETimer.start();
    connect(&timer, SIGNAL(timeout()), this, SLOT(sendRumorMsg()));
    connect(&aETimer, SIGNAL(timeout()), this, SLOT(aESendStatusMsg()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addPeer()));
    //initalize statusMsg
    statusMsg.insert("Want", QVariant(QVariantMap()));
    initializeNeighbors();
}

void ChatDialog::initializeNeighbors()
{
    QHostInfo info=QHostInfo::fromName(QHostInfo::localHostName());
    QHostAddress myAddress=info.addresses().first();
    qDebug() << myAddress.toString() << info.hostName();
    Peer *peer;
    for (int p = sock.getMyPortMin(); p <= sock.getMyPortMax(); p++) {
        if (p == sock.getPort()) {
            continue;
        }
        peer = new Peer(p, myAddress, info.hostName());
        peers.insert(peer->toString(), peer);
        peersList->append(peer->toString());
    }
    //add host from command line to peer
    QStringList arguments = QCoreApplication::arguments();
    for (int i = 1; i < arguments.size(); i++) {
        QStringList list = arguments[i].split(":");
        int id = QHostInfo::lookupHost(list[0], this, SLOT(lookedUp(QHostInfo)));
        lookUp.insert(QString::number(id), list[1].toInt());
    }
}
void ChatDialog::addPeer()
{
    QStringList list = peerInfo->text().split(":");
    QString host = list[0];
    QString port = list[1];
    peerInfo->clear();
//    QHostAddress ip = QHostAddress(host);
//    if (ip.isNull()) {//host is a hostname
    int id = QHostInfo::lookupHost(host, this, SLOT(lookedUp(QHostInfo)));
    lookUp.insert(QString::number(id), port.toInt());
}

void ChatDialog::lookedUp(const QHostInfo &host)
{
    if (host.error() != QHostInfo::NoError) {
        qDebug() << "Lookup failed:" << host.errorString();
        return;
    }
    QString lookUpId = QString::number(host.lookupId());
    int port = lookUp[lookUpId];
    QHostAddress address = host.addresses().first();
    QString hostName = host.hostName();
    Peer *p = new Peer(port, address, hostName);
    QString s = p->toString();
    if (!peers.contains(s)) {
        peers.insert(s, p);
        peersList->append(s);
        qDebug() << "Add peer:" << hostName << address.toString() << port;
    }
    lookUp.remove(lookUpId);
}

void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...

    QString content = textline->toPlainText();
    QVariantMap msg;
    msg.insert("ChatText", content);
    msg.insert("Origin", hostName);
    msg.insert("SeqNo", seqNo);
    curRumor = msg;
    Peer *neighbor = getNeighbor();
    receiverIP = neighbor->getAddress();
    receiverPort = neighbor->getPort();
    if (!timer.isActive()) {
        sendRumorMsg();
    }
    addStatus(hostName, seqNo);
    msgRepo.insert(hostName + "_" + QString::number(seqNo), msg);
    seqNo++;
    textview->append(hostName + ": " + content);

	// Clear the textline to get ready for the next input message.
	textline->clear();
}

Peer* ChatDialog::getNeighbor()
{
    qDebug() << peers.size();
    int r = qrand() % peers.size();
    QMap<QString,Peer*>::iterator i = peers.begin();
    Peer *p = (i + r).value();
    return p;
}

int ChatDialog::getNeighborSize()
{
    return peers.size();
}

bool ChatDialog::eventFilter(QObject *target, QEvent *e)
{
    if (target == textline && e->type() == QEvent::KeyPress)
    {
        QKeyEvent *event = static_cast<QKeyEvent *>(e);
        if (event->key() == Qt::Key_Return)
        {
            gotReturnPressed();
            return true;
        }
    }
    return QDialog::eventFilter(target, e);
}

void ChatDialog::sendRumorMsg()
{
	QByteArray datagram;
	QDataStream out(&datagram, QIODevice::WriteOnly);
    qDebug() << "send rumor message to " << receiverIP << ":" <<receiverPort << curRumor["ChatText"].toString();
    out << curRumor;
    sock.writeDatagram(datagram, receiverIP, receiverPort);

    //wait for responding status massage, timer start
    timer.start();
}

void ChatDialog::addStatus(const QString& origin, int n)
{
    QVariantMap map = statusMsg["Want"].toMap();
    map.insert(origin, QVariant(n + 1));
    statusMsg["Want"] = QVariant(map);
   // qDebug() << origin << qvariant_cast<QVariantMap>(statusMsg["Want"])[origin].toString();
}

void ChatDialog::sendStatusMsg(const QHostAddress desAddr, quint16 desPort)
{
    qDebug() << "send status message to" << desAddr.toString() << ":" << desPort;
    QByteArray datagram;
    QDataStream out(&datagram, QIODevice::WriteOnly);
    out << statusMsg;
    sock.writeDatagram(datagram, desAddr, desPort);
}

void ChatDialog::aESendStatusMsg()
{
    qDebug() << "Anti-entropy starts";
    Peer *neighbor = getNeighbor();
    sendStatusMsg(neighbor->getAddress(), neighbor->getPort());
}
void ChatDialog::readPendingDatagrams()
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
        if (msg.contains("ChatText")) {
            processRumorMsg(msg, sender, senderPort);
        } else if (msg.contains("Want")) {
            processStatusMsg(msg, sender, senderPort);
        }
	}
}

void ChatDialog::processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, quint16 senderPort)
{
    QString senderName = sender.toString() + ":" + QString::number(senderPort);
    qDebug() << "RUMOR: receive rumor message from " << senderName << ": " << rumorMsg["ChatText"].toString() << "--->" << rumorMsg["Origin"].toString() << rumorMsg["SeqNo"].toString();
    int n = rumorMsg["SeqNo"].toInt();
    QString origin = rumorMsg["Origin"].toString();
    QVariantMap map = statusMsg["Want"].toMap();
    if ((!map.contains(origin) && n == 1)|| (map.contains(origin) && map[origin].toInt() == n)) {
        qDebug() << "append text to TextView";
        textview->append(origin + ": " + rumorMsg["ChatText"].toString());
        //add rumor msg to the map
        msgRepo.insert(origin + "_" +QString::number(n), QVariant(rumorMsg));
        //send status msg to the sender
        addStatus(origin, n);
        //send the new rumor to its neighbour
        curRumor = rumorMsg;
        continueRumormongering(senderName);
    }
    sendStatusMsg(sender, senderPort);
}

void ChatDialog::continueRumormongering(const QString& senderName) {
    Peer *receiver;
    do {
        receiver = getNeighbor();
    } while (receiver->toString() == senderName && getNeighborSize() > 1);

    if (receiver->toString() != senderName && !timer.isActive()) {
        receiverIP = receiver->getAddress();
        receiverPort = receiver->getPort();
        sendRumorMsg();
    }
}

void ChatDialog::processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, quint16 senderPort) {
    QString senderName = sender.toString() + ":" + QString::number(senderPort);
    qDebug() << "STATUS: receive status message from" << senderName;

    timer.stop();

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
                curRumor = msgRepo[origin + "_" + QString::number(senderSeqNo)].toMap();
                receiverPort = senderPort;
                receiverIP = sender;
                if (!timer.isActive()) {
                    sendRumorMsg();
                }
                return;
            }
        } else {
            curRumor = msgRepo[origin + "_1"].toMap();
            receiverIP = sender;
            receiverPort = senderPort;
            if (!timer.isActive()) {
                sendRumorMsg();
            }
            return;
        }
    }
    //qDebug() << "Sender status:";
    for (itr = senderStatus.begin(); itr != senderStatus.end(); itr++) {
     //   qDebug() << itr.key() << senderStatus[itr.key()].toInt();
        if (!status.contains(itr.key())) {
            sendStatusMsg(sender, senderPort);
            return;
        }
    }

    if (qrand() % 2) {
        //1, head, send rumor
        continueRumormongering(senderName);
    }
}


int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();
	
	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

