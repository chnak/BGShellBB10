/*
 * The IME candidate strip - see imepanel.h.
 *
 * Geometry: 52px tall with 22px glyphs. That is a bit more than two rows of
 * the default 22px terminal font, so the terminal only gives up two lines
 * while the IME is on - the old 78px / 30px strip ate three rows and looked
 * oversized. TerminalDisplay::setBottomReserve() keeps that space free under
 * the last row, so the strip never covers the line the cursor sits on.
 */
#include <QtCore/QEvent>
#include <QtGui/QColor>
#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>

#include "imepanel.h"
#include "uitheme.h"

namespace {
const int kStripHeight = 52;
const int kMargin      = 6;
const int kChipWidth   = 36;
const int kCellHeight  = 32;
const int kGap         = 8;
}

CImePanel::CImePanel(QWidget *parent)
	: QWidget(parent)
	, _page(0)
	, _pages(0)
	, _highlight(0)
	, _bottomOffset(0)
{
	setFocusPolicy(Qt::NoFocus);
	setVisible(false);
	// Idle strip: nothing to tap, so let touches reach the terminal.
	setAttribute(Qt::WA_TransparentForMouseEvents, true);

	_glyphFont.setPixelSize(22);
	_smallFont.setPixelSize(14);
}

void CImePanel::setCjkFontFamily(const QString &family)
{
	if (family.isEmpty())
		return;
	_glyphFont.setFamily(family);
	_smallFont.setFamily(family);
	relayout();
}

int CImePanel::stripHeight() const
{
	return kStripHeight;
}

void CImePanel::setState(const QString &buffer, const QStringList &candidates,
                         int pageIndex, int pageCount)
{
	_buffer     = buffer;
	_candidates = candidates;
	_page       = pageIndex;
	_pages      = pageCount;
	_highlight  = 0;        // a new page always starts at its first candidate
	relayout();
	// With no candidates on screen there is nothing to tap, so touches fall
	// through to the terminal (scrolling, selections) instead of stopping here.
	setAttribute(Qt::WA_TransparentForMouseEvents, idle());
	update();
}

void CImePanel::relayout()
{
	_candidateRects.clear();
	_prevRect = QRect();
	_nextRect = QRect();

	const QFontMetrics small(_smallFont);
	const QFontMetrics glyph(_glyphFont);
	const int top = (kStripHeight - kCellHeight) / 2;

	int x = kMargin + kChipWidth + kGap;
	// pinyin that has been typed so far
	x += small.width(_buffer) + kGap * 2;

	// reserve room for the "3/8  < >" block when there is more than one page
	const int reserved = _pages > 1 ? 88 : kMargin;
	const int right = width() - reserved;

	for (int i = 0; i < _candidates.size(); ++i) {
		const QString digit = QString::number(i + 1);
		const int w = small.width(digit) + 3 + glyph.width(_candidates.at(i)) + 12;
		if (x + w > right && !_candidateRects.isEmpty())
			break;          // the rest of the page stays reachable by number key
		_candidateRects.append(QRect(x, top, w, kCellHeight));
		x += w;
	}

	// the highlight can only ever point at something that is drawn
	if (_highlight >= _candidateRects.size())
		_highlight = 0;

	if (_pages > 1) {
		const int w = small.width(QString("%1/%2").arg(_page + 1).arg(_pages));
		_nextRect = QRect(kMargin + width() - 26, top, 22, kCellHeight);
		_prevRect = _nextRect.translated(-(26 + w + 6), 0);
	}
}

void CImePanel::popup()
{
	if (!parentWidget())
		return;
	const int w = parentWidget()->width();
	const QRect target(0, parentWidget()->height() - _bottomOffset - kStripHeight,
	                   w, kStripHeight);
	// setState() has already laid the candidates out for this width; only
	// a real geometry change (window resized, keyboard slid up) needs the
	// extra relayout - doing it on every key press just adds work to the
	// path that has to feel instant.
	if (geometry() != target) {
		setGeometry(target);
		relayout();
	}
	show();
	raise();
}

void CImePanel::dismiss()
{
	hide();
}

void CImePanel::setBottomOffset(int px)
{
	if (px < 0)
		px = 0;
	if (px == _bottomOffset)
		return;
	_bottomOffset = px;
	if (isVisible())
		popup();
}

void CImePanel::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	const QRect r = rect();

	// Same top-edge fade as the key strip below it: the candidate strip sits
	// directly on the last row of text, so a hard rule there reads as a lid.
	p.fillRect(r, UiTheme::Surface);
	{
		const int fade = (kStripHeight - kCellHeight) / 2;
		QLinearGradient g(0, r.top(), 0, r.top() + fade);
		g.setColorAt(0.0, UiTheme::Bg);
		g.setColorAt(1.0, UiTheme::Surface);
		p.fillRect(QRect(r.left(), r.top(), r.width(), fade), QBrush(g));
	}

	const int top = (kStripHeight - kCellHeight) / 2;

	// --- the 中 / EN chip ---------------------------------------------
	{
		const QRectF chip(kMargin, top, kChipWidth - 1, kCellHeight - 1);
		p.setPen(Qt::NoPen);
		p.setBrush(UiTheme::blend(UiTheme::Surface2, UiTheme::Accent, 16));
		p.drawRoundedRect(chip, 6, 6);
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(UiTheme::blend(UiTheme::Border, UiTheme::Accent, 45), 1));
		p.drawRoundedRect(chip, 6, 6);

		p.setFont(_glyphFont);
		p.setPen(UiTheme::Accent);
		p.drawText(chip.toRect(), Qt::AlignCenter, QString::fromUtf8("中"));
	}

	const QFontMetrics small(_smallFont);

	// --- the pinyin typed so far --------------------------------------
	int x = kMargin + kChipWidth + kGap;
	if (!_buffer.isEmpty()) {
		p.setFont(_smallFont);
		p.setPen(UiTheme::Accent);
		const QRect buf(x, top, small.width(_buffer) + 4, kCellHeight);
		p.drawText(buf, Qt::AlignVCenter | Qt::AlignLeft, _buffer);
		// caret
		p.drawLine(buf.right() + 2, top + 8, buf.right() + 2, top + kCellHeight - 8);
		x += small.width(_buffer) + kGap * 2;
	}
	else {
		// IME is on but nothing is being composed: say so instead of leaving
		// an empty band, and remind how to get out.
		p.setFont(_smallFont);
		p.setPen(UiTheme::TextDim);
		p.drawText(QRect(x, top, width() - x - kMargin, kCellHeight),
		           Qt::AlignVCenter | Qt::AlignLeft,
		           QString::fromUtf8("拼音输入 · 直接打字，Esc 取消"));
	}

	// --- candidates ----------------------------------------------------
	for (int i = 0; i < _candidates.size() && i < _candidateRects.size(); ++i) {
		const QRect cell = _candidateRects.at(i);

		// The one the trackpad / arrow keys are sitting on.  A tinted well,
		// not the old saturated green box: white glyphs on #005020 were hard
		// to read and the box shouted louder than the text it framed.
		if (i == _highlight) {
			const QRectF body(cell.x(), cell.y(),
			                  cell.width() - 1, cell.height() - 1);
			p.setPen(Qt::NoPen);
			p.setBrush(UiTheme::blend(UiTheme::Surface2, UiTheme::Accent, 22));
			p.drawRoundedRect(body, 6, 6);
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(UiTheme::blend(UiTheme::Border, UiTheme::Accent, 55), 1));
			p.drawRoundedRect(body, 6, 6);
		}

		const QString digit = QString::number(i + 1);
		const int numW = small.width(digit);

		p.setFont(_smallFont);
		p.setPen(UiTheme::TextDim);
		p.drawText(QRect(cell.left() + 2, cell.top(), numW + 2, kCellHeight),
		           Qt::AlignLeft | Qt::AlignVCenter, digit);

		p.setFont(_glyphFont);
		p.setPen(UiTheme::Text);
		p.drawText(QRect(cell.left() + numW + 5, cell.top(),
		                 cell.width() - numW - 6, kCellHeight),
		           Qt::AlignLeft | Qt::AlignVCenter, _candidates.at(i));
	}
	if (_candidates.isEmpty() && !_buffer.isEmpty()) {
		p.setFont(_smallFont);
		p.setPen(UiTheme::TextDim);
		p.drawText(QRect(x, top, width() - x - 40, kCellHeight),
		           Qt::AlignVCenter | Qt::AlignLeft,
		           QString::fromUtf8("没有这个拼音 / no candidate - Esc drops it"));
	}

	// --- paging --------------------------------------------------------
	if (_pages > 1) {
		p.setFont(_smallFont);
		p.setPen(QPen(UiTheme::TextDim, 1));
		p.drawText(_prevRect, Qt::AlignCenter, QString("<"));
		p.drawText(_nextRect, Qt::AlignCenter, QString(">"));

		const int w = small.width(QString("%1/%2").arg(_page + 1).arg(_pages));
		p.setPen(UiTheme::withAlpha(UiTheme::TextDim, 150));
		p.drawText(QRect(_prevRect.right() + 6, top, w + 4, kCellHeight),
		           Qt::AlignCenter, QString("%1/%2").arg(_page + 1).arg(_pages));
	}
}

void CImePanel::mousePressEvent(QMouseEvent *event)
{
	const QPoint pos = event->pos();

	for (int i = 0; i < _candidateRects.size(); ++i) {
		if (_candidateRects.at(i).contains(pos)) {
			emit candidateChosen(i);   // engine commits via its 1..8 keys
			event->accept();
			return;
		}
	}
	if (_pages > 1 && _prevRect.contains(pos)) {
		emit pageRequested(-1);
		event->accept();
		return;
	}
	if (_pages > 1 && _nextRect.contains(pos)) {
		emit pageRequested(+1);
		event->accept();
		return;
	}
	event->ignore();
}

void CImePanel::mouseDoubleClickEvent(QMouseEvent *event)
{
	// The candidate strip has no double-click meaning; a quick second tap is
	// just another tap. Qt routes it here instead of to mousePressEvent.
	mousePressEvent(event);
}

void CImePanel::moveHighlight(int delta)
{
	const int n = _candidateRects.size();
	if (n == 0 || delta == 0)
		return;

	// the trackpad streams one event per pixel, so pace the steps the way a
	// held arrow key repeats instead of flying over the page
	const QTime now = QTime::currentTime();
	if (_lastStep.isValid() && _lastStep.msecsTo(now) < 80)
		return;
	_lastStep = now;

	_highlight = (_highlight + delta) % n;
	if (_highlight < 0)
		_highlight += n;
	update();
}

bool CImePanel::hasHighlight() const
{
	return _highlight >= 0 && _highlight < _candidateRects.size();
}

void CImePanel::commitHighlighted()
{
	if (_highlight < 0 || _highlight >= _candidateRects.size())
		return;
	emit candidateChosen(_highlight);   // engine commits via its 1..8 keys
}
