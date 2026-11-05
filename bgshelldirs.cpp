/*
 * See bgshelldirs.h.
 */
#include "bgshelldirs.h"

static QString gDataRoot;

QString bgshellDataRoot()
{
    if (!gDataRoot.isEmpty())
        return gDataRoot;
    // Nothing persistent was found at startup: behave exactly as before.
    return QDir::current().absoluteFilePath(QString::fromLatin1("data"));
}

void bgshellSetDataRoot(const QString &dir)
{
    gDataRoot = dir;
}
