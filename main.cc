#include <QApplication>
#include <QVBoxLayout>
#include <QDebug>
#include <QKeyEvent>
#include <QVector>
#include <QFormLayout>
#include "p2pchatdialog.h"
#include "main.hh"

ChatDialog::ChatDialog()
{
	setWindowTitle("Peerster");

    label = new QLabel("hostname:port/ipaddr:port :", this);
    peerInfo = new QLineEdit(this);
    addButton = new QPushButton("Add Peer",this);

//    peerListLabel = new QLabel("Peer List:");
//    peerList = new QTextEdit(this);
//    peerList->setReadOnly(true);

    peerLabel = new QLabel("Online Peers:");
    onlinePeers = new QListWidget(this);


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
    QGridLayout *layout1 = new QGridLayout();

    layout1->addWidget(textview, 0, 0, 2, -1);
    layout1->addWidget(textline, 2, 0, 1, -1);
    layout1->setRowStretch(1, 200);
    layout1->setRowStretch(2, 100);

    QVBoxLayout *layout2 = new QVBoxLayout();
    layout2->addWidget(label);
    layout2->addWidget(peerInfo);
    layout2->addWidget(addButton);
    layout2->addWidget(peerLabel);
    layout2->addWidget(onlinePeers);

    QHBoxLayout *globalLayout = new QHBoxLayout();
    globalLayout->addLayout(layout1);
    globalLayout->addLayout(layout2);
    setLayout(globalLayout);

    mapper = new QSignalMapper();

    node = new Node();

    connect(addButton, SIGNAL(clicked()), this, SLOT(addPeerButtonClicked()));
    connect(node, SIGNAL(newLog(QString)), this, SLOT(appendLog(QString)));
    connect(node, SIGNAL(newPeer(QString)), this, SLOT(addPeer(QString)));
    connect(onlinePeers, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(newDialog(QListWidgetItem*)));
    connect(node, SIGNAL(newPrivateLog(QString,QString)), this, SLOT(receiveNewPrivLog(QString, QString)));
}


void ChatDialog::newDialog(QListWidgetItem* item) {
    P2PChatDialog *p2pDialog = new P2PChatDialog(item->text(), node->getUserName());
    p2pDialog->show();
    peerDialogMap.insert(item->text(), p2pDialog);
    connect(p2pDialog, SIGNAL(returnPressed(QString, QString)), this, SLOT(sendPrivMsg(QString, QString)));
    connect(p2pDialog, SIGNAL(dialogClosed(QString)), SLOT(p2pdialogClosed(QString)));
}

void ChatDialog::p2pdialogClosed(const QString& item) {
    peerDialogMap.remove(item);
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

void ChatDialog::addPeerButtonClicked()
{
    if (peerInfo->text() == "") {
        return;
    }
    QStringList list = peerInfo->text().split(":");
    if (list.length() == 0) {
        return;
    }
    QString host = list[0];
    QString port = list[1];
    peerInfo->clear();
    node->addPeer(host, port.toInt());
//    QHostAddress ip = QHostAddress(host);
//    if (ip.isNull()) {//host is a hostname

}

void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...

    node->sendNewMsg(textline->toPlainText());
    textview->append(node->getUserName() + ": " + textline->toPlainText());
	// Clear the textline to get ready for the next input message.
	textline->clear();
}

void ChatDialog::sendPrivMsg(const QString& des, const QString& content) {
    node->sendP2PMsg(des, content);
}


void ChatDialog::appendLog(const QString& text)
{
    textview->append(text);
}

void ChatDialog::receiveNewPrivLog(const QString& origin, const QString& text)
{
    if (!peerDialogMap.contains(origin)) {
        QList<QListWidgetItem*> items = onlinePeers->findItems(origin, Qt::MatchExactly);
        if (items.size() == 0) {
            return;
        }
        newDialog(items[0]);
    }
    peerDialogMap[origin]->chatBox->append(origin + ": " + text);
}

void ChatDialog::addPeer(const QString& s)
{
    onlinePeers->addItem(s);
}

void ChatDialog::closeEvent ( QCloseEvent * event )
{
    event->accept();
    exit(0);
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
