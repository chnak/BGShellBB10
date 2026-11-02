#include "keybar.h"

#include <QtCore/QSettings>
#include <QtCore/QTimerEvent>
#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>

#include "uitheme.h"

namespace {
const int kBarHeight  = 52;
const int kMargin     = 6;
const int kCellHeight = 32;
const int kGap        = 6;
const int kMinCell    = 40;   // a one letter button still has to be tapable
const int kPageZone   = 64;   // "< n/m >" on the right when pages exist

// A held page / arrow cap repeats, the way a held key does.
const int kRepeatDelay    = 550;   // hold this long before the first repeat
const int kRepeatInterval = 110;   // then keep firing at about key speed
}

CKeyBar::CKeyBar(QWidget *parent)
	: QWidget(parent)
	, _first(0)
	, _highlight(0)
	, _pressItem(-1)
	, _repeatItem(-1)
	, _repeating(false)
{
	setFocusPolicy(Qt::NoFocus);
	setVisible(false);
	_labelFont.setPixelSize(15);
}

int CKeyBar::stripHeight() const
{
	return kBarHeight;
}

void CKeyBar::setItems(const QList<Item> &items)
{
	_items = items;
	if (_highlight >= _items.size())
		_highlight = 0;
	if (_first >= _items.size())
		_first = 0;
	relayout();
	update();
}

void CKeyBar::popup()
{
	if (!parentWidget())
		return;
	const int w = parentWidget()->width();
	const QRect target(0, parentWidget()->height() - kBarHeight, w, kBarHeight);
	if (geometry() != target) {
		setGeometry(target);
		relayout();
	}
	show();
	raise();
}

void CKeyBar::dismiss()
{
	hide();
}

void CKeyBar::relayout()
{
	_rects.clear();
	_rectItems.clear();
	_prevRect = QRect();
	_nextRect = QRect();

	if (_items.isEmpty() || width() <= 0)
		return;

	if (_first < 0 || _first >= _items.size())
		_first = 0;
	if (_highlight < 0 || _highlight >= _items.size())
		_highlight = 0;

	const QFontMetrics fm(_labelFont);
	const int top = (kBarHeight - kCellHeight) / 2;

	// First pass: assume no paging zone. If everything fits, keep it that way.
	int right = width() - kMargin;
	int x = kMargin;
	int i = _first;
	QList<QRect> rects;
	QList<int>   owners;
	while (i < _items.size()) {
		const int w = qMax(kMinCell, fm.width(_items.at(i).label) + 18);
		if (x + w > right)
			break;
		rects.append(QRect(x, top, w, kCellHeight));
		owners.append(i);
		x += w + kGap;
		++i;
	}

	if (i < _items.size()) {
		// More items than fit: reserve the page zone and lay out again.
		right = width() - kMargin - kPageZone;
		x = kMargin;
		i = _first;
		rects.clear();
		owners.clear();
		while (i < _items.size()) {
			const int w = qMax(kMinCell, fm.width(_items.at(i).label) + 18);
			if (x + w > right)
				break;
			rects.append(QRect(x, top, w, kCellHeight));
			owners.append(i);
			x += w + kGap;
			++i;
		}
		_nextRect = QRect(width() - kMargin - 22, top, 22, kCellHeight);
		_prevRect = QRect(_nextRect.left() - 26, top, 22, kCellHeight);
	}

	_rects = rects;
	_rectItems = owners;
}

void CKeyBar::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	const QRect r = rect();

	// A panel, not a neon strip: same surface as the tab bar above.  The top
	// edge fades out of the terminal backdrop instead of drawing a hard rule:
	// a one pixel line across the screen right above the last line of text is
	// what made the strip feel like it was pressing on the content.  The fade
	// occupies exactly the gap above the keycaps.
	p.fillRect(r, UiTheme::Surface);
	{
		const int fade = (kBarHeight - kCellHeight) / 2;
		QLinearGradient g(0, r.top(), 0, r.top() + fade);
		g.setColorAt(0.0, UiTheme::Bg);
		g.setColorAt(1.0, UiTheme::Surface);
		p.fillRect(QRect(r.left(), r.top(), r.width(), fade), QBrush(g));
	}

	p.setFont(_labelFont);

	if (_items.isEmpty()) {
		p.setPen(UiTheme::TextDim);
		p.drawText(r.adjusted(kMargin, 0, -kMargin, 0),
		           Qt::AlignVCenter | Qt::AlignLeft,
		           QString::fromUtf8("按键条为空 · 设置 → 按键条 里添加"));
		return;
	}

	for (int k = 0; k < _rects.size(); ++k) {
		const QRect cell = _rects.at(k);
		const int item = _rectItems.at(k);
		const bool down = (item == _pressItem) || (item == _repeatItem);
		// The trackpad highlight is a focus ring, not a press: outline only, so
		// the two states cannot be mistaken for each other at a glance.
		const bool focus = (item == _highlight) && hasHighlight();

		const QRectF body(cell.x(), cell.y(),
		                  cell.width() - 1, cell.height() - 1);

		QColor fill = UiTheme::Surface2;
		QColor edge = UiTheme::Border;
		if (focus)
			edge = UiTheme::blend(UiTheme::Border, UiTheme::Accent, 55);
		if (down) {
			fill = UiTheme::blend(UiTheme::Surface2, UiTheme::Accent, 26);
			edge = UiTheme::Accent;
		}

		p.setPen(Qt::NoPen);
		p.setBrush(fill);
		p.drawRoundedRect(body, 8, 8);
		p.setBrush(Qt::NoBrush);

		// The top highlight disappears while pressed - that flattening is the
		// entire reason a keycap looks raised.
		if (!down) {
			p.setPen(QPen(UiTheme::Hi, 1));
			p.drawLine(QPointF(body.left() + 8, body.top() + 1),
			           QPointF(body.right() - 8, body.top() + 1));
		}

		p.setPen(QPen(edge, 1));
		p.drawRoundedRect(body, 8, 8);

		p.setPen(down ? UiTheme::Accent : UiTheme::Text);
		p.drawText(cell, Qt::AlignCenter, _items.at(item).label);
	}

	if (_prevRect.isValid()) {
		p.setPen(QPen(UiTheme::TextDim, 1));
		p.drawText(_prevRect, Qt::AlignCenter, QString("<"));
		p.drawText(_nextRect, Qt::AlignCenter, QString(">"));

		int pages = 1, page = 1;
		// how many items this page holds, to show "1/2" honestly
		const int shown = _rectItems.isEmpty() ? 0
		                : (_rectItems.last() - _first + 1);
		if (shown > 0)
			pages = (_items.size() + shown - 1) / shown;
		if (pages < 1)
			pages = 1;
		page = _first / qMax(1, shown) + 1;
		p.setPen(UiTheme::withAlpha(UiTheme::TextDim, 150));
		p.drawText(QRect(_prevRect.right() + 4, _prevRect.top(),
		                 30, kCellHeight),
		           Qt::AlignCenter, QString("%1/%2").arg(page).arg(pages));
	}
}

// Which cap is under this point? -1 when the point misses all of them; the
// paging arrows and the empty margin are handled by the caller.
int CKeyBar::itemAt(const QPoint &pos) const
{
	for (int k = 0; k < _rects.size(); ++k) {
		if (_rects.at(k).contains(pos))
			return _rectItems.at(k);
	}
	return -1;
}

// Escape sequences - arrows, PgUp / PgDn, Home / End - are safe to repeat
// while the finger stays down. Anything longer, or printable, is a snippet:
// firing "git status\r" ten times because the finger rested too long would be
// a disaster, so only escape sequences get auto-repeat.
bool CKeyBar::repeatsWhileHeld(const QByteArray &payload)
{
	if (payload.isEmpty() || payload.size() > 8)
		return false;

	const char c = payload.at(0);

	// Editing keys people naturally hold down.  Backspace in particular was
	// dead before: it is a single 0x7f byte, so the old "must start with ESC"
	// rule excluded it and holding the key deleted exactly one character.
	if (payload.size() == 1
	    && (c == '\x7f' || c == '\x08' || c == '\x09' || c == '\r' || c == '\n'))
		return true;

	// Anything that starts an escape sequence: arrows, Home/End, PgUp/PgDn.
	return c == '\x1b' && payload.size() >= 2;
}

void CKeyBar::fireItem(int item)
{
	if (item < 0 || item >= _items.size())
		return;

	_highlight = item;
	_pressItem = item;
	_lastStep  = QTime::currentTime();   // keep the focus ring lit while held
	_flash.start(110, this);
	update();

	emit keyClicked(_items.at(item).payload);

	if (repeatsWhileHeld(_items.at(item).payload)) {
		_repeatItem = item;
		_repeating  = false;
		_repeat.start(kRepeatDelay, this);
	}
}

void CKeyBar::mousePressEvent(QMouseEvent *event)
{
	const QPoint pos = event->pos();
	const int item = itemAt(pos);

	if (item >= 0) {
		fireItem(item);
		event->accept();
		return;
	}

	if (_prevRect.isValid() && _prevRect.contains(pos)) {
		_first = qMax(0, _first - _rects.size());
		relayout();
		update();
		event->accept();
		return;
	}
	if (_nextRect.isValid() && _nextRect.contains(pos)) {
		if (_first + _rects.size() < _items.size()) {
			_first += _rects.size();
			relayout();
			update();
		}
		event->accept();
		return;
	}

	event->ignore();
}

// 110 ms is long enough to see on glass, short enough that a held finger
// cannot leave a cap stuck down.
void CKeyBar::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == _flash.timerId()) {
		_flash.stop();
		_pressItem = -1;
		update();
		return;
	}

	if (event->timerId() == _repeat.timerId()) {
		if (!_repeating) {
			_repeating = true;
			_repeat.start(kRepeatInterval, this);
		}
		if (_repeatItem >= 0 && _repeatItem < _items.size()) {
			emit keyClicked(_items.at(_repeatItem).payload);
			update();
		}
		return;
	}

	QWidget::timerEvent(event);
}

// Qt sends a DblClick instead of a second MouseButtonPress when two taps land
// on the same widget inside the double-click interval. The strip has no double
// click meaning, so treat it as one more press - otherwise every second tap on
// PgDn is lost.
void CKeyBar::mouseDoubleClickEvent(QMouseEvent *event)
{
	mousePressEvent(event);
}

void CKeyBar::mouseReleaseEvent(QMouseEvent *event)
{
	_repeat.stop();
	_repeating  = false;
	_repeatItem = -1;
	update();
	QWidget::mouseReleaseEvent(event);
}

void CKeyBar::clampHighlight()
{
	if (_items.isEmpty()) {
		_highlight = 0;
		return;
	}
	if (_highlight < 0)
		_highlight = _items.size() - 1;
	if (_highlight >= _items.size())
		_highlight = 0;

	// keep the highlight on the page that is drawn
	if (_rectItems.isEmpty())
		return;
	if (_highlight < _rectItems.first()) {
		_first = _highlight;
		relayout();
	} else if (_highlight > _rectItems.last()) {
		_first = _highlight;
		relayout();
		if (_rectItems.size() > 1)
			_first = qMax(0, _highlight - _rectItems.size() + 1);
		relayout();
	}
}

void CKeyBar::moveHighlight(int delta)
{
	if (_items.isEmpty() || delta == 0)
		return;

	const QTime now = QTime::currentTime();
	if (_lastStep.isValid() && _lastStep.msecsTo(now) < 80)
		return;
	_lastStep = now;

	_highlight += delta;
	clampHighlight();
	update();
}

void CKeyBar::commitHighlighted()
{
	if (_items.isEmpty() || _highlight < 0 || _highlight >= _items.size())
		return;
	emit keyClicked(_items.at(_highlight).payload);
}

bool CKeyBar::hasHighlight() const
{
	if (_items.isEmpty() || !_lastStep.isValid())
		return false;
	return _lastStep.msecsTo(QTime::currentTime()) < 2000;
}

/* ------------------------------------------------------------------------- */
/*                          storage  /  escaping                             */
/* ------------------------------------------------------------------------- */

QString CKeyBar::encodePayload(const QByteArray &bytes)
{
	QString out;
	for (int i = 0; i < bytes.size(); ++i) {
		const unsigned char c = (unsigned char)bytes.at(i);
		switch (c) {
		case '\\': out += QLatin1String("\\\\"); break;
		case 0x1b: out += QLatin1String("\\e");  break;
		case '\r': out += QLatin1String("\\r");  break;
		case '\n': out += QLatin1String("\\n");  break;
		case '\t': out += QLatin1String("\\t");  break;
		default:
			if (c >= 0x20 && c < 0x7f) {
				out += QChar((ushort)c);
			} else {
				out += QLatin1String("\\x");
				out += QString::number(c, 16).rightJustified(2, QChar('0'));
			}
			break;
		}
	}
	return out;
}

QByteArray CKeyBar::decodePayload(const QString &text)
{
	QByteArray out;
	for (int i = 0; i < text.size(); ++i) {
		const QChar ch = text.at(i);
		if (ch == QLatin1Char('\\') && i + 1 < text.size()) {
			const QChar n = text.at(i + 1);
			if (n == QLatin1Char('e') || n == QLatin1Char('E')) {
				out.append('\x1b'); i += 1; continue;
			}
			if (n == QLatin1Char('r')) { out.append('\r'); i += 1; continue; }
			if (n == QLatin1Char('n')) { out.append('\n'); i += 1; continue; }
			if (n == QLatin1Char('t')) { out.append('\t'); i += 1; continue; }
			if (n == QLatin1Char('0')) { out.append('\0'); i += 1; continue; }
			if (n == QLatin1Char('\\')) { out.append('\\'); i += 1; continue; }
			if (n == QLatin1Char('x') && i + 3 < text.size()) {
				bool ok = false;
				const int v = text.mid(i + 2, 2).toInt(&ok, 16);
				if (ok) { out.append((char)v); i += 3; continue; }
			}
			out.append('\\');
			continue;
		}
		out.append((char)(ch.unicode() & 0xff));
	}
	return out;
}

QList<CKeyBar::Item> CKeyBar::itemsFromStrings(const QStringList &list)
{
	QList<Item> items;
	for (int i = 0; i < list.size(); ++i) {
		const QString entry = list.at(i);
		if (entry.isEmpty())
			continue;
		const int tab = entry.indexOf(QLatin1Char('\t'));
		Item it;
		if (tab < 0) {
			it.label = entry;
			it.payload = QByteArray();
		} else {
			it.label = entry.left(tab);
			it.payload = decodePayload(entry.mid(tab + 1));
		}
		items.append(it);
	}
	return items;
}

QStringList CKeyBar::itemsToStrings(const QList<Item> &items)
{
	QStringList list;
	for (int i = 0; i < items.size(); ++i)
		list << (items.at(i).label + QLatin1Char('\t')
		         + encodePayload(items.at(i).payload));
	return list;
}

// The whole menu the settings screen shows as check boxes, most used first.
// Everything from kDefaultCount on is off until somebody ticks it: the strip is
// only as wide as the phone, and every extra cap pushes the rest onto page two.
static const int kDefaultCount = 12;

QList<CKeyBar::Item> CKeyBar::pool()
{
	QList<Item> items;
	// ---- on by default: what the Q30 keyboard cannot produce at all --------
	items << Item(QString::fromLatin1("Esc"),  QByteArray("\x1b", 1));
	items << Item(QString::fromLatin1("Tab"),  QByteArray("\t", 1));
	items << Item(QString::fromLatin1("^C"),   QByteArray("\x03", 1));
	items << Item(QString::fromLatin1("^D"),   QByteArray("\x04", 1));
	items << Item(QString::fromLatin1("Home"), QByteArray("\x1b[H", 3));
	items << Item(QString::fromLatin1("End"),  QByteArray("\x1b[F", 3));
	items << Item(QString::fromLatin1("PgUp"), QByteArray("\x1b[5~", 5));
	items << Item(QString::fromLatin1("PgDn"), QByteArray("\x1b[6~", 5));
	items << Item(QString::fromLatin1("Up"),   QByteArray("\x1b[A", 3));
	items << Item(QString::fromLatin1("Dn"),   QByteArray("\x1b[B", 3));
	items << Item(QString::fromLatin1("Lt"),   QByteArray("\x1b[D", 3));
	items << Item(QString::fromLatin1("Rt"),   QByteArray("\x1b[C", 3));
	// ---- off by default: useful, but either reachable from the physical
	//      keyboard or niche enough that it should not cost a slot unasked.
	items << Item(QString::fromLatin1("Del"),  QByteArray("\x1b[3~", 4));
	items << Item(QString::fromLatin1("^A"),   QByteArray("\x01", 1));
	items << Item(QString::fromLatin1("^E"),   QByteArray("\x05", 1));
	items << Item(QString::fromLatin1("^K"),   QByteArray("\x0b", 1));
	items << Item(QString::fromLatin1("^U"),   QByteArray("\x15", 1));
	items << Item(QString::fromLatin1("^W"),   QByteArray("\x17", 1));
	items << Item(QString::fromLatin1("^R"),   QByteArray("\x12", 1));
	items << Item(QString::fromLatin1("^B"),   QByteArray("\x02", 1));
	items << Item(QString::fromLatin1("^Z"),   QByteArray("\x1a", 1));
	items << Item(QString::fromLatin1("^L"),   QByteArray("\x0c", 1));
	return items;
}

int CKeyBar::defaultCount()
{
	return kDefaultCount;
}

QList<CKeyBar::Item> CKeyBar::defaultItems()
{
	const QList<Item> all = pool();
	QList<Item> items;
	for (int i = 0; i < kDefaultCount && i < all.size(); ++i)
		items << all.at(i);
	return items;
}

// The v3 default list, kept only so loadItems() can recognise a strip the user
// never edited and upgrade it to the trimmed v4 set.
static QStringList legacyV3Defaults()
{
	QList<CKeyBar::Item> items;
	items << CKeyBar::Item(QString::fromLatin1("Esc"),  QByteArray("\x1b", 1));
	items << CKeyBar::Item(QString::fromLatin1("Tab"),  QByteArray("\t", 1));
	items << CKeyBar::Item(QString::fromLatin1("^B"),   QByteArray("\x02", 1));
	items << CKeyBar::Item(QString::fromLatin1("^C"),   QByteArray("\x03", 1));
	items << CKeyBar::Item(QString::fromLatin1("^D"),   QByteArray("\x04", 1));
	items << CKeyBar::Item(QString::fromLatin1("^Z"),   QByteArray("\x1a", 1));
	items << CKeyBar::Item(QString::fromLatin1("^L"),   QByteArray("\x0c", 1));
	items << CKeyBar::Item(QString::fromLatin1("Home"), QByteArray("\x1b[H", 3));
	items << CKeyBar::Item(QString::fromLatin1("End"),  QByteArray("\x1b[F", 3));
	items << CKeyBar::Item(QString::fromLatin1("PgUp"), QByteArray("\x1b[5~", 5));
	items << CKeyBar::Item(QString::fromLatin1("PgDn"), QByteArray("\x1b[6~", 5));
	items << CKeyBar::Item(QString::fromLatin1("Up"),   QByteArray("\x1b[A", 3));
	items << CKeyBar::Item(QString::fromLatin1("Dn"),   QByteArray("\x1b[B", 3));
	items << CKeyBar::Item(QString::fromLatin1("Lt"),   QByteArray("\x1b[D", 3));
	items << CKeyBar::Item(QString::fromLatin1("Rt"),   QByteArray("\x1b[C", 3));
	return CKeyBar::itemsToStrings(items);
}

// Bumped whenever defaultItems() gains a key, so a list the user saved with an
// older build is upgraded once instead of silently missing the new button.
#define KEYBAR_DEFAULTS_VERSION 4

QList<CKeyBar::Item> CKeyBar::loadItems()
{
	QSettings settings;
	// KeyBarSet is what tells "the user turned every key off" apart from
	// "nothing has ever been saved", because both leave KeyBarKeys empty.
	if (!settings.contains(QString::fromLatin1("KeyBarSet")))
		return defaultItems();
	const QStringList list =
		settings.value(QString::fromLatin1("KeyBarKeys")).toStringList();
	QList<Item> items = itemsFromStrings(list);

	// v2 added Ctrl-B (the tmux prefix). Only add it when the saved list has no
	// Ctrl-B of its own, so a user who already made one is left alone.
	const int ver = settings.value(QString::fromLatin1("KeyBarVer"), 1).toInt();
	if (ver < 2) {
		bool hasCtrlB = false;
		for (int i = 0; i < items.size(); ++i) {
			if (items.at(i).payload == QByteArray("\x02", 1)) {
				hasCtrlB = true;
				break;
			}
		}
		if (!hasCtrlB) {
			// Right after Tab, where the defaults put it.
			int at = 0;
			for (int i = 0; i < items.size(); ++i) {
				at = i + 1;
				if (items.at(i).payload == QByteArray("\t", 1))
					break;
			}
			items.insert(at, Item(QString::fromLatin1("^B"), QByteArray("\x02", 1)));
			storeItems(items);
		}
		settings.setValue(QString::fromLatin1("KeyBarVer"), KEYBAR_DEFAULTS_VERSION);
	}

	// v3 added Home / End. A terminal without them cannot jump to the start or
	// the end of a line, and the Q30 keyboard has no such keys to fall back on.
	if (ver < 3) {
		bool hasHome = false;
		bool hasEnd  = false;
		for (int i = 0; i < items.size(); ++i) {
			if (items.at(i).payload == QByteArray("\x1b[H", 3))
				hasHome = true;
			if (items.at(i).payload == QByteArray("\x1b[F", 3))
				hasEnd = true;
		}

		if (!hasHome || !hasEnd) {
			// Right in front of PgUp, where the defaults keep them.
			int at = items.size();
			for (int i = 0; i < items.size(); ++i) {
				if (items.at(i).payload == QByteArray("\x1b[5~", 5)) {
					at = i;
					break;
				}
			}
			// Insert End first so Home lands to its left.
			if (!hasEnd)
				items.insert(at, Item(QString::fromLatin1("End"),
				                      QByteArray("\x1b[F", 3)));
			if (!hasHome)
				items.insert(at, Item(QString::fromLatin1("Home"),
				                      QByteArray("\x1b[H", 3)));
			storeItems(items);
		}

			settings.setValue(QString::fromLatin1("KeyBarVer"), KEYBAR_DEFAULTS_VERSION);
	}

	// v4 trimmed the strip: ^B / ^Z / ^L are gone. Only a list that is still
	// exactly the v3 default is replaced - a strip the user edited stays as it
	// is, whatever it holds.
	if (ver < 4 && itemsToStrings(items) == legacyV3Defaults()) {
		items = defaultItems();
		storeItems(items);
	}
	settings.setValue(QString::fromLatin1("KeyBarVer"), KEYBAR_DEFAULTS_VERSION);

	return items;
}

void CKeyBar::storeItems(const QList<Item> &items)
{
	QSettings settings;
	settings.setValue(QString::fromLatin1("KeyBarKeys"), itemsToStrings(items));
	settings.setValue(QString::fromLatin1("KeyBarSet"), true);
	settings.setValue(QString::fromLatin1("KeyBarVer"), KEYBAR_DEFAULTS_VERSION);
}
