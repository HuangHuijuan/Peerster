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
#include <QFile>
#include <QtCrypto>
#include "peer.h"
#include "netsocket.h"
#include "file.h"
#include "msgsender.h"
#include "block.h"

class Node : public QObject {

    Q_OBJECT

public slots:
    void readPendingDatagrams();
    void aESendStatusMsg();
    void lookedUp(const QHostInfo &host);
    void sendRouteRumor();
    void continueRumormongering(const QString& rumorId);
    void continueSearch(const QString& keywords);
    void sendRating(const QString& fileId, const QString& publisher, int score);

signals:
    void newLog(const QString& text);
    void newPeer(const QString& s);
    void newPrivateLog(const QString& origin, const QString& log);
    void newSearchRes(int row, const QString& fileId, double score);
    void downloadFinished(const QString& metadataStr, const QString& filename, const QString& publisher);

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
    QMap<QString, QString> keywordMap;
    QMap<QString, QTimer*> searchTimers;
    QMap<QString, QPair<quint32, quint32>> searchMatches;
    QHostAddress myAddress;
    QMap<QString, QByteArray> metafiles;
    QMap<QString, QPair<QString, int>> hashToFile;
    QMap<QString, QString> metaFileRequests;
    QMap<QByteArray, Block*> blockRequests;
    QSet<QString> searchReplies;
    QMap<double, QString> scores;
    QMap<QString, QByteArray> metadatas;
    bool forward;
    static const int BLOCKSIZE = 8000;
   // QString META_FILES_DIR = "metafiles/";
    QString DOWNLOAD_FILES_DIR = "downloads";
    int file_id;
    QStringList arguments;
    int parseIndex;
    QHostAddress serverIP;
    quint16 serverPort;

    //interact with server
    MsgSender *msgSender;

public:
    Node();
    void setUserName();
    void setServerAddress();
    void addStatus(const QString& origin, int seqNo);
    void sendStatusMsg(const QHostAddress& a, quint16 p);
    Peer* getNeighbor();
    quint32 getNeighborSize();
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
    void sendBlockRequest(const QString& nodeId, const QByteArray& hash);
    void processBlockRequest(const QVariantMap& request);
    QByteArray readBlockData(QString& filename, int id);
    void write(QFile* filename, const QByteArray& data);
    void writeMeta(const QString& filename, const QByteArray& data);
    void receiveBlockReply(const QVariantMap& reply);
    void download(const QString& nodeId, const QString& hash);
    void continueRumormongering(const QString& rumorId, const QHostAddress& add, quint16 port);
    QVector<int> shuffle(int n);
    void redistribute(const QString& origin, const QString& keywords, quint32 budget);
    void sendSearchReply(const QString& reply, const QVariantList& matchList, const QVariantList& matchIds, const QString& des);
    void processSearchRequest(const QVariantMap& req);
    void sendSearchReq(const QString& origin, const QString keywords, quint32 budget, const QHostAddress add, quint16 port);
    void initSearch(const QString keywords);
    void processSearchReply(const QVariantMap& reply);
    void downloadSearchedFile(const QString& fileId);
    void sendAddFriendReq(const QString& friendId);
    void sendScoreReq(const QString& fileId, const QString& publisher);
    void processScoreMsg(const QVariantMap& msg);
};
#endif // NODE_H
