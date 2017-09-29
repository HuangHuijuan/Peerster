#ifndef PEERSTER_MAIN_HH
#define PEERSTER_MAIN_HH
#include <QMap>
#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QSignalMapper>
#include "node.h"


class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();

public slots:
	void gotReturnPressed();
    void addPeerButtonClicked();
    void appendLog(const QString& text);
    void addPeer(const QString& s);
    void newDialog(QListWidgetItem*);
    void sendBtnClicked(const QString& des);
    void receiveNewPrivLog(const QString& origin, const QString& text);

protected:
	bool eventFilter(QObject *obj, QEvent *e);
	
private:
    QLabel *label;
    QLabel *peerLabel;
    QLabel *peerListLabel;
//    QLineEdit *host;
//    QLineEdit *port;
    QLineEdit *peerInfo;
    QPushButton *addButton;
	QTextEdit *textview;
	QTextEdit *textline;
    QTextEdit *peerList;
    QListWidget *onlinePeers;
    Node *node;
    QSignalMapper *mapper;
    QMap<QString, QTextEdit*> peerInputBoxMap;
    QMap<QString, QTextEdit*> peerChatAreaMap;
};





#endif // PEERSTER_MAIN_HH
