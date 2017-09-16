#ifndef PEERSTER_MAIN_HH
#define PEERSTER_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QHostInfo>
#include "peer.h"
#include "netsocket.h"

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();

public slots:
	void gotReturnPressed();
    void readPendingDatagrams();
    void continueRumormongering();
    void aESendStatusMsg();
    void addPeer();
    void lookedUp(const QHostInfo &host);

protected:
	bool eventFilter(QObject *obj, QEvent *e);
	
private:
    QLabel *label;
    QLabel *peerLabel;
//    QLineEdit *host;
//    QLineEdit *port;
    QLineEdit *peerInfo;
    QPushButton *addButton;
	QTextEdit *textview;
	QTextEdit *textline;
    QTextEdit *peersList;
	NetSocket sock;
    QString hostName;
    QVariantMap statusMsg;
    QVariantMap curRumor;
    QVariantMap msgRepo;
    QTimer timer;
    QTimer aETimer;
   // Peer *receiver
    QMap<QString, Peer*> peers;
    QMap<QString, int> lookUp;

    //std::map<std::string, Peer> peers;

    int seqNo;
	//send data to each port
    void addStatus(const QString& origin, int seqNo);
    void sendStatusMsg(const QHostAddress& a, quint16 p);
    Peer* getNeighbor();
    int getNeighborSize();
    void processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, quint16 senderPort);
    void processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, quint16 senderPort);
    void initializeNeighbors();
    void sendRumorMsg(const QHostAddress& a, quint16 p);
};



#endif // PEERSTER_MAIN_HH
