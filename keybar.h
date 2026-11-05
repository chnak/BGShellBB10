/*
 * The custom key strip for BGShellBB10.
 *
 * A row of user defined buttons along the bottom edge of the terminal. Each
 * button sends a fixed byte sequence to the pty, so anything a missing key
 * would have typed becomes one tap away: Esc, Tab, Ctrl-C, Ctrl-Z, the arrow
 * keys, PgUp/PgDn, or a whole command line like "git status\r".
 *
 *   [Esc] [Tab] [^C] [^D] [^Z] [^L] [PgUp] [PgDn] [<] 1/2 [>]
 *
 * A terminal cannot fake a modifier chord - Ctrl-C is the byte 0x03 and nothing
 * else - so the payload is always raw bytes, stored in QSettings as an escaped
 * text ("\x03", "\e[A", "git status\r") that can be edited on the phone.
 *
 * Touch phones (Q30 / Passport / Classic) tap a button; the QWERTY phones
 * (Q10 / Q20 / Q5) have no touchscreen, so the trackpad steps a highlight and
 * a trackpad click fires it - the same driving the candidate strip uses.
 */
#ifndef KEYBAR_H
#define KEYBAR_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QBasicTimer>
#include <QtCore/QTime>
#include <QtGui/QFont>
#include <QtGui/QWidget>

class CKeyBar : public QWidget
{
	Q_OBJECT

public:
	struct Item {
		QString    label;
		QByteArray payload;

		Item() {}
		Item(const QString &l, const QByteArray &p) : label(l), payload(p) {}
	};

	explicit CKeyBar(QWidget *parent = 0);

	// Height the terminal has to keep free below its last row.
	int stripHeight() const;

	void setItems(const QList<Item> &items);
	QList<Item> items() const { return _items; }

	void popup();
	void dismiss();
	bool isOpen() const { return isVisible(); }

	// Trackpad driving: delta = +1 next, -1 previous, paced like a held key.
	void moveHighlight(int delta);
	void commitHighlighted();
	// True only while the trackpad highlight is "live" (moved within the last
	// couple of seconds). Without this every trackpad click on the bottom edge
	// would fire a button instead of acting as a click.
	bool hasHighlight() const;

	// The menu of keys the settings screen offers as check boxes, most used
	// first. defaultItems() is just the first defaultCount() entries, so the
	// strip of a fresh install and the menu stay in step.
	static QList<Item> pool();
	static int         defaultCount();
	static QList<Item> defaultItems();
	static QList<Item> loadItems();
	static void        storeItems(const QList<Item> &items);
	static QString     encodePayload(const QByteArray &bytes);
	static QByteArray  decodePayload(const QString &text);
	static QList<Item> itemsFromStrings(const QStringList &list);
	static QStringList itemsToStrings(const QList<Item> &items);

signals:
	void keyClicked(const QByteArray &bytes);

protected:
	void paintEvent(QPaintEvent *event);
	// Qt hands the second press of a fast double tap over as a DblClick event,
	// never as another MouseButtonPress. A cap tapped twice in a row therefore
	// fired once and then stayed silent - which on the PgUp/PgDn caps looks
	// exactly like "it only works a few times". Both entry points run the
	// same code.
	void mousePressEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	void relayout();
	void clampHighlight();

	int  itemAt(const QPoint &pos) const;
	void fireItem(int item);                        // press feedback + payload
	static bool repeatsWhileHeld(const QByteArray &payload);

	QList<Item>  _items;
	QList<QRect> _rects;      // one rect per visible item
	QList<int>   _rectItems;  // index into _items for each rect
	QRect        _prevRect;
	QRect        _nextRect;
	int          _first;      // index of the first item on this page
	int          _highlight;  // index into _items
	QTime        _lastStep;
	int          _pressItem;  // cap currently held down, -1 when idle
	QBasicTimer  _flash;      // ends the press state after a moment

	// A finger resting on a page / arrow cap keeps firing it. Only escape
	// sequences repeat: a cap that types a whole command line must not run it
	// ten times because the finger stayed on the glass a little too long.
	int          _repeatItem;  // cap held for auto-repeat, -1 when idle
	bool         _repeating;   // past the initial delay
	QBasicTimer  _repeat;

	QFont _labelFont;
};

#endif // KEYBAR_H
