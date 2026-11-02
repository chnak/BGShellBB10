/*
 * One look for every surface: system menu, settings, command panel, dialogs.
 *
 * The app talks to a shell, so the chrome imitates a good terminal theme:
 * near-black backdrop, one cool surface tone, thin borders, a single green
 * accent and a muted grey for secondary text. Everything else is spacing.
 *
 * Qt 4.8 / BB10 notes: the sheet below sticks to properties the Qt style
 * sheet engine really supports on this platform (no box-shadow, no
 * selectors that need extra pseudo states). Widgets that paint themselves
 * (TerminalDisplay, ImePanel, SymbolPanel) are deliberately not targeted.
 */
#ifndef UITHEME_H
#define UITHEME_H

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtCore/QString>
#include <QtCore/QPoint>
#include <QtCore/QBasicTimer>

class QApplication;
class QMenu;
class QDialog;
class QWidget;
class QVBoxLayout;
class QScrollArea;
class QFrame;
class QPainter;
class QPaintEvent;
class QTimerEvent;

namespace UiTheme {

// palette ---------------------------------------------------------------
extern const QColor Bg;        // page backdrop, also the terminal default bg
extern const QColor Surface;   // cards, menus, dialogs
extern const QColor Surface2;  // inputs, list rows
extern const QColor Border;
extern const QColor Text;
extern const QColor TextDim;
extern const QColor Accent;    // green: primary actions, selection
extern const QColor AccentInk; // readable text on top of Accent
extern const QColor Danger;
extern const QColor Warn;

// Lift and pressure -----------------------------------------------------
// Hi is the one pixel line along the top edge of a raised surface - it is
// what makes a keycap read as a keycap instead of a rectangle.  Sunken is
// the inside of a track or an input well.  Both come from the palette above
// so the key strip, the candidate strip and the symbol sheet keep one hue.
extern const QColor Hi;
extern const QColor Sunken;

// helpers ---------------------------------------------------------------
QString hex(const QColor &c);

// Linear mix: 0 gives base, 100 gives over.  Qt 4 ships no blending helper
// and the style sheet engine cannot express "accent at 20 percent", so the
// panels compute their pressed colours here instead of each carrying a
// second hand-picked green (they had drifted to #00ff66, rgb(0,255,102) and
// #005020 - three different greens for the same interaction).
QColor blend(const QColor &base, const QColor &over, int percentOver);
QColor withAlpha(const QColor &c, int alpha);
QFont monoFont(int pixelSize, bool bold = false);
QFont uiFont(int pixelSize, bool bold = false);

// Install the application font and the global sheet.
void apply(QApplication &app);

// Rounded, padded long-press menu (a plain QMenu looks like 2003).
void styleMenu(QMenu *menu);

// A "card": rounded surface + a title line, with a body layout for the
// caller to fill. `subtitle` may be empty.
QFrame *newCard(QWidget *parent, const QString &title,
                const QString &subtitle, QVBoxLayout *&body);

// Small helpers used by the settings window and the command panel.
QWidget *newHeader(QWidget *parent, const QString &title, const QString &subtitle);

// Put a scrolling area on the backdrop colour. Call it *after* setWidget():
// that call flips autoFillBackground() back on, which is what painted the
// host-editor form paper-white on BB10.
void styleScrollArea(QScrollArea *area, const QColor &fill = Bg);
QString titleStyle();
QString subtitleStyle();
QString sectionStyle();

// A QScrollArea that a thumb can actually use on BB10.
//
// The stock widget only scrolls through its scrollbar or a wheel; the bar in
// the sheet is 12 px wide, which is unusable on glass, and the drag itself
// does nothing because the platform delivers touches as mouse events and
// QScrollArea ignores them.  This subclass drags the content, accepts the
// arrow / page keys, and forces a full repaint on every step - the default
// minimal update mode blits the old pixels sideways, and because the scrolled
// widget is transparent the text is never cleared, which is the smearing seen
// while dragging the bar on the ssh form.
class ScrollPane : public QScrollArea
{
public:
	explicit ScrollPane(QWidget *parent = 0);

	// Colour the bar repaints over itself with.  It no longer lets the style
	// paint the track, so when it narrows to a hairline it has to clear the
	// pixels it vacated - and the sheet makes the area transparent, so the
	// palette is no help there.
	void setBackdrop(const QColor &c);

protected:
	bool eventFilter(QObject *watched, QEvent *event);
	void keyPressEvent(QKeyEvent *event);

private:
	void beginDrag(const QPoint &pos);
	bool dragTo(const QPoint &pos);
	void endDrag();

	bool _press;
	bool _dragging;
	QPoint _down;
	int _lastY;
};

// How much of the scrollbar gets drawn.
//
// The bar keeps its 26 px hit area in every mode - only the ink changes - so
// switching is purely cosmetic and never breaks the drag that was just fixed.
enum ScrollBarMode {
	BarAlways   = 0, // full-width track, always visible
	BarAutoHide = 1, // 5 px hairline, widens while scrolling (default)
	BarHidden   = 2  // nothing drawn; the 26 px strip still accepts a drag
};

void setScrollBarMode(ScrollBarMode mode);
ScrollBarMode scrollBarMode();
ScrollBarMode scrollBarModeFromConfig(); // reads QSettings key "ScrollBar"

// A scrollbar that behaves like a finger expects.
//
// Qt's own bar does nothing when the track is pressed - it pages once and
// waits, and the slider is only grabbed if the press lands on it exactly.
// On glass the slider is a narrow strip and the press never lands, so the
// bar feels dead.  Here a press anywhere on the bar jumps the slider to that
// position and keeps it under the finger until release.
class ScrollBar : public QScrollBar
{
public:
	explicit ScrollBar(Qt::Orientation orientation, QWidget *parent = 0);

	void setBackdrop(const QColor &c);

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void sliderChange(SliderChange change);
	void timerEvent(QTimerEvent *event);

private:
	int valueForPos(int y) const;
	// Pure geometry, shared by the painter and the drag mapping, so the ink
	// always sits where the finger expects it.  Does not ask the style.
	void handleGeometry(QRect &handle, int &trackTop, int &trackSpan) const;
	void paintHandle(QPainter &p, bool wide);
	void kick(); // widen the bar and restart the idle timer

	QColor _backdrop;
	bool _drag;
	bool _thin;   // auto-hide: currently drawn as a hairline
	int _lastValue;
	QBasicTimer _idle;
};

} // namespace UiTheme

#endif // UITHEME_H
