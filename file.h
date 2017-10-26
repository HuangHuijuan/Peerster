#ifndef FILE_H
#define FILE_H
#include <QObject>
#include <QString>

class File : public QObject
{

    Q_OBJECT

public:
    File(const QString& n, quint64 s, const QString& m, const QByteArray& h);
    QString name;
    quint32 size;
    QString metafile;
    QString hash;
};

#endif // FILE_H
