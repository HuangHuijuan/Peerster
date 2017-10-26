#ifndef NODE_H
#define NODE_H
#include <QTextEdit>
#include <QMap>
#include <QHostInfo>
#include <QTimer>
#include <QListWidget>
#include <QSignalMapper>
#include <QVector>
#include <QSet>
#include "peer.h"
#include "netsocket.h"
#include "file.h"

class Node : public QObject {

    Q_OBJECT

public slots:
    void readPendingDatagrams();
    void aESendStatusMsg();
    void lookedUp(const QHostInfo &host);
    void sendRouteRumor();
    void continueRumormongering(const QString& rumorId);

signals:
    void newLog(const QString& text);
    void newPeer(const QString& s);
    void newPrivateLog(const QString& origin, const QString& log);

private:
    NetSocket sock;
    QVariantMap statusMsg;
    QVariantMap msgRepo;
    QTimer aETimer;
    QTimer rrTimer;
    QMap<QString, Peer*> peers;
    QMap<QString, int> lookUp;
    int seqNo;
    QString userName;
    QHash<QString, QPair<QHostAddress, quint16>> routingTable;
    QMap<QString, QTimer*> rumorTimers;
    QHostAddress myAddress;
    QMap<QByteArray, File*> metafiles;
    QMap<QByteArray, QPair<QString, int>> hashToFile;
    QSet<QString> metaFileRequests;
    QMap<QByteArray, QPair<QString, QByteArray>> blockRequests;
    bool forward;
    static const int BLOCKSIZE = 8000;
    int file_id = 0;

public:
    Node();
    void addStatus(const QString& origin, int seqNo);
    void sendStatusMsg(const QHostAddress& a, quint16 p);
    Peer* getNeighbor();
    int getNeighborSize();
    void processPrivateMsg(QVariantMap privateMsg);
    void processRumorMsg(QVariantMap rumorMsg, const QHostAddress& sender, quint16 senderPort);
    void processStatusMsg(QVariantMap senderStatusMsg, const QHostAddress& sender, quint16 senderPort);
    void initializeNeighbors();
    void sendRumorMsg(const QHostAddress& a, quint16 p, const QString& rumorId);
    void addPeer(const QString& host, quint16 port);
    void sendNewMsg(const QString& content);
    void sendP2PMsg(const QString& des, const QString& chatText);
    QSignalMapper* signalMapper;
    QString& getUserName();
    void sendMsg(const QHostAddress& receiverIP , quint16 receiverPort, const QVariantMap& msg);
    void sendMsgToAllPeers(const QVariantMap& msg);
    void shareFile(const QString& filename);
    void sendBlockRequest(const QString& nodeId, const QString& hash);
    void processBlockRequest(QVariantMap request);
    QByteArray readBlockData(QString& filename, int id);
    void write(const QString& filename, const QByteArray& data);
    void receiveBlockReply(QVariantMap reply);
    void download(const QString& nodeId, const QString& hash);
};
#endif // NODE_H
