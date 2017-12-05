#ifndef RATINGDIALOG_H
#define RATINGDIALOG_H
#include <QTextEdit>
#include <QDialog>
#include <QComboBox>
#include <QPushButton>

class RatingDialog : public QDialog
{
    Q_OBJECT
public:
    RatingDialog(const QString& metadataStr, const QString& filename, const QString& publisher);

signals:
    void rateRes(const QString& fileId, const QString& publisher, double score);

public slots:
    void submitBtnClicked();

private:
    QComboBox *combobox;
    QPushButton *submitBtn;
    QString metadataStr;
    QString filename;
    QString publisher;
};

#endif // RATINGDIALOG_H
