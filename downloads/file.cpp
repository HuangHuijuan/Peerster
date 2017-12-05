#include "file.h"

File::File(const QString& n, quint64 s, const QString& m, const QByteArray& h)
{
    name = n;
    size = s;
    metafile = m;
    hash = h;
}
