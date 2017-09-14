#ifndef PEERSTER_MAIN_HH
#define PEERSTER_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QVector>
#include <QTimer>

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();
	// Bind this socket to a Peerster-specific default port.
	bool bind();
	int getMyPortMin();
	int getMyPortMax();
    int getPort();

    QVector<int> getNeighbors();

private:
	int myPortMin, myPortMax;
    QVector<int> neighbors;
    int port;
};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();

public slots:
	void gotReturnPressed();
    void readPendingDatagrams();
    void sendRumorMsg();
    void aESendStatusMsg();

protected:
	bool eventFilter(QObject *obj, QEvent *e);
	
private:
	QTextEdit *textview;
	QTextEdit *textline;
	NetSocket sock;
    QString hostName;
    QVariantMap statusMsg;
    QVariantMap curRumor;
    QVariantMap msgRepo;
    QTimer timer;
    QTimer aaTimer;
    quint16 receiverPort;

    int seqNo;
	//send data to each port
    void addStatus(const QString& origin, const quint32 seqNo);
    void sendStatusMsg(const QHostAddress& host, const quint16 desPor);
    int getNeighborPort();
    int getNeighborPortSize();
    void processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, const quint16 senderPort);
    void processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, const quint16 senderPort);
    void continueRumormongering(quint16 senderPort);
};



#endif // PEERSTER_MAIN_HH
