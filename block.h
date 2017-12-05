#ifndef BLOCK_H
#define BLOCK_H
#include <QObject>
#include <QString>
#include <QFile>
#include <QByteArray>

class Block
{
public:
    Block(const QString& fid, const QString& fn, const QString& p, QFile *f, const QByteArray& bid, const QByteArray& h)
    : metadataStr(fid), filename(fn), publisher(p), file(f), block_id(bid), remain_hash(h) { }

    QString metadataStr;
    QString filename;
    QString publisher;
    QFile *file;
    QByteArray block_id;
    QByteArray remain_hash;
};

#endif // BLOCK_H
