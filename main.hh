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
#include "p2pchatdialog.h"

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
    void sendPrivMsg(const QString& des, const QString& content);
    void receiveNewPrivLog(const QString& origin, const QString& text);
    void p2pdialogClosed(const QString& item);

protected:
	bool eventFilter(QObject *obj, QEvent *e);
	
private:
    QLabel *label;
    QLabel *peerLabel;
    QLabel *peerListLabel;
    QLineEdit *peerInfo;
    QPushButton *addButton;
	QTextEdit *textview;
	QTextEdit *textline;
    QTextEdit *peerList;
    QListWidget *onlinePeers;
    Node *node;
    QSignalMapper *mapper;
    QMap<QString, P2PChatDialog*> peerDialogMap;
};





#endif // PEERSTER_MAIN_HH
