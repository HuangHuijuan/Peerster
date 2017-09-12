#ifndef PEERSTER_MAIN_HH
#define PEERSTER_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();
	// Bind this socket to a Peerster-specific default port.
	bool bind();
	int getMyPortMin();
	int getMyPortMax();
private:
	int myPortMin, myPortMax;
};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();

public slots:
	void gotReturnPressed();
	void readPendingDatagrams();

protected:
	bool eventFilter(QObject *obj, QEvent *e);
	
private:
	QTextEdit *textview;
	QTextEdit *textline;
	NetSocket sock;
	//send data to each port
	void sendData(const QString& s);
};



#endif // PEERSTER_MAIN_HH
