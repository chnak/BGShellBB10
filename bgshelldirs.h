/*
 * Where BGShellBB10 keeps the things that must survive an update.
 *
 * Settings, saved SSH hosts, keys, known_hosts and the diagnostic logs used to
 * live under "data/" relative to the working directory, i.e. in
 * appdata/<app id>/data.  That directory is deleted when the bar is
 * reinstalled, so every update wiped the user's setup.
 *
 * The directory cannot simply be replaced with a link to the shared folder:
 * the app is not the owner of its appdata directory, so it cannot rename or
 * unlink the "data" entry itself (the device answers "Permission denied").
 * Instead the persistent root is chosen once at startup and every path that
 * used to say "data/..." is resolved through bgshellDataPath().
 */
#ifndef BGSHELLDIRS_H
#define BGSHELLDIRS_H

#include <QtCore/QDir>
#include <QtCore/QString>

/** Absolute path of the persistent directory, no trailing slash. */
QString bgshellDataRoot();

/** Set once at startup; empty means "stay in the sandbox". */
void bgshellSetDataRoot(const QString &dir);

/**
 * Absolute path of a file inside the persistent directory.  The old
 * "data/..." spellings are accepted unchanged:
 *
 *     bgshellDataPath("data/.ssh/id_rsa")
 *     -> /accounts/1000/removable/sdcard/bgshell/.ssh/id_rsa
 */
inline QString bgshellDataPath(const QString &rel)
{
    QString r = rel;
    if (r.startsWith(QLatin1String("data/")))
        r = r.mid(5);
    else if (r == QLatin1String("data"))
        r = QString();
    if (r.isEmpty())
        return bgshellDataRoot();
    return bgshellDataRoot() + QLatin1Char('/') + r;
}

#endif // BGSHELLDIRS_H
