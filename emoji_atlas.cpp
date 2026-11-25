/*
    Colour emoji for BGShellBB10 - see emoji_atlas.h for the background.
*/

#include "emoji_atlas.h"
#include "emoji_atlas_data.h"
#include "diag.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QtGui/QImage>

namespace {

// The variation selector that turns the emoji before it into the colour one.
const ushort VARIATION_SELECTOR_16 = 0xFE0F;

// konsole_wcwidth.cpp hardcodes the range of the placeholder characters so
// that it does not have to pull in this header; the two have to agree.
#if EMOJI_ATLAS_PUA_BASE != 0xE000 || EMOJI_ATLAS_PUA_END > 0xE200
#error "the placeholder range in konsole_wcwidth.cpp does not match the atlas"
#endif

QImage g_atlas;
bool g_ready = false;
bool g_tried = false;

// Index of a code point in the atlas, or -1 when it is not in the atlas.
// The table is sorted by code point, so this is a binary search.
int entryIndex(unsigned int codepoint, unsigned short* flags)
{
    int lo = 0;
    int hi = EMOJI_ATLAS_COUNT - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        if (emoji_atlas_entries[mid].codepoint == codepoint) {
            if (flags)
                *flags = emoji_atlas_entries[mid].flags;
            return emoji_atlas_entries[mid].index;
        }
        if (emoji_atlas_entries[mid].codepoint < codepoint)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}

} // namespace

bool EmojiAtlas::init()
{
    if (g_tried)
        return g_ready;
    g_tried = true;

    const QString appDir = QCoreApplication::applicationDirPath();

    // The BAR puts the assets in native/assets, but the app is started from a
    // few different working directories, so look in all the places the bundled
    // font is looked for as well.
    QStringList candidates;
    candidates << QDir::currentPath() + QString("/assets/emoji_atlas.bin")
               << QDir::currentPath() + QString("/native/assets/emoji_atlas.bin")
               << QDir::currentPath() + QString("/app/native/assets/emoji_atlas.bin")
               << appDir + QString("/assets/emoji_atlas.bin")
               << appDir + QString("/native/assets/emoji_atlas.bin")
               << appDir + QString("/app/native/assets/emoji_atlas.bin")
               << appDir + QString("/../assets/emoji_atlas.bin")
               << QString("/accounts/1000/appdata/com.q20.shell/app/native/assets/emoji_atlas.bin");

    // The executable lives in <app>/native/bin and the assets in
    // <app>/native/assets, so walk up from it and from the working directory
    // until the assets are found.  This covers the sandbox install as well as
    // a sideloaded one, where the whole tree sits under the app data folder.
    const QStringList roots = QStringList() << appDir << QDir::currentPath();
    for (int r = 0; r < roots.size(); ++r) {
        QString directory = roots.at(r);
        for (int level = 0; level < 4; ++level) {
            candidates << directory + QString("/native/assets/emoji_atlas.bin")
                       << directory + QString("/assets/emoji_atlas.bin");
            const QString parent = QDir(directory).absolutePath();
            const QString up = QDir(parent).absolutePath();
            if (up == directory || parent.isEmpty())
                break;
            directory = parent;
        }
    }

    const qint64 expected = (qint64)EMOJI_ATLAS_WIDTH * EMOJI_ATLAS_HEIGHT * 4;

    for (int i = 0; i < candidates.size(); ++i) {
        if (candidates.at(i).isEmpty())
            continue;
        QFile file(candidates.at(i));
        if (!file.exists())
            continue;
        if (!file.open(QIODevice::ReadOnly))
            continue;

        const QByteArray data = file.readAll();
        file.close();

        if ((qint64)data.size() != expected) {
            qWarning() << "emoji atlas:" << candidates.at(i) << "has" << data.size()
                       << "bytes, expected" << expected;
            continue;
        }

        // The file is raw BGRA, which is what Format_ARGB32 stores on a little
        // endian machine.  The paint engine wants premultiplied alpha, so
        // convert once here instead of on every draw.
        QImage image((uchar*)data.data(), EMOJI_ATLAS_WIDTH, EMOJI_ATLAS_HEIGHT,
                     QImage::Format_ARGB32);
        g_atlas = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (g_atlas.isNull())
            continue;

        g_ready = true;
        qDebug() << "emoji atlas:" << EMOJI_ATLAS_COUNT << "emoji loaded from"
                 << candidates.at(i);
        bgshellDiag("emoji atlas: %d emoji from %s",
                    EMOJI_ATLAS_COUNT, candidates.at(i).toLocal8Bit().data());
        return true;
    }

    qWarning("emoji atlas not found - colour emoji are disabled");
    bgshellDiag("emoji atlas: not found - colour emoji are disabled");
    return false;
}

bool EmojiAtlas::isReady()
{
    if (!g_tried)
        init();
    return g_ready;
}

bool EmojiAtlas::isPlaceholder(quint16 ch)
{
    return ch >= EMOJI_ATLAS_PUA_BASE && ch < EMOJI_ATLAS_PUA_END;
}

int EmojiAtlas::indexFor(quint16 ch)
{
    if (!isPlaceholder(ch))
        return -1;
    return ch - EMOJI_ATLAS_PUA_BASE;
}

unsigned int EmojiAtlas::codepointFor(quint16 ch)
{
    const int index = indexFor(ch);
    if (index < 0 || index >= EMOJI_ATLAS_COUNT)
        return 0;
    return emoji_atlas_codepoints[index];
}

QString EmojiAtlas::textFor(quint16 ch)
{
    const int index = indexFor(ch);
    if (index < 0 || index >= EMOJI_ATLAS_COUNT)
        return QString();

    const uint value = emoji_atlas_codepoints[index];
    QString text = QString::fromUcs4(&value, 1);

    // The emoji that are the colour one only after a variation selector need
    // it back, or the copy would be the monochrome one.
    unsigned short flags = 0;
    if (entryIndex(emoji_atlas_codepoints[index], &flags) >= 0 &&
        (flags & EMOJI_ATLAS_VS16))
        text.append(QChar(VARIATION_SELECTOR_16));

    return text;
}

quint16 EmojiAtlas::placeholderFor(unsigned int codepoint, bool withVariationSelector)
{
    unsigned short flags = 0;
    const int index = entryIndex(codepoint, &flags);
    if (index < 0)
        return 0;
    if ((flags & EMOJI_ATLAS_VS16) && !withVariationSelector)
        return 0;
    return (quint16)(EMOJI_ATLAS_PUA_BASE + index);
}

const QImage& EmojiAtlas::atlasImage()
{
    return g_atlas;
}

QRect EmojiAtlas::cellRect(int index)
{
    if (index < 0 || index >= EMOJI_ATLAS_COUNT)
        return QRect();
    const int column = index % EMOJI_ATLAS_COLS;
    const int row = index / EMOJI_ATLAS_COLS;
    return QRect(column * EMOJI_ATLAS_CELL, row * EMOJI_ATLAS_CELL,
                 EMOJI_ATLAS_CELL, EMOJI_ATLAS_CELL);
}

QString EmojiAtlas::encode(const QString& text)
{
    if (!isReady() || text.isEmpty())
        return text;

    // Most output has no emoji at all.  Look before building a new string so
    // that the common case does not allocate anything.  U+00A9 is the lowest
    // code point an emoji can have, so anything below it cannot be one.
    bool any = false;
    for (int i = 0; i < text.length() && !any; i++) {
        const QChar ch = text.at(i);
        if (ch.unicode() == VARIATION_SELECTOR_16) {
            any = true;
        } else if (ch.isHighSurrogate()) {
            any = (i + 1 < text.length() && text.at(i + 1).isLowSurrogate());
        } else if (ch.unicode() >= 0x00A9) {
            any = (entryIndex(ch.unicode(), 0) >= 0);
        }
    }
    if (!any)
        return text;

    QString result;
    result.reserve(text.length());

    int i = 0;
    while (i < text.length()) {
        const QChar ch = text.at(i);
        unsigned int codepoint = ch.unicode();
        int consumed = 1;

        if (ch.isHighSurrogate() && i + 1 < text.length() && text.at(i + 1).isLowSurrogate()) {
            codepoint = QChar::surrogateToUcs4(ch, text.at(i + 1));
            consumed = 2;
        }

        // A variation selector makes the emoji before it the colour one.
        const bool vs16 = (i + consumed < text.length() &&
                           text.at(i + consumed).unicode() == VARIATION_SELECTOR_16);

        const quint16 placeholder = placeholderFor(codepoint, vs16);
        if (placeholder) {
            result.append(QChar(placeholder));
        } else {
            result.append(ch);
            if (consumed == 2)
                result.append(text.at(i + 1));
        }

        // The variation selector is part of the emoji and is never drawn; when
        // the emoji before it is not in the atlas it is dropped as well,
        // because on its own it means nothing.
        if (vs16)
            consumed++;

        i += consumed;
    }

    return result;
}

QString EmojiAtlas::decode(const QString& text)
{
    if (!isReady() || text.isEmpty())
        return text;

    bool any = false;
    for (int i = 0; i < text.length() && !any; i++)
        any = isPlaceholder(text.at(i).unicode());
    if (!any)
        return text;

    QString result;
    result.reserve(text.length());

    for (int i = 0; i < text.length(); i++) {
        const QChar ch = text.at(i);
        const QString emoji = textFor(ch.unicode());
        if (!emoji.isEmpty())
            result.append(emoji);
        else
            result.append(ch);
    }

    return result;
}
