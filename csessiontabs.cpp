#include "csessiontabs.h"
#include "uitheme.h"

#include <QtGui>
#include <math.h>

static const int kBarHeight = 46;
static const int kTabGap    = 2;
static const int kPlusWidth = 46;
static const int kGearWidth = 46;
static const int kCloseWide = 34;
static const int kPadH      = 12;
static const int kRadius    = 10;

// Eight-tooth gear, drawn rather than typed: the device fonts have no U+2699.
static void drawGear(QPainter &p, const QPoint &c, const QColor &col)
{
	p.save();
	p.translate(c);
	QPen pen(col);
	pen.setWidth(3);
	pen.setCapStyle(Qt::RoundCap);
	p.setPen(pen);
	for (int i = 0; i < 8; ++i) {
		const double a = 2.0 * 3.14159265358979 * i / 8.0;
		p.drawLine(QPointF(cos(a) * 8.0, sin(a) * 8.0),
		           QPointF(cos(a) * 13.0, sin(a) * 13.0));
	}
	p.drawEllipse(QPointF(0, 0), 7.5, 7.5);
	p.restore();
}

CSessionTabs::CSessionTabs(QWidget *parent)
	: QWidget(parent),
	  _current(-1),
	  _barH(kBarHeight),
	  _scroll(0),
	  _tabW(-1),
	  _pressGear(false),
	  _dragging(false),
	  _moved(false),
	  _pressX(0),
	  _pressScroll(0),
	  _pressIndex(-1),
	  _pressClose(false),
	  _pressPlus(false)
{
	setFocusPolicy(Qt::NoFocus);
	setAutoFillBackground(false);
}

// Space the pages may use (below the strip).
QRect CSessionTabs::contentGeometry() const
{
	return QRect(0, _barH, width(), qMax(0, height() - _barH));
}

// Width left for tabs: the "+" and the gear are pinned to the right edge and
// never scroll, so the tabs have to stop short of them.
int CSessionTabs::viewWidth() const
{
	return qMax(60, width() - kPlusWidth - kGearWidth);
}

int CSessionTabs::tabWidth() const
{
	const int n = _pages.size();
	if (n <= 0)
		return 0;
	if (_tabW > 0)
		return _tabW;

	const int avail = qMax(120, viewWidth() - 8);
	const int share = qMax(120, avail / n);

	// A title like "root@box: /usr/src" must not be chopped down to
	// "roo...".  The strip sizes every tab from the longest title
	// (Chrome widens its tabs evenly too) up to 220 px, but never past
	// the slice each tab is entitled to - beyond that it scrolls.
	const QFontMetrics fm(UiTheme::uiFont(19, false));
	int need = 0;
	for (int i = 0; i < _titles.size(); ++i)
		need = qMax(need, fm.width(_titles.at(i)));
	need += kPadH + kCloseWide + 10;

	int w = need;
	if (w < 132) w = 132;
	if (w > 220) w = 220;
	if (w > share) w = qMax(132, share);

	_tabW = w;
	return _tabW;
}

int CSessionTabs::totalWidth() const
{
	const int n = _pages.size();
	if (n <= 0)
		return 0;
	return n * (tabWidth() + kTabGap);
}

int CSessionTabs::maxScroll() const
{
	return qMax(0, totalWidth() - viewWidth());
}

void CSessionTabs::addPage(QWidget *page, const QString &title)
{
	if (!page)
		return;

	page->setParent(this);
	page->hide();
	_pages << page;
	_titles << title;
	_tabW = -1;

	layoutPages();
	setCurrentIndex(_pages.size() - 1);
	// Keep the fresh tab visible.
	_scroll = maxScroll();
	update();
}

void CSessionTabs::removePage(int index)
{
	if (index < 0 || index >= _pages.size() || _pages.size() <= 1)
		return;

	QWidget *page = _pages.takeAt(index);
	_titles.removeAt(index);
	_tabW = -1;
	page->hide();
	page->deleteLater();

	if (_current >= _pages.size())
		_current = _pages.size() - 1;
	else if (index < _current)
		_current -= 1;

	layoutPages();
	_current = qBound(0, _current, _pages.size() - 1);
	for (int i = 0; i < _pages.size(); ++i)
		_pages.at(i)->setVisible(i == _current);
	_scroll = qBound(0, _scroll, maxScroll());

	emit currentChanged(_current);
	update();
}

QWidget *CSessionTabs::currentPage() const
{
	if (_current < 0 || _current >= _pages.size())
		return 0;
	return _pages.at(_current);
}

QWidget *CSessionTabs::pageAt(int index) const
{
	if (index < 0 || index >= _pages.size())
		return 0;
	return _pages.at(index);
}

void CSessionTabs::setCurrentIndex(int index)
{
	if (index < 0 || index >= _pages.size() || index == _current)
		return;

	_current = index;
	for (int i = 0; i < _pages.size(); ++i)
		_pages.at(i)->setVisible(i == _current);
	if (_pages.at(_current))
		_pages.at(_current)->raise();

	// Scroll the chosen tab into view.
	const int tw = tabWidth() + kTabGap;
	const int left  = _current * tw;
	const int right = left + tw;
	if (left < _scroll)
		_scroll = left;
	else if (right > _scroll + viewWidth())
		_scroll = right - viewWidth();
	_scroll = qBound(0, _scroll, maxScroll());

	emit currentChanged(_current);
	update();
}

void CSessionTabs::setPageTitle(int index, const QString &title)
{
	if (index < 0 || index >= _titles.size())
		return;
	if (_titles.at(index) == title)
		return;
	_titles[index] = title;
	_tabW = -1;             // the longest title drives the tab width

	// A wider title may have pushed the active tab past the edge.
	const int tw = tabWidth() + kTabGap;
	const int left  = _current * tw;
	const int right = left + tw;
	if (left < _scroll)
		_scroll = left;
	else if (right > _scroll + viewWidth())
		_scroll = right - viewWidth();
	_scroll = qBound(0, _scroll, maxScroll());
	update();
}

QString CSessionTabs::pageTitle(int index) const
{
	if (index < 0 || index >= _titles.size())
		return QString();
	return _titles.at(index);
}

void CSessionTabs::refreshMetrics()
{
	_tabW = -1;
	_scroll = qBound(0, _scroll, maxScroll());
	update();
}

void CSessionTabs::layoutPages()
{
	const QRect r = contentGeometry();
	for (int i = 0; i < _pages.size(); ++i)
		_pages.at(i)->setGeometry(r);
}

void CSessionTabs::resizeEvent(QResizeEvent *)
{
	_tabW = -1;
	layoutPages();
	_scroll = qBound(0, _scroll, maxScroll());
	update();
}

bool CSessionTabs::plusHit(const QPoint &pos) const
{
	if (pos.y() >= _barH)
		return false;
	const int x = width() - kPlusWidth - kGearWidth;
	return pos.x() >= x && pos.x() < x + kPlusWidth;
}

bool CSessionTabs::gearHit(const QPoint &pos) const
{
	if (pos.y() >= _barH)
		return false;
	return pos.x() >= width() - kGearWidth;
}

int CSessionTabs::indexAt(const QPoint &pos, bool *closeHit) const
{
	if (closeHit)
		*closeHit = false;
	if (pos.y() >= _barH)
		return -1;

	const int tw = tabWidth();
	int x = -_scroll;

	for (int i = 0; i < _pages.size(); ++i) {
		const QRect r(x, 4, tw, _barH - 4);
		if (r.contains(pos)) {
			if (closeHit && pos.x() >= r.right() - kCloseWide)
				*closeHit = true;
			return i;
		}
		x += tw + kTabGap;
	}
	return -1;
}

void CSessionTabs::mousePressEvent(QMouseEvent *event)
{
	_dragging  = true;
	_moved     = false;
	_pressX    = event->pos().x();
	_pressScroll = _scroll;
	_pressClose = false;
	_pressPlus  = plusHit(event->pos());
	_pressGear  = gearHit(event->pos());
	_pressIndex = indexAt(event->pos(), &_pressClose);
	event->accept();
}

void CSessionTabs::mouseMoveEvent(QMouseEvent *event)
{
	if (!_dragging)
		return;

	const int dx = event->pos().x() - _pressX;
	if (!_moved && qAbs(dx) > 10)
		_moved = true;

	if (_moved) {
		_scroll = qBound(0, _pressScroll - dx, maxScroll());
		update();
	}
	event->accept();
}

void CSessionTabs::mouseReleaseEvent(QMouseEvent *event)
{
	if (!_dragging) {
		event->ignore();
		return;
	}
	_dragging = false;

	if (_moved) {
		event->accept();
		return;
	}

	if (_pressPlus && plusHit(event->pos())) {
		emit newSessionRequested();
		event->accept();
		return;
	}

	if (_pressGear && gearHit(event->pos())) {
		emit settingsRequested();
		event->accept();
		return;
	}

	if (_pressIndex >= 0) {
		if (_pressClose) {
			emit closeRequested(_pressIndex);
		} else {
			setCurrentIndex(_pressIndex);
			if (currentPage())
				currentPage()->setFocus();
		}
	}
	event->accept();
}

void CSessionTabs::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	// strip background
	p.fillRect(QRect(0, 0, width(), _barH), UiTheme::Surface);

	const int tw = tabWidth();
	int x = -_scroll;

	// Tabs live in the scrolling area only: the two pinned controls on the
	// right own everything from viewWidth() onwards.
	p.save();
	p.setClipRect(QRect(0, 0, viewWidth(), _barH));

	for (int i = 0; i < _pages.size(); ++i) {
		const QRect r(x, 5, tw, _barH - 5);
		const bool active = (i == _current);
		if (r.right() < -4 || r.left() > width() + 4) {
			x += tw + kTabGap;
			continue;
		}

		// Chrome-ish shape: rounded top corners, square bottom so the
		// active tab joins the terminal surface underneath.
		QPainterPath path;
		path.moveTo(r.left(), r.bottom());
		path.lineTo(r.left(), r.top() + kRadius);
		path.quadTo(r.left(), r.top(), r.left() + kRadius, r.top());
		path.lineTo(r.right() - kRadius, r.top());
		path.quadTo(r.right(), r.top(), r.right(), r.top() + kRadius);
		path.lineTo(r.right(), r.bottom());
		path.closeSubpath();

		p.fillPath(path, active ? UiTheme::Bg : UiTheme::Surface2);

		if (active) {
			QPen pen(UiTheme::Accent);
			pen.setWidth(3);
			p.setPen(pen);
			p.drawLine(r.left() + kRadius, r.top() + 1,
			           r.right() - kRadius, r.top() + 1);
		}
		// Inactive tabs carry no outline at all.  An outline around every
		// title made the bar read as a row of boxes; the surface step alone
		// separates them, which is what Chrome does too.

		// title
		p.setFont(UiTheme::uiFont(19, active));
		p.setPen(active ? UiTheme::Text : UiTheme::TextDim);
		QRect tr = r.adjusted(kPadH, 0, -kCloseWide, 0);
		const QString title = _titles.value(i);
		p.drawText(tr, Qt::AlignVCenter | Qt::AlignLeft,
		           QFontMetrics(p.font()).elidedText(title, Qt::ElideRight, tr.width()));

		// close affordance (always visible: no hover on a touch screen)
		const QRect cr(r.right() - kCloseWide, r.top(), kCloseWide, r.height());
		p.setFont(UiTheme::uiFont(24, false));
		p.setPen(active ? UiTheme::TextDim : UiTheme::Border);
		p.drawText(cr, Qt::AlignCenter, QString::fromUtf8("\xC3\x97")); // ×

		x += tw + kTabGap;
	}

	p.restore();

	// Fixed right-hand zone: [ + ][ gear ].  Painted after the tabs so a tab
	// scrolling underneath can never bleed into it.
	const int fixedX = width() - kPlusWidth - kGearWidth;
	p.fillRect(QRect(fixedX, 0, kPlusWidth + kGearWidth, _barH), UiTheme::Surface);

	// "+" - new session
	{
		const QRect pr(fixedX + 4, 10, kPlusWidth - 10, _barH - 20);
		p.setPen(Qt::NoPen);
		p.setBrush(Qt::NoBrush);
		QPen pen(UiTheme::Border);
		pen.setWidth(2);
		p.setPen(pen);
		p.drawRoundedRect(pr, 8, 8);
		pen.setColor(UiTheme::Accent);
		pen.setWidth(3);
		p.setPen(pen);
		const QPoint c = pr.center();
		p.drawLine(c.x() - 8, c.y(), c.x() + 8, c.y());
		p.drawLine(c.x(), c.y() - 8, c.x(), c.y() + 8);
	}

	// Gear - opens the settings card (settings / commands & SSH)
	{
		const QRect gr(width() - kGearWidth, 0, kGearWidth, _barH);
		if (_pressGear && _dragging) {
			p.setPen(Qt::NoPen);
			p.setBrush(UiTheme::withAlpha(UiTheme::Accent, 46));
			p.drawRoundedRect(gr.adjusted(6, 10, -6, -10), 8, 8);
			p.setBrush(Qt::NoBrush);
		}
		drawGear(p, gr.center(), UiTheme::Accent);
	}

	// baseline under the strip, but not under the active tab
	p.setPen(QPen(UiTheme::Border, 1));
	p.drawLine(0, _barH - 1, width(), _barH - 1);
	if (_current >= 0) {
		const QRect ar(_current * (tw + kTabGap) - _scroll, _barH - 3, tw, 3);
		p.fillRect(ar, UiTheme::Bg);
	}
}
