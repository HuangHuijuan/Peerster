
#include <unistd.h>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QVariantMap>
#include <QHostInfo>
#include <QVector>
#include <QTimer>
#include "main.hh"

ChatDialog::ChatDialog()
{

	setWindowTitle("Peerster");

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new QTextEdit(this);
	textline->setFocus();
	textline->installEventFilter(this);
	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);

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
    hostName = QHostInfo::localHostName() + QString::number(sock.getPort());
    timer.setInterval(2000);
    connect(&timer, SIGNAL(timeout()), this, SLOT(sendRumorMsg()));

    //initalize statusMsg
    statusMsg.insert("Want", QVariant(QVariantMap()));
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
    receiverPort = getNeighborPort();
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

int ChatDialog::getNeighborPort() {
    QVector<int> neighbors = sock.getNeighbors();
    int r = qrand() % neighbors.size();
    return neighbors[r];//randomly choose a neighbor to send the message
}

int ChatDialog::getNeighborPortSize() {
    QVector<int> neighbors = sock.getNeighbors();
    return neighbors.size();
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
    qDebug() << "send rumor message to " <<  receiverPort << ":" << curRumor["ChatText"].toString();
    out << curRumor;
    sock.writeDatagram(datagram, QHostAddress::LocalHost, receiverPort);

    //wait for responding status massage, timer start
    timer.start();
}

void ChatDialog::addStatus(const QString& origin, const quint32 n)
{
    QVariantMap map = statusMsg["Want"].toMap();
    map.insert(origin, QVariant(n + 1));
    statusMsg["Want"] = QVariant(map);
   // qDebug() << origin << qvariant_cast<QVariantMap>(statusMsg["Want"])[origin].toString();
}

void ChatDialog::sendStatusMsg(const QHostAddress& host, const quint16 desPort)
{
    qDebug() << "send status message to" << desPort;
    QByteArray datagram;
    QDataStream out(&datagram, QIODevice::WriteOnly);
    out << statusMsg;
    sock.writeDatagram(datagram, host, desPort);
}

void ChatDialog::readPendingDatagrams()
{
	while (sock.hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(sock.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;
		sock.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
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

void ChatDialog::processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, const quint16 senderPort)
{
    qDebug() << "RUMOR: receive rumor message from " << senderPort << ": " << rumorMsg["ChatText"].toString() << "--->" << rumorMsg["Origin"].toString() << rumorMsg["SeqNo"].toString();
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
        continueRumormongering(senderPort);
    }
    sendStatusMsg(sender, senderPort);
}

void ChatDialog::continueRumormongering(quint16 senderPort) {
    do {
        receiverPort = getNeighborPort();
    } while (receiverPort == senderPort && getNeighborPortSize() > 1);

    if (receiverPort != senderPort && !timer.isActive()) {
        sendRumorMsg();
    }
}

void ChatDialog::processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, const quint16 senderPort) {
    qDebug() << "STATUS: receive status message from" << senderPort;

    timer.stop();

    QVariantMap::Iterator itr;
    QVariantMap status = statusMsg["Want"].toMap();
    QVariantMap senderStatus = senderStatusMsg["Want"].toMap();
    qDebug() << "My Status:";
    for (itr = status.begin(); itr != status.end(); itr++) {
        QString origin = itr.key();
        qDebug() << origin << status[origin].toInt();
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
                if (!timer.isActive()) {
                    sendRumorMsg();
                }
                return;
            }
        } else {
            curRumor = msgRepo[origin + "_1"].toMap();
            receiverPort = senderPort;
            if (!timer.isActive()) {
                sendRumorMsg();
            }
            return;
        }
    }
    qDebug() << "Sender status:";
    for (itr = senderStatus.begin(); itr != senderStatus.end(); itr++) {
        qDebug() << itr.key() << senderStatus[itr.key()].toInt();
        if (!status.contains(itr.key())) {
            sendStatusMsg(sender, senderPort);
            return;
        }
    }

    if (qrand() % 2) {
        //1, head, send rumor
        continueRumormongering(senderPort);
    }
}

NetSocket::NetSocket()
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four Peerster instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;

}

bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
            port = p;
			qDebug() << "bound to UDP port " << p;
            if (p + 1 <= myPortMax) {
                neighbors.append(p + 1);
            }
            if (p - 1 >= myPortMin) {
                neighbors.append(p - 1);
            }
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

int NetSocket::getMyPortMin() {
	return myPortMin;
}

int NetSocket::getMyPortMax() {
	return myPortMax;
}
int NetSocket::getPort() {
    return port;
}
QVector<int> NetSocket::getNeighbors() {
    return neighbors;
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

