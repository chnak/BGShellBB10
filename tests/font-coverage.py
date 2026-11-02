from fontTools.ttLib import TTFont

f = TTFont('assets/bgscjk.ttf')
cmap = f.getBestCmap()
chars = ['x', 'X', '\u00d7', '\u2715', '\u00b7', '\u2014', '\u2264', '\u2026',
         '\u2236', '\u2022', '\u2460', '\u2191', '\uff0d', '\u25cf', '\u2192']
for ch in chars:
    print(repr(ch), hex(ord(ch)), ord(ch) in cmap)
print('glyphs in cmap:', len(cmap))
