#ifndef _COLOR_TABLE_H
#define _COLOR_TABLE_H

#include "CharacterColor.h"

using namespace Konsole;

static const ColorEntry whiteonblack_color_table[TABLE_COLORS] =
{
    // normal
    ColorEntry(QColor(0xFF,0xFF,0xFF), 0, 0 ), ColorEntry( QColor(0x00,0x00,0x00), 1, 0 ), // Dfore, Dback
    ColorEntry(QColor(0x00,0x00,0x00), 0, 0 ), ColorEntry( QColor(0xB2,0x18,0x18), 0, 0 ), // Black, Red
    ColorEntry(QColor(0x18,0xB2,0x18), 0, 0 ), ColorEntry( QColor(0xB2,0x68,0x18), 0, 0 ), // Green, Yellow
    ColorEntry(QColor(0x18,0x18,0xB2), 0, 0 ), ColorEntry( QColor(0xB2,0x18,0xB2), 0, 0 ), // Blue, Magenta
    ColorEntry(QColor(0x18,0xB2,0xB2), 0, 0 ), ColorEntry( QColor(0xB2,0xB2,0xB2), 0, 0 ), // Cyan, White
    // intensiv
    ColorEntry(QColor(0x00,0x00,0x00), 0, 1 ), ColorEntry( QColor(0xFF,0xFF,0xFF), 1, 0 ),
    ColorEntry(QColor(0x68,0x68,0x68), 0, 0 ), ColorEntry( QColor(0xFF,0x54,0x54), 0, 0 ),
    ColorEntry(QColor(0x54,0xFF,0x54), 0, 0 ), ColorEntry( QColor(0xFF,0xFF,0x54), 0, 0 ),
    ColorEntry(QColor(0x54,0x54,0xFF), 0, 0 ), ColorEntry( QColor(0xFF,0x54,0xFF), 0, 0 ),
    ColorEntry(QColor(0x54,0xFF,0xFF), 0, 0 ), ColorEntry( QColor(0xFF,0xFF,0xFF), 0, 0 )
};

static const ColorEntry greenonblack_color_table[TABLE_COLORS] =
{
    ColorEntry(QColor(    24, 240,  24),  0, 0), ColorEntry(QColor(     0,   0,   0),  1, 0),  
    ColorEntry(QColor(     0,   0,   0),  0, 0), ColorEntry(QColor(   178,  24,  24),  0, 0), 
    ColorEntry(QColor(    24, 178,  24),  0, 0), ColorEntry(QColor(   178, 104,  24),  0, 0), 
    ColorEntry(QColor(    24,  24, 178),  0, 0), ColorEntry(QColor(   178,  24, 178),  0, 0), 
    ColorEntry(QColor(    24, 178, 178),  0, 0), ColorEntry(QColor(   178, 178, 178),  0, 0), 
    // intensive colors
    ColorEntry(QColor(   24, 240,  24),  0, 1 ), ColorEntry(QColor(    0,   0,   0),  1, 0 ),
    ColorEntry(QColor(  104, 104, 104),  0, 0 ), ColorEntry(QColor(  255,  84,  84),  0, 0 ),
    ColorEntry(QColor(   84, 255,  84),  0, 0 ), ColorEntry(QColor(  255, 255,  84),  0, 0 ),
    ColorEntry(QColor(   84,  84, 255),  0, 0 ), ColorEntry(QColor(  255,  84, 255),  0, 0 ),
    ColorEntry(QColor(   84, 255, 255),  0, 0 ), ColorEntry(QColor(  255, 255, 255),  0, 0 )
};

// "Midnight" - the app's own scheme: one cool dark backdrop, a green that
// matches the UI accent for the cursor/prompt side, and an ANSI palette that
// stays readable on a 720x720 screen under sunlight.
static const ColorEntry bgshell_color_table[TABLE_COLORS] =
{
    // default foreground / background
    ColorEntry(QColor(0xD7,0xE0,0xEA), 0, 0 ), ColorEntry( QColor(0x0B,0x0E,0x11), 1, 0 ),
    // normal: black, red, green, yellow, blue, magenta, cyan, white
    ColorEntry(QColor(0x26,0x2E,0x36), 0, 0 ), ColorEntry( QColor(0xFF,0x6B,0x6B), 0, 0 ),
    ColorEntry(QColor(0x3F,0xD9,0x7F), 0, 0 ), ColorEntry( QColor(0xE3,0xB3,0x41), 0, 0 ),
    ColorEntry(QColor(0x6A,0xA8,0xFF), 0, 0 ), ColorEntry( QColor(0xC7,0x92,0xEA), 0, 0 ),
    ColorEntry(QColor(0x5F,0xD7,0xD7), 0, 0 ), ColorEntry( QColor(0xC9,0xD4,0xDE), 0, 0 ),
    // intensiv: brighter, still low glare
    ColorEntry(QColor(0xFF,0xFF,0xFF), 0, 1 ), ColorEntry( QColor(0x0B,0x0E,0x11), 1, 0 ),
    ColorEntry(QColor(0x5A,0x66,0x73), 0, 0 ), ColorEntry( QColor(0xFF,0x94,0x94), 0, 0 ),
    ColorEntry(QColor(0x6F,0xF0,0xA4), 0, 0 ), ColorEntry( QColor(0xF2,0xCC,0x60), 0, 0 ),
    ColorEntry(QColor(0x8F,0xC2,0xFF), 0, 0 ), ColorEntry( QColor(0xDB,0xB4,0xF5), 0, 0 ),
    ColorEntry(QColor(0x8F,0xE6,0xE6), 0, 0 ), ColorEntry( QColor(0xFF,0xFF,0xFF), 0, 0 )
};

static const ColorEntry blackonlightyellow_color_table[TABLE_COLORS] = 
{
    ColorEntry(QColor(  0,   0,   0),  0, 0),  ColorEntry(QColor( 255, 255, 221),  1, 0),  
    ColorEntry(QColor(  0,   0,   0),  0, 0),  ColorEntry(QColor( 178,  24,  24),  0, 0),  
    ColorEntry(QColor( 24, 178,  24),  0, 0),  ColorEntry(QColor( 178, 104,  24),  0, 0),  
    ColorEntry(QColor( 24,  24, 178),  0, 0),  ColorEntry(QColor( 178,  24, 178),  0, 0),  
    ColorEntry(QColor( 24, 178, 178),  0, 0),  ColorEntry(QColor( 178, 178, 178),  0, 0),  
    ColorEntry(QColor(  0,   0,   0),  0, 1),  ColorEntry(QColor( 255, 255, 221),  1, 0),  
    ColorEntry(QColor(104, 104, 104),  0, 0),  ColorEntry(QColor( 255,  84,  84),  0, 0),  
    ColorEntry(QColor( 84, 255,  84),  0, 0),  ColorEntry(QColor( 255, 255,  84),  0, 0),  
    ColorEntry(QColor( 84,  84, 255),  0, 0),  ColorEntry(QColor( 255,  84, 255),  0, 0),  
    ColorEntry(QColor( 84, 255, 255),  0, 0),  ColorEntry(QColor( 255, 255, 255),  0, 0)
};
 			  
			  
			  


#endif

