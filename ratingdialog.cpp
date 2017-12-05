#include "ratingdialog.h"
#include <QLabel>
#include <QGridLayout>

RatingDialog::RatingDialog(const QString& str, const QString& filename, const QString& publisher)
    : metadataStr(str), filename(filename), publisher(publisher)
{
    QLabel* label = new QLabel("File download is finished! Please rate the file.");
    QLabel* fnLabel = new QLabel("File name: " + filename);
    QLabel* pubLabel = new QLabel("Publisher: " + publisher);
    combobox = new QComboBox(this);
    combobox->addItem("0");
    combobox->addItem("1");
    combobox->addItem("2");
    combobox->addItem("3");
    combobox->addItem("4");
    combobox->addItem("5");
    submitBtn = new QPushButton("OK", this);
    QGridLayout* gl = new QGridLayout();
    gl->addWidget(label);
    gl->addWidget(fnLabel);
    gl->addWidget(pubLabel);
    gl->addWidget(combobox);
    gl->addWidget(submitBtn);
    this->setLayout(gl);
    connect(submitBtn, SIGNAL(clicked(bool)), this, SLOT(submitBtnClicked()));
}

void RatingDialog::submitBtnClicked()
{
    QString fileId = metadataStr + ";" + filename + ";" + publisher;
    emit rateRes(fileId, publisher, combobox->currentText().toDouble());
    //qDebug() << "It should be closed";
    close();
}
