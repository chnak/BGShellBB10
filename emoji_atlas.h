/*
    Colour emoji for BGShellBB10.

    Qt 4's font engine cannot rasterise a colour font (COLR/CPAL/CBDT) and the
    FreeType that ships with the BB10 NDK is 2.5, which is too old to support
    them either.  The emoji are therefore rendered on the host into one atlas
    image (tools/make-emoji-atlas.py) and blitted out of it.

    The terminal itself only ever sees a plain 16 bit character: on the way in
    every emoji is swapped for a private use character (U+E000 + cell index),
    and when text is copied the private use character is swapped back for the
    emoji it stands for.  That keeps the character grid, the width handling
    and the text selection working exactly as they do for any other double
    width character.

    Some emoji are only the colour one when a variation selector (U+FE0F)
    follows them - a red heart, a plane, a pencil.  Those are in the atlas as
    well, but they are only swapped when the selector is there, so that the
    same character without the selector stays the monochrome one.
*/

#ifndef EMOJI_ATLAS_H
#define EMOJI_ATLAS_H

#include <QImage>
#include <QRect>
#include <QString>

namespace EmojiAtlas {

// Load the atlas.  Safe to call more than once; the second call is a no-op.
// Returns false when the asset is missing, and the terminal then simply keeps
// showing the emoji as (missing) monochrome glyphs.
bool init();

bool isReady();

// True for a private use character that stands in for an emoji.
bool isPlaceholder(quint16 ch);

// The atlas cell a placeholder stands for, or -1.
int indexFor(quint16 ch);

// The code point a placeholder stands for, or 0.
unsigned int codepointFor(quint16 ch);

// The emoji a placeholder stands for, as text.  The variation selector is
// part of it when the emoji needs it.  Empty when the character is not a
// placeholder.
QString textFor(quint16 ch);

// The placeholder for an emoji code point, or 0 when the emoji is not in the
// atlas.  'withVariationSelector' tells whether a variation selector follows
// the emoji; the ones that are the colour one only after such a selector
// need it.
quint16 placeholderFor(unsigned int codepoint, bool withVariationSelector);

// The whole atlas.  Null when it could not be loaded.
const QImage& atlasImage();

// The cell of the given index inside the atlas.
QRect cellRect(int index);

// Replace every emoji in the text by its placeholder.
QString encode(const QString& text);

// Replace every placeholder in the text by the emoji it stands for.
QString decode(const QString& text);

} // namespace EmojiAtlas

#endif // EMOJI_ATLAS_H
