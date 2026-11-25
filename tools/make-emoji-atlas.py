#!/usr/bin/env python3
"""Build the colour emoji atlas that BGShellBB10 draws instead of glyphs.

Qt 4's font engine cannot rasterise colour fonts (COLR/CPAL/CBDT), and the
FreeType that ships with the BB10 NDK is 2.5 - too old for them as well.  So
the emoji are rendered here, on the host, into one atlas image; the app only
has to blit a rectangle of it.

The atlas is a grid of square cells.  Each cell holds one emoji, drawn from
Noto Color Emoji (CBDT).  The pixels are stored as raw BGRA - the byte order
Qt's Format_ARGB32 uses on little endian ARM - with no header and no
compression: the app wraps the bytes in a QImage directly, which needs
neither an image codec nor zlib on the device.  The dimensions live in the
generated header, and the app checks the file size against them.

Usage:
    make-emoji-atlas.py --font NotoColorEmoji.ttf \
                        --emoji-data emoji-data.txt \
                        --out-bin assets/emoji_atlas.bin \
                        --out-header emoji_atlas_data.h

The emoji source font and emoji-data.txt are not kept in the repository (the
font alone is 10 MB); fetch them from
  https://github.com/googlefonts/noto-emoji  -> 2D/fonts/NotoColorEmoji.ttf
  https://www.unicode.org/Public/UNIDATA/emoji/emoji-data.txt
"""

import argparse
import io
import os
import re
import sys

# How an emoji is turned into its colour form.
FLAG_ALWAYS = 1   # colour on its own
FLAG_VS16 = 2     # colour only after a variation selector

# ---------------------------------------------------------------------------
# The emoji we ship, in rough order of how often they turn up in terminal
# output.  Only codepoints whose default presentation is the emoji one
# (Emoji_Presentation=Yes) are eligible here: those are the ones a terminal
# shows in colour and, per UAX #11, two columns wide.  Text presentation
# symbols (a plain U+2713 CHECK MARK, for example) stay monochrome and one
# column wide, exactly like every other terminal - unless they are followed
# by a variation selector, which is what the second list below is for.
# ---------------------------------------------------------------------------
CURATED = """
# faces and feelings
1F600 1F603 1F604 1F601 1F606 1F605 1F923 1F602 1F642 1F643 1F609 1F60A
1F607 1F970 1F60D 1F929 1F618 1F617 1F61A 1F619 1F60B 1F61B 1F61C 1F92A
1F61D 1F911 1F917 1F92D 1F92B 1F914 1F910 1F928 1F610 1F611 1F636 1F60F
1F612 1F644 1F62C 1F925 1F60C 1F614 1F62B 1F624 1F622 1F62D 1F631 1F616
1F623 1F61E 1F613 1F629 1F62A 1F62E 1F62F 1F632 1F633 1F97A 1F979 1F97B
1F97C 1F97D 1F97E 1F97F 1F9D5 1F9D6 1F9D7 1F9D8 1F9D9 1F9DA 1F9DB 1F9DC
1F9DD 1F9DE 1F9DF 1F9E0 1F9E1
# hands and gestures
1F44B 1F44C 1F44D 1F44E 1F44F 1F450 1F91A 1F91B 1F91C 1F91D 1F91E 1F918
1F919 1F595 1F596 1F64B 1F64C 1F64D 1F64E 1F64F 1F926 1F937 270D 1F448
1F449 1F446 1F447 1F4AA
# hearts and feelings
1F494 1F495 1F496 1F497 1F498 1F499 1F49A 1F49B 1F49C 1F49D 1F49E 1F49F
1F4A0 1F5A4 1F90D 1F90E 1F48B 1F48C 1F48D 1F48E 1F48F
# stars, fire, sparkles
2B50 2728 1F31F 1F525 26A1 1F4AF 1F4A6 1F4A7 1F4A8 1F4A9 1F4A2 1F4A5
1F4A1 1F4A3 1F4A4 1F4AB 1F4AC 1F4AD 1F4AE 1F4A0 1F4B0 1F4B1 1F4B2
1F4B3 1F4B4 1F4B6 1F4B7 1F4B8 1F4B9 1F4BA 1F4BC 1F4BD
# weather and sky
2600 2601 2602 2603 2604 26C5 1F300 1F308 1F302 1F303 1F304 1F305 1F306
1F307 1F309 1F30A 1F30B 1F30C 1F30D 1F30E 1F30F 1F310 1F311 1F312 1F313
1F314 1F315 1F316 1F317 1F318 1F319 1F31A 1F31B 1F321 1F324 1F325 1F326
1F327 1F328 1F329 1F32A 1F32B 1F32C 1F32D 1F32E 1F32F 1F330 1F331 1F335
1F336
# check marks, crosses and status signs
2705 274C 2714 2716 2B55 2B1B 2B1C 2757 2753 26B1 26B2 26B3 2B06 2B07
2B05 2B04 2695 2696 270A 270B 2708 2709 2611 2610 2049 203C
# arrows and media controls
1F53A 1F53B 23E9 23EA 23EB 23EC 25B6 23F8 23F9 23FA 1F53C 1F53D 1F504
1F503 1F519 1F51A 1F51B 1F51C 1F51D 1F550 1F551 1F552 1F553 1F516 1F517
1F518 1F52E 1F52F 1F53E 1F53F
# office, paperwork, tech
1F4BB 1F4C1 1F4C2 1F4C3 1F4C4 1F4C5 1F4C6 1F4C7 1F4C8 1F4C9 1F4CA 1F4CB
1F4CC 1F4CD 1F4CE 1F4D1 1F4D2 1F4D3 1F4D4 1F4D5 1F4D6 1F4D7 1F4D8 1F4D9
1F4DA 1F4DB 1F4DC 1F4DD 1F4DE 1F4DF 1F4E0 1F4E1 1F4E2 1F4E3 1F4E4 1F4E5
1F4E6 1F4E7 1F4E8 1F4E9 1F4EA 1F4EB 1F4EC 1F4ED 1F4EE 1F4EF 1F4F0 1F4F7
1F4F1 1F4F2 1F4F3 1F4F4 1F4F5 1F4F6 1F4F8 1F4F9 1F4FA 1F4FB 1F4FC 1F4FD
1F4FE 1F4FF 1F5A5 1F5A8 1F5B1 1F5B2 1F4BE 1F4BF 1F4C0 1F50B 1F50C
# tools, locks, keys
1F527 1F528 1F529 1F52A 1F52B 1F52C 1F52D 1F512 1F513 1F50F 1F510 1F511
1F50D 1F50E 1F6E0 1F6E2 1F6E3 1F6E4 1F6E5 1F6E6 1F6E7 1F6E8 1F6E9 1F6EA
1F6EB 1F6EC 1F6ED 1F6EE 1F6EF 1F6F0 1F6F1 1F6F2 1F6F3 1F6F4 1F6F5 1F6F6
1F6F7 1F6F8 1F6F9 1F6FA 1F9F0 1F9F1 1F9F2 1F9F3 1F9F4 1F9F5 1F9F6 1F9F7
1F9F8 1F9F9 1F9FA 1F9FB 1F9FC 1F9FD 1F9FE 1F9FF
# prohibitions
1F6AB 1F6AD 1F6AF 1F6B1 1F6B2 1F6B3 1F6B7 1F6B8 1F6B9 1F6BA 1F6BC 1F6BF
1F6C1 1F6C2 1F6C3 1F6C5
# plants and flowers
1F338 1F339 1F33A 1F33B 1F33C 1F337 1F332 1F333 1F334 1F340 1F341 1F342
1F343 1F344 1F347 1F348 1F349 1F34A 1F34B 1F34C 1F34D 1F34E 1F34F 1F350
1F351 1F352 1F353 1F3F5
# food and drink
1F345 1F346 1F355 1F356 1F357 1F358 1F359 1F35A 1F35B 1F35C 1F35D 1F35E
1F35F 1F360 1F361 1F362 1F363 1F364 1F365 1F366 1F367 1F368 1F369 1F36A
1F36B 1F36C 1F36D 1F36E 1F36F 1F370 1F371 1F372 1F373 1F374 1F375 1F376
1F377 1F378 1F379 1F37A 1F37B 1F37C 1F37D 1F37E 1F37F 1F380 1F381 1F382
1F383 1F384 1F385 1F386 2615 1F950 1F956 1F968 1F96D 1F96E 1F96F 1F9C0
1F9CB 1F9C9 1F9CA 1F9CC
# animals
1F435 1F412 1F98A 1F99D 1F9A7 1F436 1F431 1F418 1F42D 1F430 1F42F 1F411
1F424 1F423 1F425 1F426 1F427 1F428 1F429 1F42A 1F42B 1F42C 1F437 1F438
1F43A 1F43B 1F43C 1F43D 1F43E 1F43F 1F440 1F441 1F442 1F443 1F444 1F445
1F40F 1F401 1F402 1F403 1F404 1F405 1F406 1F407 1F408 1F409 1F40A 1F40B
1F40C 1F40D 1F40E 1F419 1F41A 1F41B 1F41C 1F41D 1F41E 1F41F 1F420 1F421
1F422 1F42E 1F9A1 1F9A2 1F9A3 1F9A4 1F9A5 1F9A6 1F9A8 1F9A9 1F9AA 1F9AB
1F9AC 1F9AD 1F9AE 1F9AF 1F9B0 1F9B1 1F9B2 1F9B3 1F9B4 1F9B5 1F9B6 1F9B7
1F9B8 1F9B9 1F9BA 1F9BB 1F9BC 1F9BD 1F9BE 1F9BF 1F9C1 1F9C2 1F980 1F981
1F982 1F983 1F984 1F985 1F986 1F987 1F988 1F989 1F990 1F991 1F992 1F993
1F994 1F995 1F996 1F997 1F998 1F999 1F99A 1F99B 1F99C 1F99E 1F99F 1F9A0
# vehicles
1F697 1F698 1F699 1F69A 1F69B 1F69C 1F69D 1F6A0 1F6A1 1F6A2 1F6A3 1F6A4
1F6A5 1F6A6 1F6A7 1F6A8 1F6A9 1F6AA 1F6AB 1F6AC 1F6AD 1F6AE 1F6AF 1F6B0
1F6B1 1F6B2 1F6B3 1F6B4 1F6B5 1F6B6 1F6B7 1F6B8 1F6B9 1F6BA 1F6BB 1F6BC
1F6BE 1F6BF 1F6C0 1F6C1 1F6C2 1F6C3 1F6C4 1F6C5 1F6C6 1F6C7 1F6C8 1F6C9
1F6CA 1F6CB 1F6CC 1F6CD 1F6CE 1F6D1 1F6D2 1F6D5 1F6D6 1F6D7 1F6EB 1F6EC
1F6F4 1F6F9 26F5 231B 23F3 23F2 1F5FF 26F4
# buildings and places
1F3E0 1F3E1 1F3E2 1F3E3 1F3E4 1F3E5 1F3E6 1F3E7 1F3E8 1F3E9 1F3EA 1F3EB
1F3EC 1F3ED 1F3EE 1F3EF 1F3F0 1F492 1F5FC 1F5FD 1F54C 1F3D5 1F3D6 1F3DC
1F3DD 1F3DE 1F3DF 1F3DB 1F3D7 1F3D8 1F3D9 1F3DA
# sport, medals, games
1F3C0 1F3C1 1F3C2 1F3C3 1F3C4 1F3C5 1F3C6 1F3C7 1F3C8 1F3C9 1F3CA 1F3CB
1F3CC 1F3CD 1F3CE 1F3CF 1F3D0 1F3D1 1F3D2 1F3D3 1F3D4 1F3B3 1F3B5 1F3B6
1F3B7 1F3B8 1F3B9 1F3BA 1F3BB 1F3BC 1F3BD 1F3BE 1F3BF 1F94D 1F94E 1F94F
1F3AE 1F3AF 1F3B0 1F3B1 1F3B2 1F3B4 26BD 26BE 1F947 1F948 1F949 1F94A
1F94B 1F94C
# letters in squares and other marks
1F004 1F0CF 1F170 1F171 1F17E 1F18E 24C2 1F191 1F192 1F193 1F194 1F195
1F196 1F197 1F198 1F199 1F19A 1F201 1F202 1F21A 1F22F 1F232 1F235 1F236
1F238 1F239 1F23A 1F250 1F251 1F3F3 1F3F4 1F17C 1F17D 1F17B
"""


# Emoji whose *default* presentation is the monochrome one, but that a
# variation selector (U+FE0F) turns into the colour emoji - a red heart, a
# plane, a pencil.  They are in the atlas too, and the terminal only swaps
# them for their picture when the U+FE0F is there as well; on their own they
# stay monochrome, exactly like in every other terminal.
CURATED_VS16 = """
2764 2763 2708 270F 2702 2709 270C 270D 2712 2714 2716
2753 2757 26A0 2744 2747 2748 2749 274C 274E 2600 2601
2602 2603 2604 26C5 263A 2639 2660 2663 2665 2666 2668
267B 267E 2692 2694 2696 2699 269B 26A1 26AA 26AB 26B0
26BD 26BE 26C4 26C8 26F4 26F5 2934 2935 2194 2195 21A9
21AA 21BB 3030 303D 3297 3299 2618 2620 2622 2623 262E
262F 2640 2642 2611 2612 261E 261F 266B 266C 266D 266F
"""


def parse_curated(text):
    out = []
    for raw in text.splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        for tok in line.split():
            out.append(int(tok, 16))
    return out


def load_emoji_presentation(path):
    """Codepoints whose default presentation is the emoji one."""
    pres = set()
    range_re = re.compile(r"^([0-9A-Fa-f]+)\.\.([0-9A-Fa-f]+)\s*;\s*Emoji_Presentation")
    single_re = re.compile(r"^([0-9A-Fa-f]+)\s*;\s*Emoji_Presentation")
    with open(path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            m = range_re.match(line)
            if m:
                lo, hi = int(m.group(1), 16), int(m.group(2), 16)
                pres.update(range(lo, hi + 1))
                continue
            m = single_re.match(line)
            if m:
                pres.add(int(m.group(1), 16))
    return pres


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font", required=True, help="NotoColorEmoji.ttf (CBDT)")
    ap.add_argument("--emoji-data", required=True, help="emoji-data.txt from unicode.org")
    ap.add_argument("--out-bin", required=True)
    ap.add_argument("--out-header", required=True)
    ap.add_argument("--cell", type=int, default=32, help="pixels per side of one emoji")
    ap.add_argument("--cols", type=int, default=16)
    ap.add_argument("--count", type=int, default=256)
    ap.add_argument("--preview", help="also write a PNG preview here")
    args = ap.parse_args()

    from fontTools.ttLib import TTFont
    from PIL import Image, ImageDraw, ImageFont

    presentation = load_emoji_presentation(args.emoji_data)
    tt = TTFont(args.font, lazy=True)
    cmap = set(tt.getBestCmap())

    wanted = parse_curated(CURATED)
    chosen = []
    seen = set()
    for cp in wanted:
        if cp in seen:
            continue
        if cp not in cmap:
            print("skip U+%04X: not in the font" % cp)
            continue
        if cp not in presentation:
            print("skip U+%04X: text presentation by default" % cp)
            continue
        seen.add(cp)
        chosen.append((cp, FLAG_ALWAYS))
        if len(chosen) == args.count:
            break
    if len(chosen) < args.count:
        print("only %d emoji available" % len(chosen))

    # The ones that only turn into emoji when a variation selector follows.
    for cp in parse_curated(CURATED_VS16):
        if cp in seen:
            continue
        if cp not in cmap:
            print("skip U+%04X: not in the font" % cp)
            continue
        seen.add(cp)
        # A few of them are the emoji one on their own after all.
        chosen.append((cp, FLAG_ALWAYS if cp in presentation else FLAG_VS16))

    # Noto Color Emoji is a bitmap font with a single strike; Pillow only
    # accepts the size the strike was authored at.
    probe = ImageFont.truetype(args.font, 109)
    strike = 136

    cell = args.cell
    cols = args.cols
    rows = (len(chosen) + cols - 1) // cols
    atlas = Image.new("RGBA", (cols * cell, rows * cell), (0, 0, 0, 0))
    for i, (cp, flags) in enumerate(chosen):
        glyph = Image.new("RGBA", (strike, strike), (0, 0, 0, 0))
        ImageDraw.Draw(glyph).text((0, 0), chr(cp), font=probe, embedded_color=True)
        glyph = glyph.resize((cell, cell), Image.LANCZOS)
        atlas.paste(glyph, ((i % cols) * cell, (i // cols) * cell), glyph)

    # raw BGRA, the byte order Qt's Format_ARGB32 and the Screen API use on
    # little endian ARM
    raw = atlas.convert("RGBA").tobytes()
    bgra = bytearray(len(raw))
    bgra[0::4] = raw[2::4]
    bgra[1::4] = raw[1::4]
    bgra[2::4] = raw[0::4]
    bgra[3::4] = raw[3::4]
    blob = bytes(bgra)

    os.makedirs(os.path.dirname(os.path.abspath(args.out_bin)), exist_ok=True)
    with open(args.out_bin, "wb") as fh:
        fh.write(blob)

    if args.preview:
        atlas.save(args.preview)

    entries = sorted((cp, i, flags) for i, (cp, flags) in enumerate(chosen))
    with open(args.out_header, "w", encoding="utf-8") as fh:
        fh.write("/* Generated by tools/make-emoji-atlas.py - do not edit. */\n")
        fh.write("#ifndef EMOJI_ATLAS_DATA_H\n#define EMOJI_ATLAS_DATA_H\n\n")
        fh.write("#define EMOJI_ATLAS_COUNT %d\n" % len(chosen))
        fh.write("#define EMOJI_ATLAS_CELL %d\n" % cell)
        fh.write("#define EMOJI_ATLAS_COLS %d\n" % cols)
        fh.write("#define EMOJI_ATLAS_ROWS %d\n" % rows)
        fh.write("#define EMOJI_ATLAS_WIDTH %d\n" % (cols * cell))
        fh.write("#define EMOJI_ATLAS_HEIGHT %d\n" % (rows * cell))
        fh.write("/* Private use area slot that stands in for one emoji on screen. */\n")
        fh.write("#define EMOJI_ATLAS_PUA_BASE 0xE000\n")
        fh.write("#define EMOJI_ATLAS_PUA_END 0x%04X\n\n" % (0xE000 + len(chosen)))
        fh.write("/* the emoji is the colour one on its own */\n")
        fh.write("#define EMOJI_ATLAS_ALWAYS 1\n")
        fh.write("/* the emoji is the colour one only after a variation selector */\n")
        fh.write("#define EMOJI_ATLAS_VS16   2\n\n")
        fh.write("typedef struct {\n"
                 "    unsigned int  codepoint;\n"
                 "    unsigned short index;\n"
                 "    unsigned short flags;\n"
                 "} emoji_atlas_entry_t;\n\n")
        fh.write("/* sorted by codepoint, for a binary search */\n")
        fh.write("static const emoji_atlas_entry_t emoji_atlas_entries[] = {\n")
        for cp, idx, flags in entries:
            fh.write("    { 0x%04X, %d, %d },\n" % (cp, idx, flags))
        fh.write("};\n\n")
        fh.write("/* the code point of every atlas cell, indexed by cell */\n")
        fh.write("static const unsigned int emoji_atlas_codepoints[] = {\n")
        for i in range(len(chosen)):
            fh.write("    0x%04X,\n" % chosen[i][0])
        fh.write("};\n\n#endif\n")

    always = sum(1 for c, f in chosen if f == FLAG_ALWAYS)
    print("emoji     : %d (%d always, %d with a variation selector)"
          % (len(chosen), always, len(chosen) - always))
    print("atlas     : %dx%d px, cell %d, grid %dx%d" % (cols * cell, rows * cell, cell, cols, rows))
    print("raw       : %.2f MB -> %s" % (len(blob) / 1048576.0, args.out_bin))
    print("header    : %s" % args.out_header)


if __name__ == "__main__":
    sys.exit(main())
