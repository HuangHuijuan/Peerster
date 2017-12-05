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
    void shareFileButtonClicked();
    void addFriendBtnClicked();
    void appendLog(const QString& text);
    void addPeer(const QString& s);
    void newDialog(QListWidgetItem*);
    void sendPrivMsg(const QString& des, const QString& content);
    void receiveNewPrivLog(const QString& origin, const QString& text);
    void p2pdialogClosed(const QString& item);
    void search();
    void addSearchRes(int row, const QString& fileId, double score);
    void downloadSearchedFile(QListWidgetItem *item);
    void askRate(const QString& fileId, const QString& filename, const QString& publisher);

protected:
	bool eventFilter(QObject *obj, QEvent *e);
    void closeEvent ( QCloseEvent * event);
	
private:
    QLineEdit *peerInfo;
    QPushButton *addButton;
	QTextEdit *textview;
	QTextEdit *textline;
    QTextEdit *peerList;
    QListWidget *onlinePeers;
    QListWidget *sharedFiles;
    QPushButton *shareFileBtn;
    Node *node;
    QSignalMapper *mapper;
    QMap<QString, P2PChatDialog*> peerDialogMap;
    QListWidget *fileLists;
    QLineEdit *keywords;
    QPushButton *searchBtn;
    QLineEdit *friendID;
    QPushButton *addFriendBtn;
    QMap<QString, QString> fileInfo;
};





#endif // PEERSTER_MAIN_HH
