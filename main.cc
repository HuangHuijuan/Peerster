
#include <unistd.h>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QVariantMap>
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
	connect(&sock, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	// connect(textline, SIGNAL(returnPressed()),
	// 	this, SLOT(gotReturnPressed()));
}

void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...

	sendData(textline->toPlainText());
	qDebug() << "FIX: send message to other peers: " << textline->toPlainText();
	//textview->append(textline->toPlainText());

	// Clear the textline to get ready for the next input message.
	textline->clear();
}
bool ChatDialog::eventFilter(QObject *target, QEvent *e)
{
    if (target == textline && e->type() == QEvent::KeyPress)
    {
        QKeyEvent *event = static_cast<QKeyEvent *>(e);
        if (event->key() == Qt::Key_Return)
        {
            gotReturnPressed(); //发送消息的槽
            return true;
        }
    }
    return QDialog::eventFilter(target, e);
}

void ChatDialog::sendData(const QString& s) 
{
	QVariantMap map;
	QVariant qv(s);
	map.insert("ChatText", qv);
	QByteArray datagram;
	QDataStream out(&datagram, QIODevice::WriteOnly);
	out << map;
	for (int p = sock.getMyPortMin(); p <= sock.getMyPortMax(); p++) {
		sock.writeDatagram(datagram, QHostAddress::LocalHost, p);
	}
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
		QVariantMap map;
		in >> map;
		QDataStream out;
		textview->append(QString::number(senderPort) + ": " + map.take("ChatText").toString());
		//textview->append(map.take("ChatText").toString());
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
			qDebug() << "bound to UDP port " << p;
			
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

