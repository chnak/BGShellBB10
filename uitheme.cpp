#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QScrollBar>
#include <QtGui/QStyle>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>
#include "uitheme.h"

#include <QtCore/QSettings>
#include <QtCore/QTimerEvent>

#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QPalette>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFontDatabase>

// The bundled CJK face; empty when the asset is missing (see main.cpp).
extern QString gTerminalFontFamily;

namespace UiTheme {

const QColor Bg        (0x0b, 0x0e, 0x11);
const QColor Surface   (0x14, 0x1a, 0x1f);
const QColor Surface2  (0x1b, 0x22, 0x29);
const QColor Border    (0x2b, 0x35, 0x40);
const QColor Text      (0xe8, 0xed, 0xf2);
const QColor TextDim   (0x8d, 0x9a, 0xa7);
const QColor Accent    (0x00, 0xe0, 0x5a);
const QColor AccentInk (0x04, 0x14, 0x0a);
const QColor Danger    (0xff, 0x5f, 0x56);
const QColor Warn      (0xe3, 0xb3, 0x41);
const QColor Hi        (255, 255, 255, 20);
const QColor Sunken    (0x0e, 0x12, 0x16);

QString hex(const QColor &c)
{
	return c.name();
}

QColor blend(const QColor &base, const QColor &over, int percentOver)
{
	const int a = qBound(0, percentOver, 100);
	return QColor(base.red()   + ((over.red()   - base.red()) * a) / 100,
	              base.green() + ((over.green() - base.green()) * a) / 100,
	              base.blue()  + ((over.blue()  - base.blue()) * a) / 100);
}

QColor withAlpha(const QColor &c, int alpha)
{
	return QColor(c.red(), c.green(), c.blue(), qBound(0, alpha, 255));
}

QFont monoFont(int pixelSize, bool bold)
{
	QFont f(gTerminalFontFamily.isEmpty() ? QString("Courier New")
	                                      : gTerminalFontFamily);
	f.setPixelSize(pixelSize);
	f.setBold(bold);
	f.setStyleStrategy(QFont::PreferAntialias);
	return f;
}

QFont uiFont(int pixelSize, bool bold)
{
	QFont f = QApplication::font();
	f.setPixelSize(pixelSize);
	f.setBold(bold);
	return f;
}

QString titleStyle()
{
	return QString("QLabel { color: %1; font-size: 34px; font-weight: bold; }")
	    .arg(hex(Text));
}

QString subtitleStyle()
{
	return QString("QLabel { color: %1; font-size: 21px; }").arg(hex(TextDim));
}

QString sectionStyle()
{
	return QString("QLabel { color: %1; font-size: 19px; font-weight: bold;"
	               " letter-spacing: 1px; }")
	    .arg(hex(TextDim));
}

QWidget *newHeader(QWidget *parent, const QString &title, const QString &subtitle)
{
	QWidget *w = new QWidget(parent);
	QVBoxLayout *l = new QVBoxLayout(w);
	l->setContentsMargins(4, 0, 4, 0);
	l->setSpacing(2);

	QLabel *t = new QLabel(title, w);
	t->setStyleSheet(titleStyle());
	l->addWidget(t);

	if (!subtitle.isEmpty()) {
		QLabel *s = new QLabel(subtitle, w);
		s->setStyleSheet(subtitleStyle());
		s->setWordWrap(true);
		l->addWidget(s);
	}
	return w;
}

QFrame *newCard(QWidget *parent, const QString &title,
                const QString &subtitle, QVBoxLayout *&body)
{
	QFrame *card = new QFrame(parent);
	card->setObjectName(QString("card"));
	card->setStyleSheet(QString(
	    "QFrame#card { background-color: %1; border: 1px solid %2;"
	    " border-radius: 18px; }")
	    .arg(hex(Surface), hex(Border)));

	QVBoxLayout *outer = new QVBoxLayout(card);
	outer->setContentsMargins(18, 16, 18, 16);
	outer->setSpacing(8);

	if (!title.isEmpty()) {
		QLabel *t = new QLabel(title, card);
		t->setStyleSheet(sectionStyle());
		outer->addWidget(t);
	}
	if (!subtitle.isEmpty()) {
		QLabel *s = new QLabel(subtitle, card);
		s->setStyleSheet(subtitleStyle());
		s->setWordWrap(true);
		outer->addWidget(s);
	}

	QWidget *holder = new QWidget(card);
	body = new QVBoxLayout(holder);
	body->setContentsMargins(0, 6, 0, 0);
	body->setSpacing(10);
	outer->addWidget(holder);

	return card;
}

void styleScrollArea(QScrollArea *area, const QColor &fill)
{
	if (!area)
		return;

	area->setFrameShape(QFrame::NoFrame);
	area->setBackgroundRole(QPalette::NoRole);

	// The viewport is the surface that actually shows through, so it has to be
	// given our tone: the platform palette is paper white and Qt would happily
	// fill the whole form with it.
	QWidget *vp = area->viewport();
	if (vp) {
		QPalette p = vp->palette();
		p.setColor(QPalette::Window, fill);
		p.setColor(QPalette::Base, fill);
		vp->setPalette(p);
		vp->setBackgroundRole(QPalette::Window);
		vp->setAutoFillBackground(true);
	}

	// QScrollArea::setWidget() turns autoFillBackground() back on for the scrolled
	// widget; undo that so the form shows the viewport colour, not its own white.
	if (area->widget()) {
		area->widget()->setAutoFillBackground(false);
		QPalette wp = area->widget()->palette();
		wp.setColor(QPalette::Window, fill);
		area->widget()->setPalette(wp);
	}
}

void styleMenu(QMenu *menu)
{
	if (!menu)
		return;
	menu->setStyleSheet(QString(
	    "QMenu { background-color: %1; border: 1px solid %2;"
	    " border-radius: 16px; padding: 10px; }"
	    "QMenu::item { padding: 18px 34px 18px 24px; border-radius: 10px;"
	    " color: %3; font-size: 26px; }"
	    "QMenu::item:selected { background-color: #16301f; color: #ffffff; }"
	    "QMenu::separator { height: 1px; background-color: %2;"
	    " margin: 8px 14px; }")
	    .arg(hex(Surface), hex(Border), hex(Text)));
}

void apply(QApplication &app)
{
	// One family everywhere keeps the UI and the terminal visually related: the
	// bundled face has ASCII plus 6763 GB2312 ideographs, which covers every
	// string this app shows. Missing glyphs (a few symbols) fall back normally.
	QFont base = app.font();
	if (!gTerminalFontFamily.isEmpty())
		base.setFamily(gTerminalFontFamily);
	base.setPixelSize(24);
	app.setFont(base);

	QString s;

	// ---- buttons ------------------------------------------------------
	s += QString(
	    "QToolButton, QPushButton {"
	    "  background-color: %1;"
	    "  color: %2;"
	    "  border: 1px solid %3;"
	    "  border-radius: 14px;"
	    "  padding: 12px 18px;"
	    "  font-size: 25px;"
	    "}"
	    "QToolButton:pressed, QPushButton:pressed {"
	    "  background-color: #24303a; border-color: %4;"
	    "}"
	    "QToolButton:disabled, QPushButton:disabled {"
	    "  color: #5a6673; border-color: #202830;"
	    "}"
	    // primary action: green plate, dark ink
	    "QToolButton#primary, QPushButton#primary {"
	    "  background-color: %4; color: %5; border: 1px solid %4;"
	    "  font-weight: bold;"
	    "}"
	    "QToolButton#primary:pressed, QPushButton#primary:pressed {"
	    "  background-color: #00c24e; border-color: #00c24e;"
	    "}"
	    // quiet secondary
	    "QToolButton#secondary, QPushButton#secondary {"
	    "  background-color: transparent; border-color: %3; color: %2;"
	    "}"
	    "QToolButton#secondary:pressed, QPushButton#secondary:pressed {"
	    "  background-color: #1c262e;"
	    "}"
	    // destructive
	    "QToolButton#danger, QPushButton#danger {"
	    "  background-color: transparent; border-color: #3a2226;"
	    "  color: #ff8a80;"
	    "}"
	    "QToolButton#danger:pressed, QPushButton#danger:pressed {"
	    "  background-color: #2a1416;"
	    "}"
	    // square icon-ish button (close X)
	    "QToolButton#icon, QPushButton#icon {"
	    "  background-color: transparent; border: 1px solid %3;"
	    "  color: %6; padding: 0px; font-size: 30px;"
	    "}"
	    "QToolButton#icon:pressed, QPushButton#icon:pressed {"
	    "  background-color: #1c262e;"
	    "}"
	    // segmented tab
	    "QToolButton#segment {"
	    "  background-color: transparent; border: 1px solid %3;"
	    "  color: %6; font-size: 24px; padding: 10px 12px;"
	    "}"
	    "QToolButton#segment:checked {"
	    "  background-color: #11301f; border: 1px solid %4;"
	    "  color: %4; font-size: 24px; padding: 10px 12px; font-weight: bold;"
	    "}")
	    .arg(hex(Surface2), hex(Text), hex(Border), hex(Accent),
	         hex(AccentInk), hex(TextDim));

	// ---- text inputs --------------------------------------------------
	s += QString(
	    "QLineEdit {"
	    "  background-color: %1; color: %2; border: 1px solid %3;"
	    "  border-radius: 12px; padding: 10px 14px; font-size: 24px;"
	    "  selection-background-color: #1f4d33; selection-color: #ffffff;"
	    "}"
	    "QLineEdit:focus { border: 1px solid %4; }"
	    "QComboBox {"
	    "  background-color: %1; color: %2; border: 1px solid %3;"
	    "  border-radius: 12px; padding: 8px 14px; min-height: 46px;"
	    "  font-size: 24px;"
	    "}"
	    "QComboBox::drop-down { width: 40px; border: none; }"
	    "QComboBox QAbstractItemView {"
	    "  background-color: %5; color: %2; border: 1px solid %3;"
	    "  selection-background-color: #1f4d33; selection-color: #ffffff;"
	    "  outline: none; padding: 4px;"
	    "}"
	    "QCheckBox { color: %2; font-size: 24px; spacing: 14px; }"
	    "QCheckBox::indicator {"
	    "  width: 40px; height: 40px; border-radius: 10px;"
	    "  border: 1px solid %3; background-color: %1;"
	    "}"
	    "QCheckBox::indicator:checked {"
	    "  background-color: %4; border: 1px solid %4;"
	    "}")
	    .arg(hex(Surface2), hex(Text), hex(Border), hex(Accent), hex(Surface));

	// ---- lists / scrollers --------------------------------------------
	s += QString(
	    "QListWidget { background-color: transparent; border: none;"
	    "  outline: none; font-size: 26px; }"
	    "QListWidget::item { border: none; }"
	    "QScrollArea { border: none; background: transparent; }"
	    "QScrollBar:vertical { background: transparent; width: 26px;"
	    "  margin: 3px 0 3px 0; }"
	    "QScrollBar::handle:vertical { background-color: #33404b;"
	    "  border-radius: 13px; min-height: 110px; }"
	    "QScrollBar::handle:vertical:pressed { background-color: %1; }"
	    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
	    "  height: 0px; }"
	    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
	    "  background: transparent; }")
	    .arg(hex(Accent));

	// ---- menus, dialogs, misc -----------------------------------------
	s += QString(
	    "QMenu { background-color: %1; border: 1px solid %2;"
	    "  border-radius: 16px; padding: 10px; }"
	    "QMenu::item { padding: 18px 34px 18px 24px; border-radius: 10px;"
	    "  color: %3; font-size: 26px; }"
	    "QMenu::item:selected { background-color: #16301f; color: #ffffff; }"
	    "QMenu::separator { height: 1px; background-color: %2;"
	    "  margin: 8px 14px; }"
	    "QDialog, QMessageBox { background-color: %1; }"
	    "QMessageBox QLabel { color: %3; font-size: 24px; }"
	    "QGroupBox { border: 1px solid %2; border-radius: 16px;"
	    "  margin-top: 16px; padding: 16px; color: %4; }"
	    "QGroupBox::title { subcontrol-origin: margin; left: 18px;"
	    "  padding: 0 8px; font-size: 19px; }"
	    "QPlainTextEdit, QTextEdit {"
	    "  background-color: #0f1418; color: #d7e0ea;"
	    "  border: 1px solid %2; border-radius: 14px; padding: 10px;"
	    "  selection-background-color: #1f4d33;"
	    "}"
	    "QToolTip { background-color: %1; color: %3;"
	    "  border: 1px solid %2; padding: 6px; }")
	    .arg(hex(Surface), hex(Border), hex(Text), hex(TextDim));

	app.setStyleSheet(s);
}

} // namespace UiTheme

// ---- ScrollPane ------------------------------------------------------

namespace UiTheme {

ScrollPane::ScrollPane(QWidget *parent)
	: QScrollArea(parent)
	, _press(false)
	, _dragging(false)
	, _lastY(0)
{
	setFrameShape(QFrame::NoFrame);
	setWidgetResizable(true);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	// No ViewportUpdateMode here: QAbstractScrollArea does not have one on
	// Qt 4 (only QGraphicsView does).  The repaint is forced from the
	// scrollbar signal below instead.
	setFocusPolicy(Qt::StrongFocus);

	// Our own bar: track press jumps and drags, stock one only pages.
	ScrollBar *bar = new ScrollBar(Qt::Vertical, this);
	bar->setBackdrop(Bg);
	setVerticalScrollBar(bar);

	QWidget *vp = viewport();
	if (vp)
		vp->installEventFilter(this);

	QScrollBar *sb = verticalScrollBar();
	if (sb) {
		sb->setSingleStep(48);
		// QScrollArea::scrollContentsBy() blits the viewport and never clears
		// the pixels it moved away from; with a transparent content widget
		// the text survives as a trail.  Repaint the whole viewport on every
		// step.  720x720 is cheap enough on this hardware.
		connect(sb, SIGNAL(valueChanged(int)), vp, SLOT(update()));
	}
}

void ScrollPane::beginDrag(const QPoint &pos)
{
	_press = true;
	_dragging = false;
	_down = pos;
	_lastY = pos.y();
}

bool ScrollPane::dragTo(const QPoint &pos)
{
	if (!_press)
		return false;

	if (!_dragging) {
		// Stay out of the way until the finger has clearly moved: a tap has
		// to stay a tap so the widgets underneath still get their click.
		if (qAbs(pos.y() - _down.y()) < 14)
			return false;
		_dragging = true;
		_lastY = pos.y();
		return true;
	}

	QScrollBar *sb = verticalScrollBar();
	if (!sb)
		return false;

	const int dy = pos.y() - _lastY;
	if (dy) {
		sb->setValue(sb->value() - dy);
		_lastY = pos.y();
	}
	return true;
}

void ScrollPane::endDrag()
{
	_press = false;
	_dragging = false;
}

bool ScrollPane::eventFilter(QObject *watched, QEvent *event)
{
	switch (event->type()) {
	case QEvent::MouseButtonPress: {
		QMouseEvent *me = static_cast<QMouseEvent *>(event);
		if (me->button() == Qt::LeftButton) {
			QScrollBar *sb = verticalScrollBar();
			if (sb && sb->maximum() > sb->minimum())
				beginDrag(me->pos());
		}
		// never consume the press: child widgets still need it
		break;
	}
	case QEvent::MouseMove: {
		QMouseEvent *me = static_cast<QMouseEvent *>(event);
		if (_press && (me->buttons() & Qt::LeftButton) && dragTo(me->pos()))
			return true;
		break;
	}
	case QEvent::MouseButtonRelease:
		endDrag();
		break;
	default:
		break;
	}
	return QScrollArea::eventFilter(watched, event);
}

void ScrollPane::keyPressEvent(QKeyEvent *event)
{
	QScrollBar *sb = verticalScrollBar();
	if (!sb)
		return QScrollArea::keyPressEvent(event);

	switch (event->key()) {
	case Qt::Key_Up:       sb->setValue(sb->value() - 48); return;
	case Qt::Key_Down:     sb->setValue(sb->value() + 48); return;
	case Qt::Key_PageUp:   sb->setValue(sb->value() - sb->pageStep()); return;
	case Qt::Key_PageDown: sb->setValue(sb->value() + sb->pageStep()); return;
	case Qt::Key_Home:     sb->setValue(sb->minimum()); return;
	case Qt::Key_End:      sb->setValue(sb->maximum()); return;
	default: break;
	}
	QScrollArea::keyPressEvent(event);
}

} // namespace UiTheme

// ---- ScrollBar -------------------------------------------------------

namespace UiTheme {

// One process-wide appearance mode: the settings window flips it and every
// live bar repaints.  QSettings is the source of truth, so the choice
// survives a restart; auto-hide is the default because 720 px is narrow.
static ScrollBarMode gBarMode = BarAutoHide;
static bool gBarModeLoaded = false;

ScrollBarMode scrollBarModeFromConfig()
{
	QSettings s;
	const int v = s.value(QString("ScrollBar"), int(BarAutoHide)).toInt();
	if (v == BarAlways)
		return BarAlways;
	if (v == BarHidden)
		return BarHidden;
	return BarAutoHide;
}

ScrollBarMode scrollBarMode()
{
	if (!gBarModeLoaded) {
		gBarMode = scrollBarModeFromConfig();
		gBarModeLoaded = true;
	}
	return gBarMode;
}

void setScrollBarMode(ScrollBarMode mode)
{
	gBarMode = mode;
	gBarModeLoaded = true;

	// Repaint bars that are already on screen.  No Q_OBJECT on ScrollBar, so
	// qobject_cast is out and plain RTTI does the job.
	const QWidgetList all = QApplication::allWidgets();
	for (int i = 0; i < all.size(); ++i) {
		ScrollBar *bar = dynamic_cast<ScrollBar *>(all.at(i));
		if (bar)
			bar->update();
	}
}

ScrollBar::ScrollBar(Qt::Orientation orientation, QWidget *parent)
	: QScrollBar(orientation, parent)
	, _drag(false)
	, _thin(true)
	, _lastValue(0)
{
	setSingleStep(48);
	setFocusPolicy(Qt::NoFocus);

	// "Always" starts out wide; the other two start as a hairline.
	if (scrollBarMode() == BarAlways)
		_thin = false;
}

void ScrollBar::setBackdrop(const QColor &c)
{
	_backdrop = c;
	update();
}

void ScrollPane::setBackdrop(const QColor &c)
{
	ScrollBar *bar = dynamic_cast<ScrollBar *>(verticalScrollBar());
	if (bar)
		bar->setBackdrop(c);
}

// Vertical geometry only: where the handle sits and how tall it is.  The
// width depends on the current look and is decided by the painter.
void ScrollBar::handleGeometry(QRect &handle, int &trackTop, int &trackSpan) const
{
	const int margin = 6;
	trackTop = margin;
	trackSpan = qMax(1, height() - margin * 2);

	const int range = maximum() - minimum();
	const int page = qMax(1, pageStep());
	const int total = range + page;

	int handleH = int(double(trackSpan) * double(page) / double(total));
	if (handleH < 110)
		handleH = 110;
	if (handleH > trackSpan)
		handleH = trackSpan;

	int pos = 0;
	if (range > 0)
		pos = int(double(value() - minimum()) * double(trackSpan - handleH) / double(range) + 0.5);

	const int inkW = _thin ? 5 : 26;
	handle = QRect((width() - inkW) / 2, trackTop + pos, inkW, handleH);
}

int ScrollBar::valueForPos(int y) const
{
	QRect handle;
	int trackTop = 0;
	int trackSpan = 0;
	handleGeometry(handle, trackTop, trackSpan);

	// Nothing to move: the handle already fills the track.
	const int span = trackSpan - handle.height();
	if (span <= 0 || maximum() == minimum())
		return value();

	// Centre the handle under the finger, so grabbing near its edge does not
	// make the content jump.
	const int offset = y - trackTop - handle.height() / 2;
	double ratio = double(offset) / double(span);
	if (ratio < 0.0)
		ratio = 0.0;
	if (ratio > 1.0)
		ratio = 1.0;
	return minimum() + int(ratio * double(maximum() - minimum()) + 0.5);
}

void ScrollBar::kick()
{
	_thin = false;
	update();
	if (_idle.isActive())
		_idle.stop();
	_idle.start(1500, this);
}

void ScrollBar::sliderChange(SliderChange change)
{
	QScrollBar::sliderChange(change);
	// Any scroll - drag, wheel, arrow key, programmatic - wakes the bar up.
	if (change == SliderValueChange || change == SliderRangeChange)
		kick();
}

void ScrollBar::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == _idle.timerId()) {
		_idle.stop();
		if (scrollBarMode() == BarAutoHide && !_drag && !isSliderDown() && !_thin) {
			_thin = true;
			update();
		}
		return;
	}
	QScrollBar::timerEvent(event);
}

void ScrollBar::paintHandle(QPainter &p, bool wide)
{
	// Vertical geometry is shared with the drag mapping; only the width is
	// ours to choose, so the ink always tracks the finger.
	QRect geo;
	int trackTop = 0;
	int trackSpan = 0;
	handleGeometry(geo, trackTop, trackSpan);

	const int inkW = wide ? 26 : 5;
	QRect ink((width() - inkW) / 2, geo.top(), inkW, geo.height());

	QColor c;
	if (wide) {
		c = (_drag || isSliderDown()) ? Accent : QColor(0x33, 0x40, 0x4b);
	} else {
		c = QColor(255, 255, 255, 80); // hairline: just enough to be seen
	}

	p.setPen(Qt::NoPen);
	p.setBrush(c);
	const int r = inkW / 2;
	p.drawRoundedRect(ink, r, r);
}

void ScrollBar::paintEvent(QPaintEvent *)
{
	const ScrollBarMode mode = scrollBarMode();
	if (BarHidden == mode)
		return;
	// Nothing to scroll: draw nothing at all, in every mode.
	if (maximum() <= minimum())
		return;

	QPainter p(this);

	// The style no longer paints the track (we skip QScrollBar::paintEvent),
	// so the pixels left behind when the bar narrows have to be cleared here.
	// The area's background is transparent via the sheet, hence the explicit
	// backdrop colour instead of the palette.
	p.fillRect(rect(), _backdrop.isValid() ? _backdrop
	                                       : palette().color(QPalette::Window));

	p.setRenderHint(QPainter::Antialiasing, true);

	const bool wide = (BarAlways == mode) || !_thin || _drag || isSliderDown();
	paintHandle(p, wide);
}

void ScrollBar::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		QScrollBar::mousePressEvent(event);
		return;
	}

	_drag = true;
	setSliderDown(true);
	_thin = false;
	update();
	setValue(valueForPos(event->pos().y()));
	event->accept();
}

void ScrollBar::mouseMoveEvent(QMouseEvent *event)
{
	if (_drag && (event->buttons() & Qt::LeftButton)) {
		setValue(valueForPos(event->pos().y()));
		event->accept();
		return;
	}
	QScrollBar::mouseMoveEvent(event);
}

void ScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
	if (_drag) {
		_drag = false;
		setSliderDown(false);
		// Keep the bar wide for a moment after the finger leaves.
		kick();
		event->accept();
		return;
	}
	QScrollBar::mouseReleaseEvent(event);
}

} // namespace UiTheme
